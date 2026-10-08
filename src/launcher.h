#pragma once

#include <QHash>
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStringList>
#include <QVariantMap>

class QTimer;
class GpuManager;

class Launcher : public QObject
{
    Q_OBJECT

public:
    explicit Launcher(QObject *parent = nullptr);

    void setUmuPath(const QString &path);
    void setGlobalEnvVars(const QStringList &vars);
    void setGlobalLaunchOptions(const QVariantMap &options);
    void setGpuManager(GpuManager *gpus);
    void setDefaultGpu(const QString &gpu);
    void setRetroarchPath(const QString &path);
    void setUzdoomPath(const QString &path);
    void setRommCoreMap(const QVariantMap &map);
    void setRommGameCoreMap(const QVariantMap &map);
    void setDefaultRuntimeType(const QString &type);
    void setDefaultProtonPath(const QString &path);
    void setDefaultWineBinary(const QString &path);
    void setLsfgDllPath(const QString &path);

    Q_PROPERTY(QStringList activeLaunchKeys READ activeLaunchKeys NOTIFY activeLaunchesChanged)
    QStringList activeLaunchKeys() const
    {
        return m_activeLaunches.keys();
    }

    Q_PROPERTY(QStringList runningExePaths READ runningExePaths NOTIFY runningExePathsChanged)
    QStringList runningExePaths() const
    {
        return m_runningProcesses.keys();
    }

    Q_INVOKABLE qint64 launchEntry(const QVariantMap &app);
    Q_INVOKABLE void launchRom(const QVariantMap &rom, bool enableLogging = false, const QString &launchOptions = {}, bool autoHdr = false);
    Q_INVOKABLE QString detectRetroarchPath() const;
    Q_INVOKABLE QStringList availableCoresForPlatform(const QString &platformSlug) const;
    Q_INVOKABLE QString buildRomLaunchCommand(const QVariantMap &rom) const;
    Q_INVOKABLE void copyToClipboard(const QString &text) const;
    Q_INVOKABLE void openExternalUrl(const QString &url) const;
    Q_INVOKABLE void stopEntry(const QVariantMap &app);
    Q_INVOKABLE void stopLaunch(const QString &key);
    Q_INVOKABLE void launchSteamWithoutOptions();
    Q_INVOKABLE void cancelSteamLaunch();
    Q_INVOKABLE qint64 runInPrefix(const QVariantMap &app, const QString &exePath);
    Q_INVOKABLE qint64 runningPidForExe(const QString &exePath) const;
    Q_INVOKABLE void runWinecfg(const QVariantMap &app);
    Q_INVOKABLE void runRegedit(const QVariantMap &app);
    Q_INVOKABLE void runWinetricks(const QVariantMap &app);
    Q_INVOKABLE bool isWinetricksAvailable() const;
    Q_INVOKABLE bool isMangohudAvailable() const;
    Q_INVOKABLE bool isGamemodeAvailable() const;
    Q_INVOKABLE QString autoDetectLsfgDll() const;
    Q_INVOKABLE QString logDir() const;
    Q_INVOKABLE QStringList platformSlugs() const;

    Q_PROPERTY(bool sleepInhibited READ sleepInhibited NOTIFY sleepInhibitedChanged)
    Q_INVOKABLE void toggleSleepInhibit();
    bool sleepInhibited() const;

    Q_PROPERTY(bool hdrEnabled READ hdrEnabled NOTIFY hdrEnabledChanged)
    Q_PROPERTY(bool hdrSupported READ hdrSupported NOTIFY hdrSupportedChanged)
    Q_INVOKABLE void toggleHdr();
    void restoreHdrState();
    bool hdrEnabled() const;
    bool hdrSupported() const;

Q_SIGNALS:
    void retroarchBinaryChanged();
    void launched(const QString &name);
    void activeLaunchesChanged();
    void steamOverrideRequested(const QString &name, const QString &message);
    void launchError(const QString &name, const QString &error);
    void romCoreMissing(const QString &platformSlug, const QVariantMap &rom);
    void coreAutoDetected(const QString &platformSlug, const QString &corePath);
    void prefixNotReady(const QString &name);
    void processFinished(int exitCode);
    void logSessionStarted(const QString &id, const QString &name);
    void logOutput(const QString &id, const QString &text);
    void logSessionFinished(const QString &id, int exitCode);
    void runningExePathsChanged();
    void sleepInhibitedChanged();
    void hdrEnabledChanged();
    void hdrSupportedChanged();

private:
    qint64 launch(const QString &binary,
                  const QStringList &baseArgs,
                  const QString &exePath,
                  const QProcessEnvironment &env,
                  const QString &launchOptions,
                  bool enableLogging,
                  const QString &logName,
                  bool appendExe = true,
                  const QStringList &commandWrappers = QStringList(),
                  bool autoHdr = false);
    void setupLogging(QProcess *proc, const QString &name);
    void refreshHdrState();
    void cacheRetroarchBinary();
    QString autoDetectCore(const QString &platformSlug) const;
    QString m_logDir;
    QString m_umuPath;
    QString m_retroarchPath;
    QString m_uzdoomPath;
    QString m_retroarchBinary;
    QVariantMap m_rommCoreMap;
    QVariantMap m_rommGameCoreMap;
    QStringList m_globalEnvVars;
    QVariantMap m_globalLaunchOptions;
    GpuManager *m_gpuManager = nullptr;
    QString m_defaultGpu;
    QString m_defaultRuntimeType;
    QString m_defaultProtonPath;
    QString m_defaultWineBinary;
    QString m_lsfgDllPath;
    QHash<QString, QProcess *> m_runningProcesses;
    QVariantMap m_watchCandidate;
    QHash<QString, QVariantMap> m_activeLaunches;
    void trackLaunch(const QString &key, const QVariantMap &app);
    void finishLaunch(const QString &key);
    qint64 startSteamGame(const QVariantMap &app, const QProcessEnvironment &env);
    void launchSteamUrl(const QVariantMap &steamApp);
    QVariantMap m_pendingSteamLaunch;
    void stopSteamGame(int appId);
    void startSteamPoll();
    void pollSteamLaunches();
    QTimer *m_steamPoll = nullptr;
    int m_inhibitFd = -1;
    QString m_inhibitPortalRequestPath;
    bool m_hdrEnabled = false;
    bool m_hdrSupported = false;
    bool m_hdrEnabledByUs = false;
    bool m_wcgEnabled = false;
    bool m_wcgEnabledBeforeHdr = false;
};
