#include "goglibrarymodel.h"
#include "gogclient.h"
#include "gogcovercache.h"
#include <QFileInfo>

static QString formatSize(double bytes)
{
    if (bytes < 0)
        return {};
    if (bytes >= 1024.0 * 1024 * 1024)
        return QStringLiteral("%1 GB").arg(bytes / (1024.0 * 1024 * 1024), 0, 'f', 1);
    return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024), 0, 'f', 0);
}

GogLibraryModel::GogLibraryModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void GogLibraryModel::setClient(GogClient *client)
{
    m_client = client;
    connect(client, &GogClient::libraryFetched, this, &GogLibraryModel::onLibraryFetched);
    connect(client, &GogClient::busyChanged, this, [this]() {
        setBusy(m_client->busy());
    });
    connect(client, &GogClient::statusTextChanged, this, [this]() {
        setStatusText(m_client->statusText());
    });
    connect(client, &GogClient::sizeFetched, this, &GogLibraryModel::onSizeFetched);
    connect(client, &GogClient::error, this, &GogLibraryModel::error);
}

void GogLibraryModel::setCoverCache(GogCoverCache *cache)
{
    m_coverCache = cache;
}

int GogLibraryModel::rowCount(const QModelIndex &) const
{
    return m_entries.size();
}

QVariant GogLibraryModel::data(const QModelIndex &index, int role) const
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
    case SizeTextRole:
        return formatSize(e.sizeBytes);
    }
    return {};
}

QHash<int, QByteArray> GogLibraryModel::roleNames() const
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
        {SizeTextRole, "sizeText"},
    };
}

bool GogLibraryModel::busy() const
{
    return m_busy;
}

QString GogLibraryModel::statusText() const
{
    return m_statusText;
}

void GogLibraryModel::setBusy(bool b)
{
    if (m_busy == b)
        return;
    m_busy = b;
    Q_EMIT busyChanged();
}

void GogLibraryModel::setStatusText(const QString &s)
{
    if (m_statusText == s)
        return;
    m_statusText = s;
    Q_EMIT statusTextChanged();
}

void GogLibraryModel::fetchLibrary(const QString &search, int page)
{
    if (!m_client)
        return;
    m_currentPage = page;
    m_client->fetchLibrary(search, page);
}

void GogLibraryModel::fetchNextPage(const QString &search)
{
    if (m_currentPage >= m_totalPages)
        return;
    fetchLibrary(search, m_currentPage + 1);
}

void GogLibraryModel::clear()
{
    if (m_entries.isEmpty())
        return;
    beginResetModel();
    m_entries.clear();
    endResetModel();
    m_currentPage = 1;
    m_totalPages = 1;
    Q_EMIT countChanged();
}

QVariantMap GogLibraryModel::getGame(int index) const
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

void GogLibraryModel::setInstalledMap(const QVariantMap &map)
{
    m_installed.clear();
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
        m_installed.insert(it.key(), it.value().toString());
    if (!m_entries.isEmpty())
        Q_EMIT dataChanged(index(0, 0), index(m_entries.size() - 1, 0), {InstalledRole, ExePathRole});
}

void GogLibraryModel::markInstalled(const QString &gameId, const QString &exePath)
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

bool GogLibraryModel::isInstalled(const QString &id) const
{
    const QString path = m_installed.value(id);
    return !path.isEmpty() && QFileInfo::exists(path);
}

void GogLibraryModel::revalidateInstalled()
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

void GogLibraryModel::notifyCoverCached(const QString &gameId, const QString &localPath)
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

void GogLibraryModel::onLibraryFetched(const QVariantList &items, int totalPages, int page)
{
    m_totalPages = totalPages;
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
    requestSizes();
    Q_EMIT libraryUpdated(m_currentPage < m_totalPages);
}

void GogLibraryModel::onSizeFetched(const QString &gameId, double bytes)
{
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].id == gameId) {
            m_entries[i].sizeBytes = bytes;
            auto idx = index(i, 0);
            Q_EMIT dataChanged(idx, idx, {SizeTextRole});
            return;
        }
    }
}

void GogLibraryModel::requestSizes()
{
    if (!m_client)
        return;
    // -1 = not requested, -2 = request in flight, >=0 = known.
    for (auto &e : m_entries) {
        if (e.sizeBytes == -1) {
            e.sizeBytes = -2;
            m_client->fetchGameSize(e.id, e.worksOnLinux);
        }
    }
}

void GogLibraryModel::requestCovers()
{
    if (!m_coverCache)
        return;
    for (const auto &e : std::as_const(m_entries)) {
        if (e.localCover.isEmpty() && !e.coverUrl.isEmpty())
            m_coverCache->requestCover(e.id, e.coverUrl);
    }
}
