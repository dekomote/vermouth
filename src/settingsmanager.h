#pragma once

#include <QObject>
#include <QSettings>
#include <QStringList>

class SettingsManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString defaultPrefixDir READ defaultPrefixDir WRITE setDefaultPrefixDir NOTIFY defaultPrefixDirChanged)
    Q_PROPERTY(QString defaultGamePrefix READ defaultGamePrefix WRITE setDefaultGamePrefix NOTIFY defaultGamePrefixChanged)
    Q_PROPERTY(QString defaultWinePrefix READ defaultWinePrefix WRITE setDefaultWinePrefix NOTIFY defaultWinePrefixChanged)
    Q_PROPERTY(QString gamepadFullscreenButton READ gamepadFullscreenButton WRITE setGamepadFullscreenButton NOTIFY gamepadFullscreenButtonChanged)
    Q_PROPERTY(QStringList extraProtonPaths READ extraProtonPaths WRITE setExtraProtonPaths NOTIFY extraProtonPathsChanged)
    Q_PROPERTY(QString defaultRuntimeType READ defaultRuntimeType WRITE setDefaultRuntimeType NOTIFY defaultRuntimeChanged)
    Q_PROPERTY(QString defaultProtonPath READ defaultProtonPath WRITE setDefaultProtonPath NOTIFY defaultRuntimeChanged)
    Q_PROPERTY(QString defaultWineBinary READ defaultWineBinary WRITE setDefaultWineBinary NOTIFY defaultRuntimeChanged)
    Q_PROPERTY(bool drawerPinned READ drawerPinned WRITE setDrawerPinned NOTIFY drawerPinnedChanged)
    Q_PROPERTY(QString umuPath READ umuPath WRITE setUmuPath NOTIFY umuPathChanged)
    Q_PROPERTY(QStringList globalEnvVars READ globalEnvVars WRITE setGlobalEnvVars NOTIFY globalEnvVarsChanged)
    Q_PROPERTY(bool lightsOut READ lightsOut WRITE setLightsOut NOTIFY lightsOutChanged)
    Q_PROPERTY(QString lightsOutColor READ lightsOutColor WRITE setLightsOutColor NOTIFY lightsOutColorChanged)
    Q_PROPERTY(bool bigPicture READ bigPicture WRITE setBigPicture NOTIFY bigPictureChanged)
    Q_PROPERTY(QString steamGridDbApiKey READ steamGridDbApiKey WRITE setSteamGridDbApiKey NOTIFY steamGridDbApiKeyChanged)
    Q_PROPERTY(bool autoDownloadArt READ autoDownloadArt WRITE setAutoDownloadArt NOTIFY autoDownloadArtChanged)
    Q_PROPERTY(QString rommServerUrl READ rommServerUrl WRITE setRommServerUrl NOTIFY rommServerUrlChanged)
    Q_PROPERTY(QString rommApiKey READ rommApiKey WRITE setRommApiKey NOTIFY rommApiKeyChanged)
    Q_PROPERTY(QString gogRefreshToken READ gogRefreshToken WRITE setGogRefreshToken NOTIFY gogRefreshTokenChanged)
    Q_PROPERTY(QString gogUsername READ gogUsername WRITE setGogUsername NOTIFY gogUsernameChanged)
    Q_PROPERTY(QString gogCacheDir READ gogCacheDir WRITE setGogCacheDir NOTIFY gogCacheDirChanged)
    Q_PROPERTY(QString gogInstallDir READ gogInstallDir WRITE setGogInstallDir NOTIFY gogInstallDirChanged)
    Q_PROPERTY(QString retroarchPath READ retroarchPath WRITE setRetroarchPath NOTIFY retroarchPathChanged)
    Q_PROPERTY(QString uzdoomPath READ uzdoomPath WRITE setUzdoomPath NOTIFY uzdoomPathChanged)
    Q_PROPERTY(QString romCacheDir READ romCacheDir WRITE setRomCacheDir NOTIFY romCacheDirChanged)
    Q_PROPERTY(bool firstRunComplete READ firstRunComplete WRITE setFirstRunComplete NOTIFY firstRunCompleteChanged)
    Q_PROPERTY(bool showTips READ showTips WRITE setShowTips NOTIFY showTipsChanged)
    Q_PROPERTY(bool sleepInhibited READ sleepInhibited WRITE setSleepInhibited NOTIFY sleepInhibitedChanged)
    Q_PROPERTY(bool gridAltBackground READ gridAltBackground WRITE setGridAltBackground NOTIFY gridAltBackgroundChanged)
    Q_PROPERTY(bool showRunnerIcons READ showRunnerIcons WRITE setShowRunnerIcons NOTIFY showRunnerIconsChanged)
    Q_PROPERTY(QString lsfgDllPath READ lsfgDllPath WRITE setLsfgDllPath NOTIFY lsfgDllPathChanged)

public:
    explicit SettingsManager(QObject *parent = nullptr);

    QString defaultPrefixDir() const;
    Q_INVOKABLE void setDefaultPrefixDir(const QString &path);

    QString defaultGamePrefix() const;
    Q_INVOKABLE void setDefaultGamePrefix(const QString &path);

    QString defaultWinePrefix() const;
    Q_INVOKABLE void setDefaultWinePrefix(const QString &path);

    QString gamepadFullscreenButton() const;
    Q_INVOKABLE void setGamepadFullscreenButton(const QString &button);

    QStringList extraProtonPaths() const;
    void setExtraProtonPaths(const QStringList &paths);

    Q_INVOKABLE void addExtraProtonPath(const QString &path);
    Q_INVOKABLE void removeExtraProtonPath(int index);

    QString defaultRuntimeType() const;
    Q_INVOKABLE void setDefaultRuntimeType(const QString &type);

    QString defaultProtonPath() const;
    Q_INVOKABLE void setDefaultProtonPath(const QString &path);

    QString defaultWineBinary() const;
    Q_INVOKABLE void setDefaultWineBinary(const QString &path);

    bool drawerPinned() const;
    Q_INVOKABLE void setDrawerPinned(bool pinned);

    QString umuPath() const;
    Q_INVOKABLE void setUmuPath(const QString &path);

    QStringList globalEnvVars() const;
    Q_INVOKABLE void setGlobalEnvVars(const QStringList &vars);

    bool lightsOut() const;
    Q_INVOKABLE void setLightsOut(bool enabled);

    QString lightsOutColor() const;
    Q_INVOKABLE void setLightsOutColor(const QString &color);

    bool bigPicture() const;
    Q_INVOKABLE void setBigPicture(bool enabled);

    QString steamGridDbApiKey() const;
    Q_INVOKABLE void setSteamGridDbApiKey(const QString &key);

    bool autoDownloadArt() const;
    Q_INVOKABLE void setAutoDownloadArt(bool enabled);

    QString rommServerUrl() const;
    Q_INVOKABLE void setRommServerUrl(const QString &url);
    QString rommApiKey() const;
    Q_INVOKABLE void setRommApiKey(const QString &key);
    QString retroarchPath() const;
    Q_INVOKABLE void setRetroarchPath(const QString &path);
    QString uzdoomPath() const;
    Q_INVOKABLE void setUzdoomPath(const QString &path);
    QString romCacheDir() const;
    Q_INVOKABLE void setRomCacheDir(const QString &dir);

    QString gogRefreshToken() const;
    Q_INVOKABLE void setGogRefreshToken(const QString &token);
    QString gogUsername() const;
    Q_INVOKABLE void setGogUsername(const QString &name);
    QString gogCacheDir() const;
    Q_INVOKABLE void setGogCacheDir(const QString &dir);
    QString gogInstallDir() const;
    Q_INVOKABLE void setGogInstallDir(const QString &dir);

    QVariantMap gogInstalledGames() const;
    Q_INVOKABLE void setGogInstalledGame(const QString &gameId, const QString &exePath);
    Q_INVOKABLE void removeGogInstalledGame(const QString &gameId);

    bool firstRunComplete() const;
    Q_INVOKABLE void setFirstRunComplete(bool complete);

    bool showTips() const;
    Q_INVOKABLE void setShowTips(bool enabled);

    bool sleepInhibited() const;
    Q_INVOKABLE void setSleepInhibited(bool inhibited);

    bool gridAltBackground() const;
    Q_INVOKABLE void setGridAltBackground(bool enabled);

    bool showRunnerIcons() const;
    Q_INVOKABLE void setShowRunnerIcons(bool enabled);

    QString lsfgDllPath() const;
    Q_INVOKABLE void setLsfgDllPath(const QString &path);

    QVariantMap rommCoreMap() const;
    Q_INVOKABLE QString rommCore(const QString &platformSlug) const;
    Q_INVOKABLE void setRommCore(const QString &platformSlug, const QString &corePath);

    QVariantMap rommGameCoreMap() const;
    Q_INVOKABLE QString rommGameCore(int romId) const;
    Q_INVOKABLE void setRommGameCore(int romId, const QString &corePath);

Q_SIGNALS:
    void defaultPrefixDirChanged();
    void defaultGamePrefixChanged();
    void defaultWinePrefixChanged();
    void gamepadFullscreenButtonChanged();
    void extraProtonPathsChanged();
    void defaultRuntimeChanged();
    void drawerPinnedChanged();
    void umuPathChanged();
    void globalEnvVarsChanged();
    void lightsOutChanged();
    void lightsOutColorChanged();
    void bigPictureChanged();
    void steamGridDbApiKeyChanged();
    void autoDownloadArtChanged();
    void rommServerUrlChanged();
    void rommApiKeyChanged();
    void gogRefreshTokenChanged();
    void gogUsernameChanged();
    void gogCacheDirChanged();
    void gogInstallDirChanged();
    void gogInstalledGamesChanged();
    void retroarchPathChanged();
    void uzdoomPathChanged();
    void romCacheDirChanged();
    void firstRunCompleteChanged();
    void showTipsChanged();
    void sleepInhibitedChanged();
    void gridAltBackgroundChanged();
    void showRunnerIconsChanged();
    void lsfgDllPathChanged();
    void rommCoreMapChanged();
    void rommGameCoreMapChanged();

private:
    QSettings m_settings;
};
