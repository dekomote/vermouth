#include "appmodel.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QUuid>

AppModel::AppModel(QObject *parent)
    : QAbstractListModel(parent)
{
    load();
}

int AppModel::sourceIndex(int filteredIndex) const
{
    if (filteredIndex < 0 || filteredIndex >= m_filtered.size())
        return -1;
    return m_filtered[filteredIndex];
}

void AppModel::rebuildFilter()
{
    m_filtered.clear();
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_filter.isEmpty() || m_entries[i].name.contains(m_filter, Qt::CaseInsensitive))
            m_filtered.append(i);
    }
    std::sort(m_filtered.begin(), m_filtered.end(), [this](int a, int b) {
        return m_entries[a].name.compare(m_entries[b].name, Qt::CaseInsensitive) < 0;
    });
}

void AppModel::setFilterString(const QString &filter)
{
    if (m_filter == filter)
        return;
    beginResetModel();
    m_filter = filter;
    rebuildFilter();
    endResetModel();
    Q_EMIT countChanged();
}

int AppModel::rowCount(const QModelIndex &) const
{
    return m_filtered.size();
}

QVariant AppModel::data(const QModelIndex &index, int role) const
{
    int src = sourceIndex(index.row());
    if (src < 0)
        return {};

    const auto &e = m_entries[src];
    switch (role) {
    case IdRole:
        return e.id;
    case NameRole:
        return e.name;
    case ExePathRole:
        return e.exePath;
    case RuntimeTypeRole:
        return QString::fromLatin1(AppEntry::runtimeTypeString(e.runtimeType));
    case ProtonPathRole:
        return e.protonPath;
    case ProtonPrefixRole:
        return e.protonPrefix;
    case WineBinaryRole:
        return e.wineBinary;
    case WinePrefixRole:
        return e.winePrefix;
    case IconPathRole:
        return e.iconPath;
    case GridPathRole:
        return e.gridPath;
    case HeroPathRole:
        return e.heroPath;
    case LogoPathRole:
        return e.logoPath;
    case SteamGridDbIdRole:
        return e.steamGridDbId;
    case SteamAppIdRole:
        return e.steamAppId;
    case PlatformSlugRole:
        return e.platformSlug;
    case CustomCorePathRole:
        return e.customCorePath;
    case LaunchOptionsRole:
        return e.launchOptions;
    case EnableLoggingRole:
        return e.enableLogging;
    }
    return {};
}

QHash<int, QByteArray> AppModel::roleNames() const
{
    return {
        {IdRole, "appId"},
        {NameRole, "name"},
        {ExePathRole, "exePath"},
        {RuntimeTypeRole, "runtimeType"},
        {ProtonPathRole, "protonPath"},
        {ProtonPrefixRole, "protonPrefix"},
        {WineBinaryRole, "wineBinary"},
        {WinePrefixRole, "winePrefix"},
        {IconPathRole, "iconPath"},
        {GridPathRole, "gridPath"},
        {HeroPathRole, "heroPath"},
        {LogoPathRole, "logoPath"},
        {SteamGridDbIdRole, "steamGridDbId"},
        {SteamAppIdRole, "steamAppId"},
        {PlatformSlugRole, "platformSlug"},
        {CustomCorePathRole, "customCorePath"},
        {LaunchOptionsRole, "launchOptions"},
        {EnableLoggingRole, "enableLogging"},
    };
}

void AppModel::addApp(const QVariantMap &app)
{
    AppEntry e;
    e.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    e.updateFromVariantMap(app);

    beginResetModel();
    m_entries.append(e);
    rebuildFilter();
    endResetModel();
    Q_EMIT countChanged();
    save();
}

void AppModel::removeApp(int index)
{
    int src = sourceIndex(index);
    if (src < 0)
        return;

    beginResetModel();
    m_entries.removeAt(src);
    rebuildFilter();
    endResetModel();
    Q_EMIT countChanged();
    save();
}

void AppModel::removeAndCleanApp(int index)
{
    int src = sourceIndex(index);
    if (src < 0)
        return;

    if (!m_entries[src].winePrefix.isEmpty()) {
        if (QDir(m_entries[src].winePrefix).exists()) {
            QDir(m_entries[src].winePrefix).removeRecursively();
        }
    }

    if (!m_entries[src].protonPrefix.isEmpty()) {
        if (QDir(m_entries[src].protonPrefix).exists()) {
            QDir(m_entries[src].protonPrefix).removeRecursively();
        }
    }

    AppModel::removeApp(index);
}

void AppModel::editApp(int index, const QVariantMap &app)
{
    int src = sourceIndex(index);
    if (src < 0)
        return;

    auto &e = m_entries[src];
    e.updateFromVariantMap(app);

    beginResetModel();
    rebuildFilter();
    endResetModel();
    Q_EMIT countChanged();
    save();
}

void AppModel::updateAppArt(const QString &id,
                            const QString &iconPath,
                            const QString &gridPath,
                            const QString &heroPath,
                            const QString &logoPath,
                            int steamGridDbId)
{
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].id == id) {
            if (!iconPath.isEmpty())
                m_entries[i].iconPath = iconPath;
            if (!gridPath.isEmpty())
                m_entries[i].gridPath = gridPath;
            if (!heroPath.isEmpty())
                m_entries[i].heroPath = heroPath;
            if (!logoPath.isEmpty())
                m_entries[i].logoPath = logoPath;
            if (steamGridDbId > 0)
                m_entries[i].steamGridDbId = steamGridDbId;
            beginResetModel();
            rebuildFilter();
            endResetModel();
            save();
            return;
        }
    }
}

QVariantMap AppModel::getApp(int index) const
{
    int src = sourceIndex(index);
    if (src < 0)
        return {};

    const auto &e = m_entries[src];
    return e.toVariantMap();
}

QString AppModel::configPath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/apps.json");
}

QVariantMap AppModel::getAppById(const QString &id) const
{
    for (const auto &e : m_entries) {
        if (e.id == id) {
            return e.toVariantMap();
        }
    }
    return {};
}

QVariantMap AppModel::getAppByExePath(const QString &exePath) const
{
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].exePath == exePath) {
            return m_entries[i].toVariantMap();
        }
    }
    return {};
}

QString AppModel::generateUUID() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool AppModel::hasSteamApp(int appId) const
{
    for (const auto &e : m_entries) {
        if (e.steamAppId == appId && e.steamAppId > 0)
            return true;
    }
    return false;
}

void AppModel::load()
{
    QFile f(configPath());
    if (!f.open(QIODevice::ReadOnly))
        return;
    auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray())
        return;

    bool needsMigration = false;
    beginResetModel();
    m_entries.clear();
    for (const auto &val : doc.array()) {
        auto obj = val.toObject();
        if (!obj.contains(QStringLiteral("id")))
            needsMigration = true;
        m_entries.append(AppEntry::fromJson(obj));
    }
    rebuildFilter();
    endResetModel();
    Q_EMIT countChanged();

    if (needsMigration)
        save();
}

void AppModel::save() const
{
    QJsonArray arr;
    for (const auto &e : m_entries)
        arr.append(e.toJson());

    QFile f(configPath());
    if (!f.open(QIODevice::WriteOnly))
        return;
    f.write(QJsonDocument(arr).toJson());
}
