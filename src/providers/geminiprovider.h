#pragma once
#include "provider.h"

class GeminiProvider : public SseProvider
{
    Q_OBJECT
public:
    using SseProvider::SseProvider;
    static QString defaultBaseUrl() { return QStringLiteral("https://generativelanguage.googleapis.com"); }

protected:
    QByteArray buildRequest(const QList<Message> &conv, const ProviderConfig &cfg, QNetworkRequest &req, QString &error) override;
    bool handleEvent(const QByteArray &data) override;
};
