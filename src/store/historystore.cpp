#include "historystore.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

HistoryStore::HistoryStore(const QString &dir, const QString &connectionName)
    : m_dir(dir)
{
    QDir().mkpath(shotsDir());
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    m_db.setDatabaseName(dir + QStringLiteral("/history.db"));
    if (!m_db.open()) {
        qWarning() << "history db:" << m_db.lastError().text();
        return;
    }
    exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    exec(QStringLiteral("CREATE TABLE IF NOT EXISTS session ("
                        " id INTEGER PRIMARY KEY, created_at INTEGER NOT NULL, updated_at INTEGER NOT NULL,"
                        " provider TEXT, title TEXT)"));
    exec(QStringLiteral("CREATE TABLE IF NOT EXISTS message ("
                        " id INTEGER PRIMARY KEY, session_id INTEGER NOT NULL REFERENCES session(id) ON DELETE CASCADE,"
                        " seq INTEGER NOT NULL, role TEXT NOT NULL, model TEXT, text_md TEXT, images TEXT,"
                        " status TEXT, error TEXT)"));
    exec(QStringLiteral("CREATE INDEX IF NOT EXISTS message_session ON message(session_id, seq)"));
}

HistoryStore::~HistoryStore()
{
    const QString name = m_db.connectionName();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(name);
}

void HistoryStore::exec(const QString &sql) const
{
    QSqlQuery q(m_db);
    if (!q.exec(sql))
        qWarning() << "sql:" << q.lastError().text() << sql;
}

QString HistoryStore::storeImage(const QString &src) const
{
    const QString ext = QFileInfo(src).suffix().isEmpty() ? QStringLiteral("png") : QFileInfo(src).suffix();
    const QString dst = shotsDir() + QLatin1Char('/') + QUuid::createUuid().toString(QUuid::WithoutBraces) + QLatin1Char('.') + ext;
    return QFile::copy(src, dst) ? dst : QString();
}

qint64 HistoryStore::createSession(const QString &provider, const QString &title)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO session(created_at, updated_at, provider, title) VALUES(?,?,?,?)"));
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    q.addBindValue(now);
    q.addBindValue(now);
    q.addBindValue(provider);
    q.addBindValue(title);
    q.exec();
    return q.lastInsertId().toLongLong();
}

void HistoryStore::setTitle(qint64 session, const QString &title)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE session SET title=? WHERE id=?"));
    q.addBindValue(title);
    q.addBindValue(session);
    q.exec();
}

static QString imagesToJson(const QStringList &l)
{
    return QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(l)).toJson(QJsonDocument::Compact));
}

qint64 HistoryStore::addMessage(qint64 session, const Message &m)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO message(session_id, seq, role, model, text_md, images, status, error)"
                             " VALUES(?, (SELECT COALESCE(MAX(seq),0)+1 FROM message WHERE session_id=?), ?,?,?,?,?,?)"));
    q.addBindValue(session);
    q.addBindValue(session);
    q.addBindValue(m.role);
    q.addBindValue(m.model);
    q.addBindValue(m.text);
    q.addBindValue(imagesToJson(m.images));
    q.addBindValue(m.status);
    q.addBindValue(m.error);
    q.exec();
    QSqlQuery t(m_db);
    t.prepare(QStringLiteral("UPDATE session SET updated_at=? WHERE id=?"));
    t.addBindValue(QDateTime::currentSecsSinceEpoch());
    t.addBindValue(session);
    t.exec();
    return q.lastInsertId().toLongLong();
}

void HistoryStore::updateMessage(const Message &m)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE message SET model=?, text_md=?, images=?, status=?, error=? WHERE id=?"));
    q.addBindValue(m.model);
    q.addBindValue(m.text);
    q.addBindValue(imagesToJson(m.images));
    q.addBindValue(m.status);
    q.addBindValue(m.error);
    q.addBindValue(m.id);
    q.exec();
    QSqlQuery t(m_db);
    t.prepare(QStringLiteral("UPDATE session SET updated_at=? WHERE id=(SELECT session_id FROM message WHERE id=?)"));
    t.addBindValue(QDateTime::currentSecsSinceEpoch());
    t.addBindValue(m.id);
    t.exec();
}

QList<Message> HistoryStore::messages(qint64 session) const
{
    QList<Message> out;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id, role, model, text_md, images, status, error FROM message WHERE session_id=? ORDER BY seq"));
    q.addBindValue(session);
    q.exec();
    while (q.next()) {
        Message m;
        m.id = q.value(0).toLongLong();
        m.role = q.value(1).toString();
        m.model = q.value(2).toString();
        m.text = q.value(3).toString();
        for (const auto &v : QJsonDocument::fromJson(q.value(4).toString().toUtf8()).array())
            m.images << v.toString();
        m.status = q.value(5).toString();
        m.error = q.value(6).toString();
        // A message left "streaming" means the app died mid-answer.
        if (m.status == QLatin1String("streaming"))
            m.status = QStringLiteral("error"), m.error = QStringLiteral("Interrupted.");
        out << m;
    }
    return out;
}

QList<SessionInfo> HistoryStore::sessions(const QString &search) const
{
    QList<SessionInfo> out;
    QSqlQuery q(m_db);
    QString sql = QStringLiteral(
        "SELECT s.id, s.title, s.updated_at, s.provider,"
        " (SELECT model FROM message WHERE session_id=s.id AND role='model' ORDER BY seq DESC LIMIT 1),"
        " (SELECT images FROM message WHERE session_id=s.id AND images != '[]' ORDER BY seq LIMIT 1),"
        " (SELECT text_md FROM message WHERE session_id=s.id AND role='model' ORDER BY seq LIMIT 1)"
        " FROM session s");
    if (!search.trimmed().isEmpty())
        sql += QStringLiteral(" WHERE s.title LIKE :s ESCAPE '\\' OR EXISTS (SELECT 1 FROM message m WHERE m.session_id=s.id AND m.text_md LIKE :s ESCAPE '\\')");
    sql += QStringLiteral(" ORDER BY s.updated_at DESC, s.id DESC");
    q.prepare(sql);
    if (!search.trimmed().isEmpty()) {
        QString esc = search.trimmed();
        esc.replace(QLatin1Char('\\'), QStringLiteral("\\\\")).replace(QLatin1Char('%'), QStringLiteral("\\%")).replace(QLatin1Char('_'), QStringLiteral("\\_"));
        q.bindValue(QStringLiteral(":s"), QString(QLatin1Char('%') + esc + QLatin1Char('%')));
    }
    q.exec();
    while (q.next()) {
        SessionInfo s;
        s.id = q.value(0).toLongLong();
        s.title = q.value(1).toString();
        s.updated = QDateTime::fromSecsSinceEpoch(q.value(2).toLongLong());
        s.provider = q.value(3).toString();
        s.model = q.value(4).toString();
        const auto imgs = QJsonDocument::fromJson(q.value(5).toString().toUtf8()).array();
        if (!imgs.isEmpty())
            s.thumbnail = imgs.first().toString();
        s.preview = q.value(6).toString().left(200).simplified();
        out << s;
    }
    return out;
}

void HistoryStore::deleteSession(qint64 session)
{
    for (const Message &m : messages(session))
        for (const QString &img : m.images)
            QFile::remove(img);
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM session WHERE id=?"));
    q.addBindValue(session);
    q.exec();
}

int HistoryStore::purgeOlderThan(int days)
{
    if (days <= 0)
        return 0;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id FROM session WHERE updated_at < ?"));
    q.addBindValue(QDateTime::currentSecsSinceEpoch() - qint64(days) * 86400);
    q.exec();
    QList<qint64> ids;
    while (q.next())
        ids << q.value(0).toLongLong();
    for (qint64 id : ids)
        deleteSession(id);
    return ids.size();
}
