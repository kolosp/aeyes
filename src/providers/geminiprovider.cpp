#include "geminiprovider.h"
#include "imageutil.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

QByteArray GeminiProvider::buildRequest(const QList<Message> &conv, const ProviderConfig &cfg, QNetworkRequest &req, QString &error)
{
    if (cfg.apiKey.isEmpty()) {
        error = QStringLiteral("No Gemini API key configured.");
        return {};
    }
    const QString base = cfg.baseUrl.isEmpty() ? defaultBaseUrl() : cfg.baseUrl;
    req.setUrl(QUrl(QStringLiteral("%1/v1beta/models/%2:streamGenerateContent?alt=sse").arg(base, cfg.model)));
    req.setRawHeader("x-goog-api-key", cfg.apiKey.toUtf8());

    QJsonArray contents;
    for (const Message &m : conv) {
        QJsonArray parts;
        if (!m.text.isEmpty())
            parts.append(QJsonObject{{QStringLiteral("text"), m.text}});
        for (const QString &path : m.images) {
            QByteArray b64;
            QString mime;
            if (!ImageUtil::encodeForUpload(path, b64, mime))
                continue;
            parts.append(QJsonObject{{QStringLiteral("inlineData"),
                                      QJsonObject{{QStringLiteral("mimeType"), mime}, {QStringLiteral("data"), QString::fromLatin1(b64)}}}});
        }
        if (parts.isEmpty())
            continue;
        contents.append(QJsonObject{{QStringLiteral("role"), m.role == QLatin1String("user") ? QStringLiteral("user") : QStringLiteral("model")},
                                    {QStringLiteral("parts"), parts}});
    }
    QJsonObject body{{QStringLiteral("contents"), contents}};
    if (!cfg.systemPrompt.isEmpty())
        body.insert(QStringLiteral("systemInstruction"),
                    QJsonObject{{QStringLiteral("parts"), QJsonArray{QJsonObject{{QStringLiteral("text"), cfg.systemPrompt}}}}});
    return QJsonDocument(body).toJson(QJsonDocument::Compact);
}

bool GeminiProvider::handleEvent(const QByteArray &data)
{
    const auto obj = QJsonDocument::fromJson(data).object();
    if (obj.contains(QStringLiteral("error"))) {
        emitError(obj.value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString());
        return false;
    }
    const auto block = obj.value(QStringLiteral("promptFeedback")).toObject().value(QStringLiteral("blockReason")).toString();
    if (!block.isEmpty()) {
        emitError(QStringLiteral("Request blocked by Gemini (%1).").arg(block));
        return false;
    }
    const auto cand = obj.value(QStringLiteral("candidates")).toArray().first().toObject();
    for (const auto &p : cand.value(QStringLiteral("content")).toObject().value(QStringLiteral("parts")).toArray()) {
        const auto part = p.toObject();
        if (part.value(QStringLiteral("thought")).toBool())
            continue; // thinking summaries are not part of the answer
        const QString t = part.value(QStringLiteral("text")).toString();
        if (!t.isEmpty())
            emitDelta(t);
    }
    return true;
}
