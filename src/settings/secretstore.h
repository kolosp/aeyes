#pragma once
#include <QHash>
#include <QObject>

// API keys live in the user's wallet (KWallet / Secret Service) via QtKeychain.
// Environment variables GEMINI_API_KEY / OPENAI_API_KEY take precedence when set.
class SecretStore : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool walletAvailable READ walletAvailable NOTIFY changed)
public:
    explicit SecretStore(QObject *parent = nullptr);

    // Blocks (local event loop, short timeout) until the first read of that provider finished.
    Q_INVOKABLE QString key(const QString &provider);
    Q_INVOKABLE void setKey(const QString &provider, const QString &value);
    Q_INVOKABLE bool hasKey(const QString &provider);
    bool walletAvailable() const { return m_walletOk; }

Q_SIGNALS:
    void changed();

private:
    void load(const QString &provider);

    QHash<QString, QString> m_keys;
    QHash<QString, bool> m_loaded;
    bool m_walletOk = true;
};
