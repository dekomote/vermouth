#include "settingsmanager.h"

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
{
}

QString SettingsManager::defaultPrefixDir() const {
    return m_settings.value(QStringLiteral("defaultPrefixDir")).toString();
}

void SettingsManager::setDefaultPrefixDir(const QString &path) {
    if (defaultPrefixDir() == path)
        return;
    m_settings.setValue(QStringLiteral("defaultPrefixDir"), path);
    Q_EMIT defaultPrefixDirChanged();
}


QStringList SettingsManager::extraProtonPaths() const {
    return m_settings.value(QStringLiteral("extraProtonPaths")).toStringList();
}

void SettingsManager::setExtraProtonPaths(const QStringList &paths) {
    m_settings.setValue(QStringLiteral("extraProtonPaths"), paths);
    Q_EMIT extraProtonPathsChanged();
}

void SettingsManager::addExtraProtonPath(const QString &path) {
    auto paths = extraProtonPaths();
    if (!paths.contains(path)) {
        paths << path;
        setExtraProtonPaths(paths);
    }
}

void SettingsManager::removeExtraProtonPath(int index) {
    auto paths = extraProtonPaths();
    if (index >= 0 && index < paths.size()) {
        paths.removeAt(index);
        setExtraProtonPaths(paths);
    }
}
