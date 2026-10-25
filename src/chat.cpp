#include "chat.h"
#include "imageutil.h"
#include "providers/geminiprovider.h"
#include "providers/openaiprovider.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>

Chat::Chat(Settings *settings, SecretStore *secrets, HistoryStore *store, QNetworkAccessManager *nam, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_secrets(secrets)
    , m_store(store)
    , m_nam(nam)
{
    m_flushTimer.setInterval(40);
    m_flushTimer.setSingleShot(true);
    connect(&m_flushTimer, &QTimer::timeout, this, &Chat::flush);
}

std::unique_ptr<Provider> Chat::makeProvider() const
{
    if (m_settings->provider() == QLatin1String("gemini"))
        return std::make_unique<GeminiProvider>(m_nam);
    return std::make_unique<OpenAiProvider>(m_nam);
}

void Chat::reset()
{
    stop();
    m_msgs.clear();
    m_session = 0;
    m_title.clear();
    Q_EMIT sessionChanged();
    Q_EMIT messagesReset();
}

void Chat::startNew(const QString &imagePath, const QString &prompt)
{
    reset();
    m_title = prompt.isEmpty() ? tr("Screenshot") : prompt.left(70);
    m_session = m_store->createSession(m_settings->provider(), m_title);
    Q_EMIT sessionChanged();
    send(prompt, {imagePath});
}

void Chat::send(const QString &text, const QStringList &images)
{
    if (m_busy)
        return;
    if (m_session == 0) {
        m_title = text.left(70);
        m_session = m_store->createSession(m_settings->provider(), m_title);
        Q_EMIT sessionChanged();
    }
    Message u;
    u.role = QStringLiteral("user");
    u.text = text.trimmed();
    for (const QString &i : images) {
        const QString local = i.startsWith(QLatin1String("file:")) ? QUrl(i).toLocalFile() : i;
        const QString stored = m_store->storeImage(local);
        if (!stored.isEmpty())
            u.images << stored;
    }
    if (u.text.isEmpty() && u.images.isEmpty())
        return;
    u.id = m_store->addMessage(m_session, u);
    m_msgs << u;
    ask(false);
}

void Chat::regenerate(const QString &model)
{
    if (m_busy || m_msgs.isEmpty())
        return;
    if (!model.isEmpty())
        m_settings->setActiveModel(model);
    ask(m_msgs.last().role == QLatin1String("model"));
}

void Chat::ask(bool reuseLast)
{
    // The conversation sent upstream excludes the (new) pending answer.
    QList<Message> conv;
    for (int i = 0; i < m_msgs.size() - (reuseLast ? 1 : 0); ++i)
        if (const Message &m = m_msgs.at(i); m.role == QLatin1String("user") || (!m.text.isEmpty() && m.status == QLatin1String("done")))
            conv << m;

    if (!reuseLast) {
        Message a;
        a.role = QStringLiteral("model");
        a.model = m_settings->activeModel();
        a.status = QStringLiteral("streaming");
        a.id = m_store->addMessage(m_session, a);
        m_msgs << a;
    } else {
        // regenerate(): the answer row already exists and is overwritten
        m_msgs.last().text.clear();
        m_msgs.last().error.clear();
        m_msgs.last().status = QStringLiteral("streaming");
        m_msgs.last().model = m_settings->activeModel();
        m_store->updateMessage(m_msgs.last());
    }

    ProviderConfig cfg;
    cfg.model = m_settings->activeModel();
    cfg.baseUrl = m_settings->provider() == QLatin1String("gemini") ? QString() : m_settings->openaiBaseUrl();
    cfg.systemPrompt = m_settings->effectivePrompt();
    cfg.apiKey = m_secrets->key(m_settings->provider());

    m_busy = true;
    Q_EMIT busyChanged();
    Q_EMIT messagesReset();

    m_provider = makeProvider();
    connect(m_provider.get(), &Provider::delta, this, [this](const QString &t) {
        m_msgs.last().text += t;
        m_dirty = true;
        if (!m_flushTimer.isActive())
            m_flushTimer.start();
    });
    connect(m_provider.get(), &Provider::finished, this, [this] { finish({}); });
    connect(m_provider.get(), &Provider::failed, this, [this](const QString &e) { finish(e); });
    m_provider->ask(conv, cfg);
}

void Chat::flush()
{
    if (!m_dirty || m_msgs.isEmpty())
        return;
    m_dirty = false;
    Q_EMIT lastUpdated(QString::fromUtf8(QJsonDocument(QJsonArray{m_msgs.last().text}).toJson(QJsonDocument::Compact)), {}, {});
}

void Chat::finish(const QString &error)
{
    if (!m_busy)
        return;
    m_flushTimer.stop();
    Message &a = m_msgs.last();
    if (!error.isEmpty()) {
        a.status = QStringLiteral("error");
        a.error = error;
    } else if (a.text.trimmed().isEmpty()) {
        a.status = QStringLiteral("error");
        a.error = tr("The model returned an empty answer.");
    } else {
        a.status = QStringLiteral("done");
    }
    m_store->updateMessage(a);

    if (a.status == QLatin1String("done") && (m_title.isEmpty() || m_title == tr("Screenshot"))) {
        QString first = a.text.trimmed().section(QLatin1Char('\n'), 0, 0);
        first.remove(QRegularExpression(QStringLiteral("^[#>*\\-\\s]+"))).remove(QLatin1Char('*')).remove(QLatin1Char('`')).remove(QLatin1Char('$'));
        m_title = first.left(70);
        m_store->setTitle(m_session, m_title);
        Q_EMIT sessionChanged();
    }
    m_busy = false;
    Q_EMIT busyChanged();
    Q_EMIT lastUpdated(QString::fromUtf8(QJsonDocument(QJsonArray{a.text}).toJson(QJsonDocument::Compact)), a.status, a.error);
    Q_EMIT sessionSaved();
    m_provider.release()->deleteLater();
}

void Chat::stop()
{
    if (!m_busy)
        return;
    m_provider->cancel();
    finish(tr("Stopped."));
}

void Chat::open(qint64 session)
{
    stop();
    m_msgs = m_store->messages(session);
    m_session = session;
    m_title.clear();
    for (const SessionInfo &s : m_store->sessions())
        if (s.id == session)
            m_title = s.title;
    Q_EMIT sessionChanged();
    Q_EMIT messagesReset();
}

QString Chat::textAt(int index) const
{
    return index >= 0 && index < m_msgs.size() ? m_msgs.at(index).text : QString();
}

QString Chat::transcriptJson()
{
    QJsonArray arr;
    for (const Message &m : std::as_const(m_msgs)) {
        QJsonArray thumbs;
        for (const QString &p : m.images) {
            if (!m_thumbs.contains(p))
                m_thumbs[p] = ImageUtil::thumbnailDataUrl(p);
            if (!m_thumbs[p].isEmpty())
                thumbs << m_thumbs[p];
        }
        arr << QJsonObject{{QStringLiteral("role"), m.role},
                           {QStringLiteral("text"), m.text},
                           {QStringLiteral("model"), m.model},
                           {QStringLiteral("status"), m.status},
                           {QStringLiteral("error"), m.error},
                           {QStringLiteral("thumbs"), thumbs}};
    }
    return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
