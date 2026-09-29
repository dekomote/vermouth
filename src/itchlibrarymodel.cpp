#include "itchlibrarymodel.h"
#include "itchclient.h"
#include "itchcovercache.h"
#include <QFileInfo>

ItchLibraryModel::ItchLibraryModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void ItchLibraryModel::setClient(ItchClient *client)
{
    m_client = client;
    connect(client, &ItchClient::libraryFetched, this, &ItchLibraryModel::onLibraryFetched);
    connect(client, &ItchClient::busyChanged, this, [this]() {
        setBusy(m_client->busy());
    });
    connect(client, &ItchClient::statusTextChanged, this, [this]() {
        setStatusText(m_client->statusText());
    });
    connect(client, &ItchClient::error, this, &ItchLibraryModel::error);
}

void ItchLibraryModel::setCoverCache(ItchCoverCache *cache)
{
    m_coverCache = cache;
}

int ItchLibraryModel::rowCount(const QModelIndex &) const
{
    return m_entries.size();
}

QVariant ItchLibraryModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const auto &e = m_entries[index.row()];
    switch (role) {
    case GameIdRole:
        return e.id;
    case TitleRole:
        return e.title;
    case CoverUrlRole:
        return e.coverUrl;
    case LocalCoverRole:
        return e.localCover;
    case WorksOnWindowsRole:
        return e.worksOnWindows;
    case WorksOnLinuxRole:
        return e.worksOnLinux;
    case InstalledRole:
        return isInstalled(e.id);
    case ExePathRole:
        return m_installed.value(e.id);
    }
    return {};
}

QHash<int, QByteArray> ItchLibraryModel::roleNames() const
{
    return {
        {GameIdRole, "gameId"},
        {TitleRole, "title"},
        {CoverUrlRole, "coverUrl"},
        {LocalCoverRole, "localCover"},
        {WorksOnWindowsRole, "worksOnWindows"},
        {WorksOnLinuxRole, "worksOnLinux"},
        {InstalledRole, "installed"},
        {ExePathRole, "exePath"},
    };
}

bool ItchLibraryModel::busy() const
{
    return m_busy;
}

QString ItchLibraryModel::statusText() const
{
    return m_statusText;
}

void ItchLibraryModel::setBusy(bool b)
{
    if (m_busy == b)
        return;
    m_busy = b;
    Q_EMIT busyChanged();
}

void ItchLibraryModel::setStatusText(const QString &s)
{
    if (m_statusText == s)
        return;
    m_statusText = s;
    Q_EMIT statusTextChanged();
}

void ItchLibraryModel::fetchLibrary(const QString &search, int page)
{
    if (!m_client)
        return;
    m_client->fetchLibrary(search, page);
}

void ItchLibraryModel::fetchNextPage(const QString &search)
{
    if (!m_hasMore)
        return;
    fetchLibrary(search, m_currentPage + 1);
}

void ItchLibraryModel::clear()
{
    if (m_entries.isEmpty())
        return;
    beginResetModel();
    m_entries.clear();
    endResetModel();
    m_currentPage = 1;
    m_hasMore = false;
    Q_EMIT countChanged();
}

QVariantMap ItchLibraryModel::getGame(int index) const
{
    if (index < 0 || index >= m_entries.size())
        return {};
    const auto &e = m_entries[index];
    QVariantMap m;
    m[QStringLiteral("gameId")] = e.id;
    m[QStringLiteral("title")] = e.title;
    m[QStringLiteral("coverUrl")] = e.coverUrl;
    m[QStringLiteral("localCover")] = e.localCover;
    m[QStringLiteral("worksOnWindows")] = e.worksOnWindows;
    m[QStringLiteral("worksOnLinux")] = e.worksOnLinux;
    m[QStringLiteral("installed")] = isInstalled(e.id);
    m[QStringLiteral("exePath")] = m_installed.value(e.id);
    return m;
}

QVariantMap ItchLibraryModel::getGameById(const QString &gameId) const
{
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].id == gameId)
            return getGame(i);
    }
    return {};
}

void ItchLibraryModel::setInstalledMap(const QVariantMap &map)
{
    m_installed.clear();
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
        m_installed.insert(it.key(), it.value().toString());
    if (!m_entries.isEmpty())
        Q_EMIT dataChanged(index(0, 0), index(m_entries.size() - 1, 0), {InstalledRole, ExePathRole});
}

void ItchLibraryModel::markInstalled(const QString &gameId, const QString &exePath)
{
    m_installed.insert(gameId, exePath);
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].id == gameId) {
            auto idx = index(i, 0);
            Q_EMIT dataChanged(idx, idx, {InstalledRole, ExePathRole});
            return;
        }
    }
}

bool ItchLibraryModel::isInstalled(const QString &id) const
{
    const QString path = m_installed.value(id);
    return !path.isEmpty() && QFileInfo::exists(path);
}

void ItchLibraryModel::revalidateInstalled()
{
    QStringList stale;
    for (auto it = m_installed.constBegin(); it != m_installed.constEnd(); ++it) {
        if (it.value().isEmpty() || !QFileInfo::exists(it.value()))
            stale << it.key();
    }
    if (stale.isEmpty())
        return;
    for (const QString &id : std::as_const(stale)) {
        m_installed.remove(id);
        Q_EMIT installedRemoved(id);
    }
    for (int i = 0; i < m_entries.size(); ++i) {
        if (stale.contains(m_entries[i].id)) {
            auto idx = index(i, 0);
            Q_EMIT dataChanged(idx, idx, {InstalledRole, ExePathRole});
        }
    }
}

void ItchLibraryModel::notifyCoverCached(const QString &gameId, const QString &localPath)
{
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].id == gameId) {
            m_entries[i].localCover = localPath;
            auto idx = index(i, 0);
            Q_EMIT dataChanged(idx, idx, {LocalCoverRole});
            return;
        }
    }
}

void ItchLibraryModel::onLibraryFetched(const QVariantList &items, bool hasMore, int page)
{
    m_hasMore = hasMore;
    m_currentPage = page;

    auto buildEntry = [this](const QVariant &var) {
        auto map = var.toMap();
        GameEntry e;
        e.id = map[QStringLiteral("gameId")].toString();
        e.title = map[QStringLiteral("title")].toString();
        e.coverUrl = map[QStringLiteral("coverUrl")].toString();
        e.worksOnWindows = map[QStringLiteral("worksOnWindows")].toBool();
        e.worksOnLinux = map[QStringLiteral("worksOnLinux")].toBool();
        if (m_coverCache)
            e.localCover = m_coverCache->cachedPath(e.id);
        return e;
    };

    if (page <= 1) {
        beginResetModel();
        m_entries.clear();
        for (const auto &var : items)
            m_entries.append(buildEntry(var));
        endResetModel();
    } else {
        int first = m_entries.size();
        int last = first + items.size() - 1;
        if (last < first) {
            Q_EMIT libraryUpdated(false);
            return;
        }
        beginInsertRows({}, first, last);
        for (const auto &var : items)
            m_entries.append(buildEntry(var));
        endInsertRows();
    }
    Q_EMIT countChanged();
    requestCovers();
    Q_EMIT libraryUpdated(m_hasMore);
}

void ItchLibraryModel::requestCovers()
{
    if (!m_coverCache)
        return;
    for (const auto &e : std::as_const(m_entries)) {
        if (e.localCover.isEmpty() && !e.coverUrl.isEmpty())
            m_coverCache->requestCover(e.id, e.coverUrl);
    }
}
