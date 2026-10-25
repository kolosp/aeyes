#pragma once
#include "types.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>

// Streams a model response for a conversation. Emits delta() with text chunks, then exactly one
// of finished() / failed().
class Provider : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    virtual void ask(const QList<Message> &conversation, const ProviderConfig &cfg) = 0;
    virtual void cancel() = 0;

Q_SIGNALS:
    void delta(const QString &text);
    void finished();
    void failed(const QString &message);
};

// Shared HTTP + Server-Sent-Events plumbing. Subclasses build the request and parse events.
class SseProvider : public Provider
{
    Q_OBJECT
public:
    explicit SseProvider(QNetworkAccessManager *nam, QObject *parent = nullptr);
    void ask(const QList<Message> &conversation, const ProviderConfig &cfg) override;
    void cancel() override;

protected:
    // Fill url/headers and return the JSON body. Return an empty body (and set `error`) on failure.
    virtual QByteArray buildRequest(const QList<Message> &conv, const ProviderConfig &cfg, QNetworkRequest &req, QString &error) = 0;
    // Handle one SSE `data:` payload. Return false when the stream is complete.
    virtual bool handleEvent(const QByteArray &data) = 0;
    // Extract a human readable message from a non-2xx response body.
    virtual QString errorFromBody(const QByteArray &body) const;

    void emitDelta(const QString &t) { Q_EMIT delta(t); }
    void emitError(const QString &t);

private:
    void onReadyRead();
    void onFinished();
    void consume(bool flush);

    QNetworkAccessManager *m_nam;
    QPointer<QNetworkReply> m_reply;
    QByteArray m_buffer; // pending SSE bytes
    QByteArray m_errorBody; // body of a failed response
    bool m_done = false;
};
