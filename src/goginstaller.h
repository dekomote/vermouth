#pragma once

#include <QObject>
#include <QVariantMap>

class QProcess;
class QProcessEnvironment;

class GogInstaller : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit GogInstaller(QObject *parent = nullptr);

    void setUmuPath(const QString &path);

    bool busy() const;

    Q_INVOKABLE void installWindows(const QString &gameId,
                                    const QString &installerExe,
                                    const QString &runtimeType,
                                    const QString &runtimePath,
                                    const QString &prefix,
                                    bool silent);
    Q_INVOKABLE void installLinux(const QString &gameId, const QString &installerSh, const QString &destDir);

    Q_INVOKABLE QVariantMap findInstalledWindowsGame(const QString &prefix, const QString &gameName, const QString &runtimeType) const;
    Q_INVOKABLE QVariantMap findInstalledLinuxGame(const QString &destDir) const;

Q_SIGNALS:
    void busyChanged();
    void installStarted(const QString &gameId);
    void installFinished(const QString &gameId, int exitCode);
    void installError(const QString &gameId, const QString &message);

private:
    void runProcess(const QString &gameId,
                    const QString &program,
                    const QStringList &args,
                    const QProcessEnvironment &env,
                    const QString &workingDir,
                    const QString &logPath = QString());

    QString m_umuPath;
    bool m_busy = false;
};
