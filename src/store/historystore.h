#pragma once
#include "types.h"

#include <QDateTime>
#include <QSqlDatabase>

struct SessionInfo {
    qint64 id = 0;
    QString title;
    QDateTime updated;
    QString provider;
    QString model;
    QString thumbnail; // first screenshot, absolute path
    QString preview; // start of the first answer
};

// SQLite-backed storage for sessions (= conversations). Screenshots live as files next to the DB.
class HistoryStore
{
public:
    // `dir` is the data directory; pass a temp dir in tests.
    explicit HistoryStore(const QString &dir, const QString &connectionName = QStringLiteral("aeyes"));
    ~HistoryStore();
    bool isOpen() const { return m_db.isOpen(); }

    QString shotsDir() const { return m_dir + QStringLiteral("/shots"); }
    // Copies `src` into the shots dir under a unique name and returns the new path.
    QString storeImage(const QString &src) const;

    qint64 createSession(const QString &provider, const QString &title);
    void setTitle(qint64 session, const QString &title);
    qint64 addMessage(qint64 session, const Message &m);
    void updateMessage(const Message &m);
    QList<Message> messages(qint64 session) const;

    QList<SessionInfo> sessions(const QString &search = {}) const;
    void deleteSession(qint64 session);
    int purgeOlderThan(int days);

private:
    void exec(const QString &sql) const;

    QString m_dir;
    QSqlDatabase m_db;
};
