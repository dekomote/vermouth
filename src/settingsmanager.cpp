#include "settingsmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

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

QString SettingsManager::defaultGamePrefix() const
{
    return m_settings.value(QStringLiteral("defaultGamePrefix")).toString();
}

void SettingsManager::setDefaultGamePrefix(const QString &path)
{
    if (defaultGamePrefix() == path)
        return;
    m_settings.setValue(QStringLiteral("defaultGamePrefix"), path);
    Q_EMIT defaultGamePrefixChanged();
}

QString SettingsManager::defaultWinePrefix() const
{
    return m_settings.value(QStringLiteral("defaultWinePrefix")).toString();
}

void SettingsManager::setDefaultWinePrefix(const QString &path)
{
    if (defaultWinePrefix() == path)
        return;
    m_settings.setValue(QStringLiteral("defaultWinePrefix"), path);
    Q_EMIT defaultWinePrefixChanged();
}

QString SettingsManager::gamepadFullscreenButton() const
{
    return m_settings.value(QStringLiteral("gamepadFullscreenButton"), QStringLiteral("guide")).toString();
}

void SettingsManager::setGamepadFullscreenButton(const QString &button)
{
    if (gamepadFullscreenButton() == button)
        return;
    m_settings.setValue(QStringLiteral("gamepadFullscreenButton"), button);
    Q_EMIT gamepadFullscreenButtonChanged();
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

QString SettingsManager::umuPath() const
{
    return m_settings.value(QStringLiteral("umuPath")).toString();
}

void SettingsManager::setUmuPath(const QString &path)
{
    if (umuPath() == path)
        return;
    m_settings.setValue(QStringLiteral("umuPath"), path);
    Q_EMIT umuPathChanged();
}

QStringList SettingsManager::globalEnvVars() const
{
    return m_settings.value(QStringLiteral("globalEnvVars")).toStringList();
}

void SettingsManager::setGlobalEnvVars(const QStringList &vars)
{
    m_settings.setValue(QStringLiteral("globalEnvVars"), vars);
    Q_EMIT globalEnvVarsChanged();
}

bool SettingsManager::lightsOut() const
{
    return m_settings.value(QStringLiteral("lightsOut"), false).toBool();
}

void SettingsManager::setLightsOut(bool enabled)
{
    if (lightsOut() == enabled)
        return;
    m_settings.setValue(QStringLiteral("lightsOut"), enabled);
    Q_EMIT lightsOutChanged();
}

QString SettingsManager::lightsOutColor() const
{
    return m_settings.value(QStringLiteral("lightsOutColor"), QStringLiteral("#0d1b3e")).toString();
}

void SettingsManager::setLightsOutColor(const QString &color)
{
    if (lightsOutColor() == color)
        return;
    m_settings.setValue(QStringLiteral("lightsOutColor"), color);
    Q_EMIT lightsOutColorChanged();
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

bool SettingsManager::showTabBar() const
{
    return m_settings.value(QStringLiteral("showTabBar"), true).toBool();
}

void SettingsManager::setShowTabBar(bool show)
{
    if (showTabBar() == show)
        return;
    m_settings.setValue(QStringLiteral("showTabBar"), show);
    Q_EMIT showTabBarChanged();
}

bool SettingsManager::bigPicture() const
{
    return m_settings.value(QStringLiteral("bigPicture"), false).toBool();
}

void SettingsManager::setBigPicture(bool enabled)
{
    if (bigPicture() == enabled)
        return;
    m_settings.setValue(QStringLiteral("bigPicture"), enabled);
    Q_EMIT bigPictureChanged();
}

QString SettingsManager::steamGridDbApiKey() const
{
    return m_settings.value(QStringLiteral("steamGridDbApiKey")).toString();
}

void SettingsManager::setSteamGridDbApiKey(const QString &key)
{
    if (steamGridDbApiKey() == key)
        return;
    m_settings.setValue(QStringLiteral("steamGridDbApiKey"), key);
    Q_EMIT steamGridDbApiKeyChanged();
}

bool SettingsManager::autoDownloadArt() const
{
    return m_settings.value(QStringLiteral("autoDownloadArt"), true).toBool();
}

void SettingsManager::setAutoDownloadArt(bool enabled)
{
    if (autoDownloadArt() == enabled)
        return;
    m_settings.setValue(QStringLiteral("autoDownloadArt"), enabled);
    Q_EMIT autoDownloadArtChanged();
}

QString SettingsManager::rommServerUrl() const
{
    return m_settings.value(QStringLiteral("rommServerUrl")).toString();
}

void SettingsManager::setRommServerUrl(const QString &url)
{
    if (rommServerUrl() == url)
        return;
    m_settings.setValue(QStringLiteral("rommServerUrl"), url);
    Q_EMIT rommServerUrlChanged();
}

QString SettingsManager::rommApiKey() const
{
    return m_settings.value(QStringLiteral("rommApiKey")).toString();
}

void SettingsManager::setRommApiKey(const QString &key)
{
    if (rommApiKey() == key)
        return;
    m_settings.setValue(QStringLiteral("rommApiKey"), key);
    Q_EMIT rommApiKeyChanged();
}

QString SettingsManager::retroarchPath() const
{
    return m_settings.value(QStringLiteral("retroarchPath")).toString();
}

void SettingsManager::setRetroarchPath(const QString &path)
{
    if (retroarchPath() == path)
        return;
    m_settings.setValue(QStringLiteral("retroarchPath"), path);
    Q_EMIT retroarchPathChanged();
}

QString SettingsManager::romCacheDir() const
{
    QString stored = m_settings.value(QStringLiteral("romCacheDir")).toString();
    if (!stored.isEmpty())
        return stored;
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/romm");
}

void SettingsManager::setRomCacheDir(const QString &dir)
{
    if (m_settings.value(QStringLiteral("romCacheDir")).toString() == dir)
        return;
    m_settings.setValue(QStringLiteral("romCacheDir"), dir);
    Q_EMIT romCacheDirChanged();
}

bool SettingsManager::firstRunComplete() const
{
    return m_settings.value(QStringLiteral("firstRunComplete"), false).toBool();
}

void SettingsManager::setFirstRunComplete(bool complete)
{
    if (firstRunComplete() == complete)
        return;
    m_settings.setValue(QStringLiteral("firstRunComplete"), complete);
    Q_EMIT firstRunCompleteChanged();
}

bool SettingsManager::sleepInhibited() const
{
    return m_settings.value(QStringLiteral("sleepInhibited"), false).toBool();
}

void SettingsManager::setSleepInhibited(bool inhibited)
{
    if (sleepInhibited() == inhibited)
        return;
    m_settings.setValue(QStringLiteral("sleepInhibited"), inhibited);
    Q_EMIT sleepInhibitedChanged();
}

bool SettingsManager::showTips() const
{
    return m_settings.value(QStringLiteral("showTips"), true).toBool();
}

void SettingsManager::setShowTips(bool enabled)
{
    if (showTips() == enabled)
        return;
    m_settings.setValue(QStringLiteral("showTips"), enabled);
    Q_EMIT showTipsChanged();
}

QVariantMap SettingsManager::rommCoreMap() const
{
    QString json = m_settings.value(QStringLiteral("rommCoreMap")).toString();
    if (json.isEmpty())
        return {};
    return QJsonDocument::fromJson(json.toUtf8()).object().toVariantMap();
}

QString SettingsManager::rommCore(const QString &platformSlug) const
{
    return rommCoreMap().value(platformSlug).toString();
}

void SettingsManager::setRommCore(const QString &platformSlug, const QString &corePath)
{
    QVariantMap map = rommCoreMap();
    if (corePath.isEmpty())
        map.remove(platformSlug);
    else
        map[platformSlug] = corePath;
    m_settings.setValue(QStringLiteral("rommCoreMap"), QString::fromUtf8(QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact)));
    Q_EMIT rommCoreMapChanged();
}

QVariantMap SettingsManager::rommGameCoreMap() const
{
    QString json = m_settings.value(QStringLiteral("rommGameCoreMap")).toString();
    if (json.isEmpty())
        return {};
    return QJsonDocument::fromJson(json.toUtf8()).object().toVariantMap();
}

QString SettingsManager::rommGameCore(int romId) const
{
    return rommGameCoreMap().value(QString::number(romId)).toString();
}

void SettingsManager::setRommGameCore(int romId, const QString &corePath)
{
    QVariantMap map = rommGameCoreMap();
    QString key = QString::number(romId);
    if (corePath.isEmpty())
        map.remove(key);
    else
        map[key] = corePath;
    m_settings.setValue(QStringLiteral("rommGameCoreMap"), QString::fromUtf8(QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact)));
    Q_EMIT rommGameCoreMapChanged();
}
