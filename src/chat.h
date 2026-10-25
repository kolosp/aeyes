#pragma once
#include "providers/provider.h"
#include "settings/secretstore.h"
#include "settings/settings.h"
#include "store/historystore.h"

#include <QHash>
#include <QTimer>
#include <memory>

// The conversation currently shown in the window. Talks to a Provider and persists to HistoryStore.
class Chat : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(qint64 sessionId READ sessionId NOTIFY sessionChanged)
    Q_PROPERTY(QString title READ title NOTIFY sessionChanged)
    Q_PROPERTY(int count READ count NOTIFY messagesReset)
public:
    Chat(Settings *settings, SecretStore *secrets, HistoryStore *store, QNetworkAccessManager *nam, QObject *parent = nullptr);

    bool busy() const { return m_busy; }
    qint64 sessionId() const { return m_session; }
    QString title() const { return m_title; }
    int count() const { return m_msgs.size(); }

    // Start a fresh session from a screenshot (copied into the store).
    Q_INVOKABLE void startNew(const QString &imagePath, const QString &prompt);
    // Follow-up turn. `images` are local file paths or file:// URLs.
    Q_INVOKABLE void send(const QString &text, const QStringList &images);
    // Re-ask the last user turn, optionally with another model ("" = current).
    Q_INVOKABLE void regenerate(const QString &model = {});
    Q_INVOKABLE void stop();
    Q_INVOKABLE void open(qint64 session);
    Q_INVOKABLE QString textAt(int index) const;
    Q_INVOKABLE void reset();
    // Current transcript for the web view.
    Q_INVOKABLE QString transcriptJson();

Q_SIGNALS:
    void busyChanged();
    void sessionChanged();
    void messagesReset(); // re-send transcriptJson()
    void lastUpdated(const QString &textJson, const QString &status, const QString &error);
    void sessionSaved();

private:
    void ask(bool reuseLast);
    void flush();
    void finish(const QString &error);
    std::unique_ptr<Provider> makeProvider() const;

    Settings *m_settings;
    SecretStore *m_secrets;
    HistoryStore *m_store;
    QNetworkAccessManager *m_nam;

    QList<Message> m_msgs;
    qint64 m_session = 0;
    QString m_title;
    bool m_busy = false;
    bool m_dirty = false;
    QTimer m_flushTimer;
    std::unique_ptr<Provider> m_provider;
    QHash<QString, QString> m_thumbs;
};
