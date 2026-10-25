#pragma once
#include <QList>
#include <QString>
#include <QStringList>

// One turn of a conversation. `images` are absolute paths of PNG/JPEG files on disk.
struct Message {
    qint64 id = 0;
    QString role; // "user" | "model"
    QString text;
    QString model; // for role == "model"
    QStringList images;
    QString status = QStringLiteral("done"); // "streaming" | "done" | "error"
    QString error;
};

struct ProviderConfig {
    QString model;
    QString apiKey;
    QString baseUrl;
    QString systemPrompt;
};
