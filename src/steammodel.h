#pragma once

#include "steamlibrary.h"
#include <QAbstractListModel>
#include <QStringList>
#include <QVector>

class SteamModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    enum Roles {
        SteamIdRole = Qt::UserRole + 1,
        NameRole,
        GridPathRole,
        HeroPathRole,
        IconPathRole,
        LogoPathRole,
        InstallDirRole,
    };

    explicit SteamModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool busy() const;
    QString statusText() const;

    Q_INVOKABLE void scanLibraries();
    Q_INVOKABLE QVariantMap getGame(int index) const;

Q_SIGNALS:
    void countChanged();
    void busyChanged();
    void statusTextChanged();
    void gamesFetched();

private:
    void setBusy(bool b);
    void setStatusText(const QString &s);

    QVector<SteamEntry> m_entries;
    bool m_busy = false;
    QString m_statusText;
};
