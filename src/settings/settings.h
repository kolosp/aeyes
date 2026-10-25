#pragma once
#include <KConfigGroup>
#include <KSharedConfig>
#include <QObject>
#include <QStringList>
#include <QUrl>

// User preferences, stored in ~/.config/aeyesrc. API keys are NOT here, see SecretStore.
class Settings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString provider READ provider WRITE setProvider NOTIFY changed) // "gemini" | "openai"
    Q_PROPERTY(QString geminiModel READ geminiModel WRITE setGeminiModel NOTIFY changed)
    Q_PROPERTY(bool geminiPaid READ geminiPaid WRITE setGeminiPaid NOTIFY changed)
    Q_PROPERTY(QString openaiBaseUrl READ openaiBaseUrl WRITE setOpenaiBaseUrl NOTIFY changed)
    Q_PROPERTY(QString openaiModel READ openaiModel WRITE setOpenaiModel NOTIFY changed)
    Q_PROPERTY(bool confirmBeforeSend READ confirmBeforeSend WRITE setConfirmBeforeSend NOTIFY changed)
    Q_PROPERTY(QString captureMode READ captureMode WRITE setCaptureMode NOTIFY changed) // region|screen|window|full
    Q_PROPERTY(QString systemPrompt READ systemPrompt WRITE setSystemPrompt NOTIFY changed)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY changed)
    Q_PROPERTY(int retentionDays READ retentionDays WRITE setRetentionDays NOTIFY changed)
    Q_PROPERTY(QStringList ackedProviders READ ackedProviders WRITE setAckedProviders NOTIFY changed)
    Q_PROPERTY(QStringList geminiModels READ geminiModels CONSTANT)

    // Derived, for UI
    Q_PROPERTY(QString activeModel READ activeModel WRITE setActiveModel NOTIFY changed)
    Q_PROPERTY(bool acknowledged READ acknowledged NOTIFY changed)
    Q_PROPERTY(QString dataPolicy READ dataPolicy NOTIFY changed)
    Q_PROPERTY(bool dataPolicyIsWarning READ dataPolicyIsWarning NOTIFY changed)

public:
    explicit Settings(QObject *parent = nullptr);

    QString provider() const { return m_grp.readEntry("provider", QStringLiteral("gemini")); }
    QString geminiModel() const { return m_grp.readEntry("geminiModel", QStringLiteral("gemini-3.5-flash-lite")); }
    bool geminiPaid() const { return m_grp.readEntry("geminiPaid", false); }
    QString openaiBaseUrl() const { return m_grp.readEntry("openaiBaseUrl", QStringLiteral("http://localhost:11434/v1")); }
    QString openaiModel() const { return m_grp.readEntry("openaiModel", QString()); }
    bool confirmBeforeSend() const { return m_grp.readEntry("confirmBeforeSend", true); }
    QString captureMode() const { return m_grp.readEntry("captureMode", QStringLiteral("region")); }
    QString systemPrompt() const;
    QString effectivePrompt() const; // + language instruction, what is actually sent
    QString language() const { return m_grp.readEntry("language", QString()); }
    int retentionDays() const { return m_grp.readEntry("retentionDays", 0); }
    QStringList ackedProviders() const { return m_grp.readEntry("ackedProviders", QStringList()); }
    QStringList geminiModels() const { return {QStringLiteral("gemini-3.5-flash-lite"), QStringLiteral("gemini-3.6-flash")}; }

    void setProvider(const QString &v) { set("provider", v); }
    void setGeminiModel(const QString &v) { set("geminiModel", v); }
    void setGeminiPaid(bool v) { set("geminiPaid", v); }
    void setOpenaiBaseUrl(const QString &v) { set("openaiBaseUrl", v); }
    void setOpenaiModel(const QString &v) { set("openaiModel", v); }
    void setConfirmBeforeSend(bool v) { set("confirmBeforeSend", v); }
    void setCaptureMode(const QString &v) { set("captureMode", v); }
    void setSystemPrompt(const QString &v) { set("systemPrompt", v); }
    void setLanguage(const QString &v) { set("language", v); }
    void setRetentionDays(int v) { set("retentionDays", v); }
    void setAckedProviders(const QStringList &v) { set("ackedProviders", v); }

    QString activeModel() const { return provider() == QLatin1String("gemini") ? geminiModel() : openaiModel(); }
    void setActiveModel(const QString &m);

    // The user has seen the data-handling notice for the current provider configuration.
    bool acknowledged() const { return ackedProviders().contains(ackKey()); }
    Q_INVOKABLE void acknowledge();

    // One-line, human readable statement of what happens to submitted screenshots.
    QString dataPolicy() const;
    bool dataPolicyIsWarning() const;
    bool openaiIsLocal() const;

    static QString defaultSystemPrompt();
    Q_INVOKABLE QString defaultPrompt() const { return defaultSystemPrompt(); }

Q_SIGNALS:
    void changed();

private:
    template<typename T>
    void set(const char *key, const T &v)
    {
        if (m_grp.readEntry(key, T()) == v && m_grp.hasKey(key))
            return;
        m_grp.writeEntry(key, v);
        m_cfg->sync();
        Q_EMIT changed();
    }
    QString ackKey() const;

    KSharedConfigPtr m_cfg;
    KConfigGroup m_grp;
};
