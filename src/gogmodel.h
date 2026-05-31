#pragma once

#include "gogentry.h"
#include <QAbstractListModel>
#include <QVector>

class GogModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    enum Roles {
        GameIdRole = Qt::UserRole + 1,
        NameRole,
        ExePathRole,
        IconPathRole,
        IsWindowsRole,
    };

    explicit GogModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool busy() const;

    Q_INVOKABLE void scanFolder(const QString &path);
    Q_INVOKABLE QVariantMap getGame(int index) const;

Q_SIGNALS:
    void countChanged();
    void busyChanged();
    void gamesFetched();

private:
    QVector<GogEntry> m_entries;
    bool m_busy = false;
};
