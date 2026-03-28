#include "appmodel.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

AppModel::AppModel(QObject *parent)
    : QAbstractListModel(parent)
{
    load();
}

int AppModel::sourceIndex(int filteredIndex) const {
    if (filteredIndex < 0 || filteredIndex >= m_filtered.size())
        return -1;
    return m_filtered[filteredIndex];
}

void AppModel::rebuildFilter() {
    m_filtered.clear();
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_filter.isEmpty() ||
            m_entries[i].name.contains(m_filter, Qt::CaseInsensitive))
            m_filtered.append(i);
    }
}

void AppModel::setFilterString(const QString &filter) {
    if (m_filter == filter)
        return;
    beginResetModel();
    m_filter = filter;
    rebuildFilter();
    endResetModel();
    Q_EMIT countChanged();
}

int AppModel::rowCount(const QModelIndex &) const {
    return m_filtered.size();
}

QVariant AppModel::data(const QModelIndex &index, int role) const {
    int src = sourceIndex(index.row());
    if (src < 0)
        return {};

    const auto &e = m_entries[src];
    switch (role) {
    case NameRole:          return e.name;
    case ExePathRole:       return e.exePath;
    case RuntimeTypeRole:   return (e.runtimeType == AppEntry::Proton) ? QStringLiteral("proton") : QStringLiteral("wine");
    case ProtonPathRole:    return e.protonPath;
    case ProtonPrefixRole:  return e.protonPrefix;
    case WineBinaryRole:    return e.wineBinary;
    case WinePrefixRole:    return e.winePrefix;
    case IconPathRole:      return e.iconPath;
    case LaunchOptionsRole: return e.launchOptions;
    case EnableLoggingRole: return e.enableLogging;
    }
    return {};
}

QHash<int, QByteArray> AppModel::roleNames() const {
    return {
        {NameRole,          "name"},
        {ExePathRole,       "exePath"},
        {RuntimeTypeRole,   "runtimeType"},
        {ProtonPathRole,    "protonPath"},
        {ProtonPrefixRole,  "protonPrefix"},
        {WineBinaryRole,    "wineBinary"},
        {WinePrefixRole,    "winePrefix"},
        {IconPathRole,      "iconPath"},
        {LaunchOptionsRole, "launchOptions"},
        {EnableLoggingRole, "enableLogging"},
    };
}

void AppModel::addApp(const QString &name, const QString &exePath,
                      const QString &runtimeType,
                      const QString &protonPath, const QString &protonPrefix,
                      const QString &wineBinary, const QString &winePrefix,
                      const QString &iconPath,
                      const QString &launchOptions,
                      bool enableLogging) {
    AppEntry e;
    e.name = name;
    e.exePath = exePath;
    e.runtimeType = (runtimeType == QStringLiteral("proton")) ? AppEntry::Proton : AppEntry::Wine;
    e.protonPath = protonPath;
    e.protonPrefix = protonPrefix;
    e.wineBinary = wineBinary;
    e.winePrefix = winePrefix;
    e.iconPath = iconPath;
    e.launchOptions = launchOptions;
    e.enableLogging = enableLogging;

    beginResetModel();
    m_entries.append(e);
    rebuildFilter();
    endResetModel();
    Q_EMIT countChanged();
    save();
}

void AppModel::removeApp(int index) {
    int src = sourceIndex(index);
    if (src < 0) return;

    beginResetModel();
    m_entries.removeAt(src);
    rebuildFilter();
    endResetModel();
    Q_EMIT countChanged();
    save();
}

void AppModel::removeAndCleanApp(int index) {
    int src = sourceIndex(index);
    if (src < 0) return;

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

void AppModel::editApp(int index,
                       const QString &name, const QString &exePath,
                       const QString &runtimeType,
                       const QString &protonPath, const QString &protonPrefix,
                       const QString &wineBinary, const QString &winePrefix,
                       const QString &iconPath,
                       const QString &launchOptions,
                       bool enableLogging) {
    int src = sourceIndex(index);
    if (src < 0) return;

    auto &e = m_entries[src];
    e.name = name;
    e.exePath = exePath;
    e.runtimeType = (runtimeType == QStringLiteral("proton")) ? AppEntry::Proton : AppEntry::Wine;
    e.protonPath = protonPath;
    e.protonPrefix = protonPrefix;
    e.wineBinary = wineBinary;
    e.winePrefix = winePrefix;
    e.iconPath = iconPath;
    e.launchOptions = launchOptions;
    e.enableLogging = enableLogging;

    beginResetModel();
    rebuildFilter();
    endResetModel();
    Q_EMIT countChanged();
    save();
}

QVariantMap AppModel::getApp(int index) const {
    int src = sourceIndex(index);
    if (src < 0) return {};

    const auto &e = m_entries[src];
    return {
        {QStringLiteral("name"), e.name},
        {QStringLiteral("exePath"), e.exePath},
        {QStringLiteral("runtimeType"), (e.runtimeType == AppEntry::Proton) ? QStringLiteral("proton") : QStringLiteral("wine")},
        {QStringLiteral("protonPath"), e.protonPath},
        {QStringLiteral("protonPrefix"), e.protonPrefix},
        {QStringLiteral("wineBinary"), e.wineBinary},
        {QStringLiteral("winePrefix"), e.winePrefix},
        {QStringLiteral("iconPath"), e.iconPath},
        {QStringLiteral("launchOptions"), e.launchOptions},
        {QStringLiteral("enableLogging"), e.enableLogging},
    };
}

QString AppModel::configPath() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/apps.json");
}

void AppModel::load() {
    QFile f(configPath());
    if (!f.open(QIODevice::ReadOnly)) return;
    auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray()) return;

    beginResetModel();
    m_entries.clear();
    for (const auto &val : doc.array())
        m_entries.append(AppEntry::fromJson(val.toObject()));
    rebuildFilter();
    endResetModel();
    Q_EMIT countChanged();
}

void AppModel::save() const {
    QJsonArray arr;
    for (const auto &e : m_entries)
        arr.append(e.toJson());

    QFile f(configPath());
    if (!f.open(QIODevice::WriteOnly)) return;
    f.write(QJsonDocument(arr).toJson());
}
