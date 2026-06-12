#pragma once

#include <QObject>
#include <QVariantMap>

class QProcess;
class QProcessEnvironment;

// Runs a downloaded GOG installer: Windows installers through Proton in a fresh
// prefix, Linux mojosetup installers via a silent extract. After the process
// exits, helpers scan the install location to locate the game executable.
class GogInstaller : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit GogInstaller(QObject *parent = nullptr);

    void setUmuPath(const QString &path);

    bool busy() const;

    // Run a Windows .exe installer with the chosen runtime ("proton" or "wine")
    // inside prefix (created if needed). runtimePath is the Proton dir or the
    // Wine binary respectively.
    Q_INVOKABLE void
    installWindows(const QString &gameId, const QString &installerExe, const QString &runtimeType, const QString &runtimePath, const QString &prefix);
    // Run a Linux .sh installer silently, extracting into destDir.
    Q_INVOKABLE void installLinux(const QString &gameId, const QString &installerSh, const QString &destDir);

    // After a Windows install, scan the prefix drive_c for the installed game's
    // executable. runtimeType ("proton"/"wine") determines the drive_c location.
    // Returns a map {name, exePath, iconPath, launchOptions, isWindows} or empty.
    Q_INVOKABLE QVariantMap findInstalledWindowsGame(const QString &prefix, const QString &gameName, const QString &runtimeType) const;
    // After a Linux install, scan destDir for the start.sh launcher.
    Q_INVOKABLE QVariantMap findInstalledLinuxGame(const QString &destDir) const;

Q_SIGNALS:
    void busyChanged();
    void installStarted(const QString &gameId);
    void installFinished(const QString &gameId, int exitCode);
    void installError(const QString &gameId, const QString &message);

private:
    void runProcess(const QString &gameId, const QString &program, const QStringList &args, const QProcessEnvironment &env, const QString &workingDir);

    QString m_umuPath;
    bool m_busy = false;
};
