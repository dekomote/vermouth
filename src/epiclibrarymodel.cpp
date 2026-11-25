#include "epiclibrarymodel.h"
#include "epicclient.h"
#include "gogcovercache.h"
#include <algorithm>

EpicLibraryModel::EpicLibraryModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void EpicLibraryModel::setClient(EpicClient *client)
{
    m_client = client;
    connect(client, &EpicClient::libraryFetched, this, &EpicLibraryModel::onLibraryFetched);
    connect(client, &EpicClient::updatesChecked, this, &EpicLibraryModel::setOutdated);
}

void EpicLibraryModel::setCoverCache(GogCoverCache *cache)
{
    m_coverCache = cache;
}

int EpicLibraryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_view.size();
}

int EpicLibraryModel::totalCount() const
{
    return m_all.size();
}

QHash<int, QByteArray> EpicLibraryModel::roleNames() const
{
    return {
        {AppNameRole, "appName"},
        {TitleRole, "title"},
        {CoverUrlRole, "coverUrl"},
        {LocalCoverRole, "localCover"},
        {InstalledRole, "installed"},
        {InstallPathRole, "installPath"},
        {ExePathRole, "exePath"},
        {UpdateAvailableRole, "updateAvailable"},
    };
}

QVariant EpicLibraryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_view.size())
        return {};
    const Game &g = m_all.at(m_view.at(index.row()));
    switch (role) {
    case AppNameRole:
        return g.appName;
    case TitleRole:
        return g.title;
    case CoverUrlRole:
        return g.coverUrl;
    case LocalCoverRole:
        return g.localCover;
    case InstalledRole:
        return !g.installPath.isEmpty();
    case InstallPathRole:
        return g.installPath;
    case ExePathRole:
        return g.exePath;
    case UpdateAvailableRole:
        return g.outdated && !g.installPath.isEmpty();
    }
    return {};
}

void EpicLibraryModel::refresh()
{
    if (m_client)
        m_client->fetchLibrary();
}

void EpicLibraryModel::setFilter(const QString &text)
{
    if (m_filter == text)
        return;
    m_filter = text;
    rebuild();
}

void EpicLibraryModel::clear()
{
    beginResetModel();
    m_all.clear();
    m_view.clear();
    endResetModel();
    Q_EMIT countChanged();
}

void EpicLibraryModel::rebuild()
{
    beginResetModel();
    m_view.clear();
    for (int i = 0; i < m_all.size(); ++i) {
        if (m_filter.isEmpty() || m_all.at(i).title.contains(m_filter, Qt::CaseInsensitive))
            m_view.append(i);
    }
    endResetModel();
    Q_EMIT countChanged();
}

void EpicLibraryModel::onLibraryFetched(const QVariantList &games, const QVariantList &installed)
{
    QHash<QString, QString> paths;
    QHash<QString, QString> exes;
    for (const QVariant &v : installed) {
        const QVariantMap m = v.toMap();
        const QString app = m.value(QStringLiteral("appName")).toString();
        const QString dir = m.value(QStringLiteral("installPath")).toString();
        const QString exe = m.value(QStringLiteral("executable")).toString();
        paths.insert(app, dir);
        exes.insert(app, exe.isEmpty() ? QString() : dir + QLatin1Char('/') + exe);
    }

    QVector<Game> all;
    for (const QVariant &v : games) {
        const QVariantMap m = v.toMap();
        Game g;
        g.appName = m.value(QStringLiteral("appName")).toString();
        g.title = m.value(QStringLiteral("title")).toString();
        g.coverUrl = m.value(QStringLiteral("coverUrl")).toString();
        g.installPath = paths.value(g.appName);
        g.exePath = exes.value(g.appName);
        if (g.appName.isEmpty() || g.title.isEmpty())
            continue;
        if (m_coverCache)
            g.localCover = m_coverCache->cachedPath(g.appName);
        all.append(g);
    }
    std::sort(all.begin(), all.end(), [](const Game &a, const Game &b) {
        return a.title.compare(b.title, Qt::CaseInsensitive) < 0;
    });
    m_all = all;
    rebuild();

    if (m_coverCache) {
        for (const Game &g : std::as_const(m_all)) {
            if (g.localCover.isEmpty())
                m_coverCache->requestCover(g.appName, g.coverUrl);
        }
    }
    Q_EMIT libraryUpdated();
}

int EpicLibraryModel::viewRow(const QString &appName) const
{
    for (int row = 0; row < m_view.size(); ++row) {
        if (m_all.at(m_view.at(row)).appName == appName)
            return row;
    }
    return -1;
}

QVariantMap EpicLibraryModel::getGame(int index) const
{
    if (index < 0 || index >= m_view.size())
        return {};
    const Game &g = m_all.at(m_view.at(index));
    return {{QStringLiteral("appName"), g.appName},
            {QStringLiteral("title"), g.title},
            {QStringLiteral("installed"), !g.installPath.isEmpty()},
            {QStringLiteral("installPath"), g.installPath},
            {QStringLiteral("exePath"), g.exePath},
            {QStringLiteral("updateAvailable"), g.outdated && !g.installPath.isEmpty()},
            {QStringLiteral("coverPath"), g.localCover}};
}

void EpicLibraryModel::markInstalled(const QString &appName, const QString &installPath)
{
    for (Game &g : m_all) {
        if (g.appName == appName)
            g.installPath = installPath;
    }
    const int row = viewRow(appName);
    if (row >= 0)
        Q_EMIT dataChanged(index(row), index(row), {InstalledRole, InstallPathRole});
}

void EpicLibraryModel::markUninstalled(const QString &appName)
{
    for (Game &g : m_all) {
        if (g.appName == appName)
            g.installPath.clear();
    }
    const int row = viewRow(appName);
    if (row >= 0)
        Q_EMIT dataChanged(index(row), index(row), {InstalledRole, InstallPathRole});
}

void EpicLibraryModel::setOutdated(const QStringList &appNames)
{
    for (int row = 0; row < m_view.size(); ++row) {
        Game &g = m_all[m_view.at(row)];
        const bool outdated = appNames.contains(g.appName);
        if (g.outdated != outdated) {
            g.outdated = outdated;
            Q_EMIT dataChanged(index(row), index(row), {UpdateAvailableRole});
        }
    }
    for (int i = 0; i < m_all.size(); ++i)
        m_all[i].outdated = appNames.contains(m_all[i].appName);
}

void EpicLibraryModel::notifyCoverCached(const QString &appName, const QString &localPath)
{
    for (Game &g : m_all) {
        if (g.appName == appName)
            g.localCover = localPath;
    }
    const int row = viewRow(appName);
    if (row >= 0)
        Q_EMIT dataChanged(index(row), index(row), {LocalCoverRole});
}
