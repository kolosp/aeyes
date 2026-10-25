#include "capture/spectaclecapture.h"
#include "store/historystore.h"

#include <QImage>
#include <QTemporaryDir>
#include <QtTest>

class StoreTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void roundTrip()
    {
        QTemporaryDir dir;
        HistoryStore st(dir.path(), QStringLiteral("t1"));
        QVERIFY(st.isOpen());

        QImage(10, 10, QImage::Format_RGB32).save(dir.filePath(QStringLiteral("s.png")));
        const QString stored = st.storeImage(dir.filePath(QStringLiteral("s.png")));
        QVERIFY(QFile::exists(stored));

        const qint64 s = st.createSession(QStringLiteral("gemini"), QStringLiteral("Title"));
        Message u;
        u.role = QStringLiteral("user");
        u.text = QStringLiteral("what is 100% of _this_?");
        u.images << stored;
        u.id = st.addMessage(s, u);
        Message a;
        a.role = QStringLiteral("model");
        a.model = QStringLiteral("m1");
        a.status = QStringLiteral("streaming");
        a.id = st.addMessage(s, a);
        a.text = QStringLiteral("Answer with $x$");
        a.status = QStringLiteral("done");
        st.updateMessage(a);

        const auto msgs = st.messages(s);
        QCOMPARE(msgs.size(), 2);
        QCOMPARE(msgs[0].images, QStringList{stored});
        QCOMPARE(msgs[1].text, QStringLiteral("Answer with $x$"));
        QCOMPARE(msgs[1].status, QStringLiteral("done"));

        auto list = st.sessions();
        QCOMPARE(list.size(), 1);
        QCOMPARE(list[0].thumbnail, stored);
        QCOMPARE(list[0].model, QStringLiteral("m1"));

        // search covers messages and treats LIKE wildcards literally
        QCOMPARE(st.sessions(QStringLiteral("answer")).size(), 1);
        QCOMPARE(st.sessions(QStringLiteral("100%")).size(), 1);
        QCOMPARE(st.sessions(QStringLiteral("_th")).size(), 1);
        QCOMPARE(st.sessions(QStringLiteral("zzz")).size(), 0);
        QCOMPARE(st.sessions(QStringLiteral("%")).size(), 1); // literal % exists
        QCOMPARE(st.sessions(QStringLiteral("a_b")).size(), 0);

        st.deleteSession(s);
        QVERIFY(!QFile::exists(stored));
        QCOMPARE(st.sessions().size(), 0);
        QCOMPARE(st.messages(s).size(), 0);
    }

    void interruptedAnswerIsMarked()
    {
        QTemporaryDir dir;
        HistoryStore st(dir.path(), QStringLiteral("t2"));
        const qint64 s = st.createSession(QStringLiteral("gemini"), QStringLiteral("T"));
        Message a;
        a.role = QStringLiteral("model");
        a.status = QStringLiteral("streaming");
        st.addMessage(s, a);
        QCOMPARE(st.messages(s)[0].status, QStringLiteral("error"));
    }

    void purge()
    {
        QTemporaryDir dir;
        HistoryStore st(dir.path(), QStringLiteral("t3"));
        st.createSession(QStringLiteral("g"), QStringLiteral("new"));
        QCOMPARE(st.purgeOlderThan(0), 0);
        QCOMPARE(st.purgeOlderThan(1), 0);
        QCOMPARE(st.sessions().size(), 1);
    }

    void spectacleArgs()
    {
        QCOMPARE(SpectacleCapture::arguments(QStringLiteral("region"), QStringLiteral("/x.png")),
                 (QStringList{QStringLiteral("-b"), QStringLiteral("-n"), QStringLiteral("-r"), QStringLiteral("-o"), QStringLiteral("/x.png")}));
        QVERIFY(SpectacleCapture::arguments(QStringLiteral("activewindow"), QStringLiteral("/x.png")).contains(QStringLiteral("-a")));
        QVERIFY(SpectacleCapture::arguments(QStringLiteral("bogus"), QStringLiteral("/x.png")).contains(QStringLiteral("-r")));
    }
};

QTEST_MAIN(StoreTest)
#include "tst_store.moc"
