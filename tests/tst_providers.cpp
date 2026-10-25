#include "providers/geminiprovider.h"
#include "providers/openaiprovider.h"

#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

// Tiny one-shot HTTP server that records the request and replies with a canned response.
class MockServer : public QObject
{
    Q_OBJECT
public:
    MockServer()
    {
        connect(&m_server, &QTcpServer::newConnection, this, [this] {
            m_sock = m_server.nextPendingConnection();
            connect(m_sock, &QTcpSocket::readyRead, this, &MockServer::onData);
        });
        m_server.listen(QHostAddress::LocalHost);
    }
    QString url() const { return QStringLiteral("http://127.0.0.1:%1").arg(m_server.serverPort()); }
    QByteArray request;
    int status = 200;
    QByteArray body;

private:
    void onData()
    {
        request += m_sock->readAll();
        const int hdrEnd = request.indexOf("\r\n\r\n");
        if (hdrEnd < 0)
            return;
        const int cl = QRegularExpression(QStringLiteral("Content-Length: (\\d+)"), QRegularExpression::CaseInsensitiveOption)
                           .match(QString::fromLatin1(request.left(hdrEnd)))
                           .captured(1)
                           .toInt();
        if (request.size() < hdrEnd + 4 + cl)
            return;
        m_sock->write("HTTP/1.1 " + QByteArray::number(status) + " X\r\nContent-Type: text/event-stream\r\nContent-Length: " + QByteArray::number(body.size())
                      + "\r\nConnection: close\r\n\r\n" + body);
        m_sock->disconnectFromHost();
    }
    QTcpServer m_server;
    QTcpSocket *m_sock = nullptr;
};

class ProvidersTest : public QObject
{
    Q_OBJECT
    QTemporaryDir dir;
    QString img;
    QNetworkAccessManager nam;

    QList<Message> conv() const
    {
        Message u;
        u.role = QStringLiteral("user");
        u.text = QStringLiteral("what is this?");
        u.images << img;
        Message m;
        m.role = QStringLiteral("model");
        m.text = QStringLiteral("a square");
        Message u2;
        u2.role = QStringLiteral("user");
        u2.text = QStringLiteral("and now?");
        return {u, m, u2};
    }

    static QString run(Provider &p, const ProviderConfig &cfg, const QList<Message> &c, QString *error = nullptr)
    {
        QString out;
        QObject::connect(&p, &Provider::delta, &p, [&](const QString &t) { out += t; });
        QSignalSpy fin(&p, &Provider::finished);
        QSignalSpy fail(&p, &Provider::failed);
        p.ask(c, cfg);
        for (int i = 0; i < 500 && fin.count() + fail.count() == 0; ++i)
            QTest::qWait(10);
        if (fin.count() + fail.count() != 1)
            return QStringLiteral("<TIMEOUT>");
        if (error && !fail.isEmpty())
            *error = fail.first().first().toString();
        return out;
    }

private Q_SLOTS:
    void initTestCase()
    {
        img = dir.filePath(QStringLiteral("a.png"));
        QImage(40, 30, QImage::Format_RGB32).save(img);
    }

    void geminiStreams()
    {
        MockServer s;
        s.body = "data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"Hel\"}]}}]}\r\n\r\n"
                 "data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"hidden\",\"thought\":true},{\"text\":\"lo $x^2$\"}]}}]}\r\n\r\n";
        GeminiProvider p(&nam);
        ProviderConfig cfg{QStringLiteral("gemini-x"), QStringLiteral("KEY"), s.url(), QStringLiteral("be brief")};
        QCOMPARE(run(p, cfg, conv()), QStringLiteral("Hello $x^2$"));

        QVERIFY(s.request.startsWith("POST /v1beta/models/gemini-x:streamGenerateContent?alt=sse"));
        QVERIFY(s.request.toLower().contains("x-goog-api-key: key"));
        const auto body = QJsonDocument::fromJson(s.request.mid(s.request.indexOf("\r\n\r\n") + 4)).object();
        const auto contents = body.value(QStringLiteral("contents")).toArray();
        QCOMPARE(contents.size(), 3);
        QCOMPARE(contents[1].toObject().value(QStringLiteral("role")).toString(), QStringLiteral("model"));
        QVERIFY(contents[0].toObject().value(QStringLiteral("parts")).toArray()[1].toObject().contains(QStringLiteral("inlineData")));
        QVERIFY(body.contains(QStringLiteral("systemInstruction")));
    }

    void geminiHttpError()
    {
        MockServer s;
        s.status = 400;
        s.body = "{\"error\":{\"code\":400,\"message\":\"API key not valid\"}}";
        GeminiProvider p(&nam);
        QString err;
        run(p, {QStringLiteral("m"), QStringLiteral("bad"), s.url(), {}}, conv(), &err);
        QVERIFY2(err.contains(QStringLiteral("API key not valid")), qPrintable(err));
        QVERIFY(err.contains(QStringLiteral("400")));
    }

    void geminiNoKey()
    {
        GeminiProvider p(&nam);
        QString err;
        run(p, {QStringLiteral("m"), {}, {}, {}}, conv(), &err);
        QVERIFY(err.contains(QStringLiteral("API key")));
    }

    void openaiStreams()
    {
        MockServer s;
        s.body = "data: {\"choices\":[{\"delta\":{\"content\":\"Hi\"}}]}\n\n"
                 "data: {\"choices\":[{\"delta\":{\"content\":\" there\"}}]}\n\n"
                 "data: [DONE]\n\n";
        OpenAiProvider p(&nam);
        QCOMPARE(run(p, {QStringLiteral("llava"), {}, s.url() + QStringLiteral("/v1/"), QStringLiteral("sys")}, conv()), QStringLiteral("Hi there"));
        QVERIFY(s.request.startsWith("POST /v1/chat/completions"));
        QVERIFY(!s.request.toLower().contains("authorization"));
        const auto body = QJsonDocument::fromJson(s.request.mid(s.request.indexOf("\r\n\r\n") + 4)).object();
        const auto msgs = body.value(QStringLiteral("messages")).toArray();
        QCOMPARE(msgs.size(), 4); // system + 3
        QCOMPARE(msgs[2].toObject().value(QStringLiteral("role")).toString(), QStringLiteral("assistant"));
        QVERIFY(msgs[2].toObject().value(QStringLiteral("content")).isString());
        QCOMPARE(msgs[1].toObject().value(QStringLiteral("content")).toArray()[1].toObject().value(QStringLiteral("type")).toString(), QStringLiteral("image_url"));
    }

    void cancelStops()
    {
        GeminiProvider p(&nam);
        QSignalSpy fin(&p, &Provider::finished);
        QSignalSpy fail(&p, &Provider::failed);
        p.ask(conv(), {QStringLiteral("m"), QStringLiteral("k"), QStringLiteral("http://127.0.0.1:9"), {}});
        p.cancel();
        QTest::qWait(200);
        QCOMPARE(fin.count() + fail.count(), 0);
    }
};

QTEST_MAIN(ProvidersTest)
#include "tst_providers.moc"
