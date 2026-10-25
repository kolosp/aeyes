#include "sessionmodel.h"

#include <QLocale>
#include <QUrl>

SessionModel::SessionModel(HistoryStore *store, QObject *parent)
    : QAbstractListModel(parent)
    , m_store(store)
{
    refresh();
}

int SessionModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant SessionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};
    const SessionInfo &s = m_items.at(index.row());
    switch (role) {
    case IdRole:
        return s.id;
    case TitleRole:
        return s.title.isEmpty() ? tr("Untitled") : s.title;
    case UpdatedRole:
        return QLocale().toString(s.updated, QLocale::ShortFormat);
    case ModelRole:
        return s.model;
    case ThumbRole:
        return s.thumbnail.isEmpty() ? QUrl() : QUrl::fromLocalFile(s.thumbnail);
    case PreviewRole:
        return s.preview;
    }
    return {};
}

QHash<int, QByteArray> SessionModel::roleNames() const
{
    return {{IdRole, "sessionId"}, {TitleRole, "title"}, {UpdatedRole, "updated"}, {ModelRole, "modelName"}, {ThumbRole, "thumb"}, {PreviewRole, "preview"}};
}

void SessionModel::setSearch(const QString &s)
{
    if (s == m_search)
        return;
    m_search = s;
    Q_EMIT searchChanged();
    refresh();
}

void SessionModel::refresh()
{
    beginResetModel();
    m_items = m_store->sessions(m_search);
    endResetModel();
}

void SessionModel::remove(qint64 id)
{
    m_store->deleteSession(id);
    refresh();
}
