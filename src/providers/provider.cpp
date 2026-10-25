#include "provider.h"

#include <QJsonDocument>
#include <QJsonObject>

SseProvider::SseProvider(QNetworkAccessManager *nam, QObject *parent)
    : Provider(parent)
    , m_nam(nam)
{
}

void SseProvider::ask(const QList<Message> &conv, const ProviderConfig &cfg)
{
    cancel();
    m_buffer.clear();
    m_errorBody.clear();
    m_done = false;

    QNetworkRequest req;
    QString err;
    const QByteArray body = buildRequest(conv, cfg, req, err);
    if (body.isEmpty()) {
        emitError(err);
        return;
    }
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Accept", "text/event-stream");
    req.setTransferTimeout(120000);
    m_reply = m_nam->post(req, body);
    connect(m_reply, &QNetworkReply::readyRead, this, &SseProvider::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &SseProvider::onFinished);
}

void SseProvider::cancel()
{
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    m_done = true;
}

void SseProvider::emitError(const QString &t)
{
    if (m_done)
        return;
    m_done = true;
    Q_EMIT failed(t);
}

QString SseProvider::errorFromBody(const QByteArray &body) const
{
    const auto obj = QJsonDocument::fromJson(body).object();
    const auto e = obj.value(QStringLiteral("error"));
    if (e.isObject())
        return e.toObject().value(QStringLiteral("message")).toString();
    if (e.isString())
        return e.toString();
    return QString::fromUtf8(body.left(500));
}

void SseProvider::onReadyRead()
{
    const QByteArray chunk = m_reply->readAll();
    const int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status >= 400) {
        m_errorBody += chunk;
        return;
    }
    m_buffer += chunk;
    consume(false);
}

void SseProvider::consume(bool flush)
{
    m_buffer.replace("\r\n", "\n");
    while (!m_done) {
        int end = m_buffer.indexOf("\n\n");
        QByteArray event;
        if (end >= 0) {
            event = m_buffer.left(end);
            m_buffer.remove(0, end + 2);
        } else if (flush && !m_buffer.trimmed().isEmpty()) {
            event = m_buffer;
            m_buffer.clear();
        } else {
            return;
        }
        QByteArray data;
        for (const QByteArray &line : event.split('\n')) {
            if (line.startsWith("data:")) {
                if (!data.isEmpty())
                    data += '\n';
                data += line.mid(5).trimmed();
            }
        }
        if (!data.isEmpty() && !handleEvent(data)) {
            m_done = true;
            Q_EMIT finished();
            return;
        }
    }
}

void SseProvider::onFinished()
{
    QNetworkReply *r = m_reply;
    if (!r || m_done)
        return;
    const int status = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray rest = r->readAll();
    if (status >= 400) {
        m_errorBody += rest;
        emitError(QStringLiteral("HTTP %1: %2").arg(status).arg(errorFromBody(m_errorBody)));
    } else if (r->error() != QNetworkReply::NoError) {
        emitError(r->errorString());
    } else {
        m_buffer += rest;
        consume(true);
        if (!m_done) {
            m_done = true;
            Q_EMIT finished();
        }
    }
    r->deleteLater();
    m_reply = nullptr;
}
