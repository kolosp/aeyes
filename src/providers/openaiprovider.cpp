#include "openaiprovider.h"
#include "imageutil.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

QByteArray OpenAiProvider::buildRequest(const QList<Message> &conv, const ProviderConfig &cfg, QNetworkRequest &req, QString &error)
{
    QString base = cfg.baseUrl.isEmpty() ? defaultBaseUrl() : cfg.baseUrl;
    while (base.endsWith(QLatin1Char('/')))
        base.chop(1);
    req.setUrl(QUrl(base + QStringLiteral("/chat/completions")));
    if (!cfg.apiKey.isEmpty())
        req.setRawHeader("Authorization", "Bearer " + cfg.apiKey.toUtf8());
    if (cfg.model.isEmpty()) {
        error = QStringLiteral("No model configured.");
        return {};
    }

    QJsonArray messages;
    if (!cfg.systemPrompt.isEmpty())
        messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("system")}, {QStringLiteral("content"), cfg.systemPrompt}});
    for (const Message &m : conv) {
        QJsonArray content;
        if (!m.text.isEmpty())
            content.append(QJsonObject{{QStringLiteral("type"), QStringLiteral("text")}, {QStringLiteral("text"), m.text}});
        for (const QString &path : m.images) {
            QByteArray b64;
            QString mime;
            if (!ImageUtil::encodeForUpload(path, b64, mime))
                continue;
            content.append(QJsonObject{{QStringLiteral("type"), QStringLiteral("image_url")},
                                       {QStringLiteral("image_url"),
                                        QJsonObject{{QStringLiteral("url"), QStringLiteral("data:%1;base64,%2").arg(mime, QString::fromLatin1(b64))}}}});
        }
        if (content.isEmpty())
            continue;
        const bool user = m.role == QLatin1String("user");
        // Assistant turns are plain strings; some servers reject content arrays there.
        messages.append(QJsonObject{{QStringLiteral("role"), user ? QStringLiteral("user") : QStringLiteral("assistant")},
                                    {QStringLiteral("content"), user ? QJsonValue(content) : QJsonValue(m.text)}});
    }
    return QJsonDocument(QJsonObject{{QStringLiteral("model"), cfg.model}, {QStringLiteral("stream"), true}, {QStringLiteral("messages"), messages}})
        .toJson(QJsonDocument::Compact);
}

bool OpenAiProvider::handleEvent(const QByteArray &data)
{
    if (data == "[DONE]")
        return false;
    const auto obj = QJsonDocument::fromJson(data).object();
    if (obj.contains(QStringLiteral("error"))) {
        const auto e = obj.value(QStringLiteral("error"));
        emitError(e.isObject() ? e.toObject().value(QStringLiteral("message")).toString() : e.toString());
        return false;
    }
    const auto choice = obj.value(QStringLiteral("choices")).toArray().first().toObject();
    const QString t = choice.value(QStringLiteral("delta")).toObject().value(QStringLiteral("content")).toString();
    if (!t.isEmpty())
        emitDelta(t);
    return true;
}
