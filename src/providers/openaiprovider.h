#pragma once
#include "provider.h"

// OpenAI-compatible /chat/completions (OpenAI, OpenRouter, Ollama, LM Studio, ...).
class OpenAiProvider : public SseProvider
{
    Q_OBJECT
public:
    using SseProvider::SseProvider;
    static QString defaultBaseUrl() { return QStringLiteral("https://api.openai.com/v1"); }

protected:
    QByteArray buildRequest(const QList<Message> &conv, const ProviderConfig &cfg, QNetworkRequest &req, QString &error) override;
    bool handleEvent(const QByteArray &data) override;
};
