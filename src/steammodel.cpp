#include "steammodel.h"
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>

SteamModel::SteamModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int SteamModel::rowCount(const QModelIndex &) const
{
    return m_entries.size();
}

QVariant SteamModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const auto &e = m_entries[index.row()];
    switch (role) {
    case SteamIdRole:
        return e.appId;
    case NameRole:
        return e.name;
    case GridPathRole:
        return e.gridPath;
    case HeroPathRole:
        return e.heroPath;
    case IconPathRole:
        return e.iconPath;
    case LogoPathRole:
        return e.logoPath;
    case InstallDirRole:
        return e.installDir;
    }
    return {};
}

QHash<int, QByteArray> SteamModel::roleNames() const
{
    return {
        {SteamIdRole, "steamId"},
        {NameRole, "name"},
        {GridPathRole, "gridPath"},
        {HeroPathRole, "heroPath"},
        {IconPathRole, "iconPath"},
        {LogoPathRole, "logoPath"},
        {InstallDirRole, "installDir"},
    };
}

bool SteamModel::busy() const
{
    return m_busy;
}

QString SteamModel::statusText() const
{
    return m_statusText;
}

void SteamModel::setBusy(bool b)
{
    if (m_busy == b)
        return;
    m_busy = b;
    Q_EMIT busyChanged();
}

void SteamModel::setStatusText(const QString &s)
{
    if (m_statusText == s)
        return;
    m_statusText = s;
    Q_EMIT statusTextChanged();
}

void SteamModel::scanLibraries()
{
    if (m_busy)
        return;

    setBusy(true);
    setStatusText(QStringLiteral("Scanning Steam library..."));

    auto *watcher = new QFutureWatcher<QVector<SteamEntry>>(this);
    connect(watcher, &QFutureWatcher<QVector<SteamEntry>>::finished, this, [this, watcher] {
        beginResetModel();
        m_entries = watcher->result();
        endResetModel();
        watcher->deleteLater();
        setBusy(false);
        setStatusText({});
        Q_EMIT countChanged();
        Q_EMIT gamesFetched();
    });
    watcher->setFuture(QtConcurrent::run(&SteamLibrary::scan));
}

QVariantMap SteamModel::getGame(int index) const
{
    if (index < 0 || index >= m_entries.size())
        return {};
    const auto &e = m_entries[index];
    QVariantMap m;
    m[QStringLiteral("steamId")] = e.appId;
    m[QStringLiteral("name")] = e.name;
    m[QStringLiteral("gridPath")] = e.gridPath;
    m[QStringLiteral("heroPath")] = e.heroPath;
    m[QStringLiteral("iconPath")] = e.iconPath;
    m[QStringLiteral("logoPath")] = e.logoPath;
    m[QStringLiteral("installDir")] = e.installDir;
    return m;
}
