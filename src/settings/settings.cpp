#include "settings.h"

#include <KLocalizedString>
#include <QUrl>

Settings::Settings(QObject *parent)
    : QObject(parent)
    , m_cfg(KSharedConfig::openConfig(QStringLiteral("aeyesrc")))
    , m_grp(m_cfg, QStringLiteral("General"))
{
}

QString Settings::defaultSystemPrompt()
{
    return QStringLiteral(
        "You are a screen assistant. The user sends a screenshot of a region of their screen, "
        "usually something they find confusing. Briefly explain what it shows and answer any question it contains. "
        "If it is an error message, say what it means and how to fix it. If it is code, math, a form or a diagram, "
        "explain it step by step. Be concise and concrete. "
        "Reply in Markdown. Write math as LaTeX between $...$ (inline) or $$...$$ (display). Use fenced code blocks for code.");
}

QString Settings::systemPrompt() const
{
    const QString p = m_grp.readEntry("systemPrompt", QString());
    return p.trimmed().isEmpty() ? defaultSystemPrompt() : p;
}

QString Settings::effectivePrompt() const
{
    QString p = systemPrompt();
    const QString lang = language().trimmed();
    if (!lang.isEmpty())
        p += QStringLiteral(" Always answer in %1.").arg(lang);
    return p;
}

void Settings::setActiveModel(const QString &m)
{
    if (provider() == QLatin1String("gemini"))
        setGeminiModel(m);
    else
        setOpenaiModel(m);
}

QString Settings::ackKey() const
{
    // Acknowledgement is tied to what actually determines where data goes.
    if (provider() == QLatin1String("gemini"))
        return QStringLiteral("gemini:") + (geminiPaid() ? QStringLiteral("paid") : QStringLiteral("free"));
    return QStringLiteral("openai:") + QUrl(openaiBaseUrl()).host();
}

void Settings::acknowledge()
{
    QStringList l = ackedProviders();
    if (!l.contains(ackKey())) {
        l << ackKey();
        setAckedProviders(l);
    }
}

bool Settings::openaiIsLocal() const
{
    const QString h = QUrl(openaiBaseUrl()).host();
    return h == QLatin1String("localhost") || h.startsWith(QLatin1String("127.")) || h == QLatin1String("::1") || h.startsWith(QLatin1String("192.168.")) || h.startsWith(QLatin1String("10."));
}

bool Settings::dataPolicyIsWarning() const
{
    if (provider() == QLatin1String("gemini"))
        return !geminiPaid();
    return !openaiIsLocal();
}

QString Settings::dataPolicy() const
{
    if (provider() == QLatin1String("gemini")) {
        if (geminiPaid())
            return i18n("Gemini API (paid tier): Google states that submitted content is not used to improve its models.");
        return i18n("Gemini API (free tier): Google may use everything you send, including screenshots and any sensitive "
                    "content visible in them, to improve its products and models, and human reviewers may read it.");
    }
    if (openaiIsLocal())
        return i18n("Local server (%1): screenshots stay on your network.", QUrl(openaiBaseUrl()).host());
    return i18n("Remote server (%1): screenshots are sent there. Check its data retention and training policy.", QUrl(openaiBaseUrl()).host());
}
