#pragma once

#include <QDBusUnixFileDescriptor>
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>

class Launcher : public QObject
{
    Q_OBJECT

public:
    explicit Launcher(QObject *parent = nullptr);

    void setUmuPath(const QString &path);

    Q_INVOKABLE void launchEntry(const QVariantMap &app);
    Q_INVOKABLE void runInPrefix(const QVariantMap &app, const QString &exePath);
    Q_INVOKABLE void runWinecfg(const QVariantMap &app);
    Q_INVOKABLE void runRegedit(const QVariantMap &app);
    Q_INVOKABLE void runWinetricks(const QVariantMap &app);
    Q_INVOKABLE bool isWinetricksAvailable() const;
    Q_INVOKABLE QString logDir() const;

    Q_PROPERTY(bool sleepInhibited READ sleepInhibited NOTIFY sleepInhibitedChanged)
    Q_INVOKABLE void toggleSleepInhibit();
    bool sleepInhibited() const;

    Q_PROPERTY(bool hdrEnabled READ hdrEnabled NOTIFY hdrEnabledChanged)
    Q_PROPERTY(bool hdrSupported READ hdrSupported NOTIFY hdrSupportedChanged)
    Q_INVOKABLE void toggleHdr();
    bool hdrEnabled() const;
    bool hdrSupported() const;

Q_SIGNALS:
    void launched(const QString &name);
    void launchError(const QString &name, const QString &error);
    void prefixNotReady(const QString &name);
    void processFinished(int exitCode);
    void sleepInhibitedChanged();
    void hdrEnabledChanged();
    void hdrSupportedChanged();

private:
    void launch(const QString &binary,
                const QStringList &baseArgs,
                const QString &exePath,
                const QProcessEnvironment &env,
                const QString &launchOptions,
                bool enableLogging,
                const QString &logName);
    void setupLogging(QProcess *proc, const QString &name);
    void refreshHdrState();
    QString m_logDir;
    QString m_umuPath;
    int m_inhibitFd = -1;
    bool m_hdrEnabled = false;
    bool m_hdrSupported = false;
};
