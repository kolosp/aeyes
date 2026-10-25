#pragma once
#include "historystore.h"

#include <QAbstractListModel>

class SessionModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY searchChanged)
public:
    enum Roles { IdRole = Qt::UserRole + 1, TitleRole, UpdatedRole, ModelRole, ThumbRole, PreviewRole };
    explicit SessionModel(HistoryStore *store, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString search() const { return m_search; }
    void setSearch(const QString &s);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void remove(qint64 id);

Q_SIGNALS:
    void searchChanged();

private:
    HistoryStore *m_store;
    QString m_search;
    QList<SessionInfo> m_items;
};
