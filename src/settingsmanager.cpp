#include "settingsmanager.h"

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
{
}

QString SettingsManager::defaultPrefixDir() const
{
    return m_settings.value(QStringLiteral("defaultPrefixDir")).toString();
}

void SettingsManager::setDefaultPrefixDir(const QString &path)
{
    if (defaultPrefixDir() == path)
        return;
    m_settings.setValue(QStringLiteral("defaultPrefixDir"), path);
    Q_EMIT defaultPrefixDirChanged();
}

QStringList SettingsManager::extraProtonPaths() const
{
    return m_settings.value(QStringLiteral("extraProtonPaths")).toStringList();
}

void SettingsManager::setExtraProtonPaths(const QStringList &paths)
{
    m_settings.setValue(QStringLiteral("extraProtonPaths"), paths);
    Q_EMIT extraProtonPathsChanged();
}

void SettingsManager::addExtraProtonPath(const QString &path)
{
    auto paths = extraProtonPaths();
    if (!paths.contains(path)) {
        paths << path;
        setExtraProtonPaths(paths);
    }
}

void SettingsManager::removeExtraProtonPath(int index)
{
    auto paths = extraProtonPaths();
    if (index >= 0 && index < paths.size()) {
        paths.removeAt(index);
        setExtraProtonPaths(paths);
    }
}

QString SettingsManager::defaultRuntimeType() const
{
    return m_settings.value(QStringLiteral("defaultRuntimeType")).toString();
}

void SettingsManager::setDefaultRuntimeType(const QString &type)
{
    if (defaultRuntimeType() == type)
        return;
    m_settings.setValue(QStringLiteral("defaultRuntimeType"), type);
    Q_EMIT defaultRuntimeChanged();
}

QString SettingsManager::defaultProtonPath() const
{
    return m_settings.value(QStringLiteral("defaultProtonPath")).toString();
}

void SettingsManager::setDefaultProtonPath(const QString &path)
{
    if (defaultProtonPath() == path)
        return;
    m_settings.setValue(QStringLiteral("defaultProtonPath"), path);
    Q_EMIT defaultRuntimeChanged();
}

QString SettingsManager::defaultWineBinary() const
{
    return m_settings.value(QStringLiteral("defaultWineBinary")).toString();
}

void SettingsManager::setDefaultWineBinary(const QString &path)
{
    if (defaultWineBinary() == path)
        return;
    m_settings.setValue(QStringLiteral("defaultWineBinary"), path);
    Q_EMIT defaultRuntimeChanged();
}

bool SettingsManager::drawerPinned() const
{
    return m_settings.value(QStringLiteral("drawerPinned"), false).toBool();
}

void SettingsManager::setDrawerPinned(bool pinned)
{
    if (drawerPinned() == pinned)
        return;
    m_settings.setValue(QStringLiteral("drawerPinned"), pinned);
    Q_EMIT drawerPinnedChanged();
}
