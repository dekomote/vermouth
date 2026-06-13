#include "goginstaller.h"
#include "goglibrary.h"
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>

GogInstaller::GogInstaller(QObject *parent)
    : QObject(parent)
{
}

void GogInstaller::setUmuPath(const QString &path)
{
    m_umuPath = path;
}

bool GogInstaller::busy() const
{
    return m_busy;
}

void GogInstaller::runProcess(const QString &gameId, const QString &program, const QStringList &args, const QProcessEnvironment &env, const QString &workingDir)
{
    if (m_busy) {
        Q_EMIT installError(gameId, QStringLiteral("An installation is already in progress"));
        return;
    }
    m_busy = true;
    Q_EMIT busyChanged();
    Q_EMIT installStarted(gameId);

    auto *proc = new QProcess(this);
    proc->setProcessEnvironment(env);
    if (!workingDir.isEmpty())
        proc->setWorkingDirectory(workingDir);

    connect(proc, &QProcess::finished, this, [this, proc, gameId](int exitCode) {
        m_busy = false;
        Q_EMIT busyChanged();
        Q_EMIT installFinished(gameId, exitCode);
        proc->deleteLater();
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc, gameId](QProcess::ProcessError) {
        if (proc->state() == QProcess::NotRunning && m_busy) {
            m_busy = false;
            Q_EMIT busyChanged();
            Q_EMIT installError(gameId, proc->errorString());
        }
    });

    proc->start(program, args);
    if (!proc->waitForStarted(5000)) {
        m_busy = false;
        Q_EMIT busyChanged();
        Q_EMIT installError(gameId, proc->errorString());
        proc->deleteLater();
    }
}

void GogInstaller::installWindows(const QString &gameId,
                                  const QString &installerExe,
                                  const QString &runtimeType,
                                  const QString &runtimePath,
                                  const QString &prefix,
                                  bool silent)
{
    if (!QFileInfo::exists(installerExe)) {
        Q_EMIT installError(gameId, QStringLiteral("Installer not found: %1").arg(installerExe));
        return;
    }
    QDir().mkpath(prefix);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString workingDir = QFileInfo(installerExe).absolutePath();

    QStringList args;
    if (silent) {
        args = {
            QStringLiteral("/DIR=c:\\game"),
            QStringLiteral("/LANG=en-US"),
            QStringLiteral("/SP-"),
            QStringLiteral("/SILENT"),
            QStringLiteral("/NORESTART"),
            QStringLiteral("/SUPPRESSMSGBOXES"),
        };
    }

    if (runtimeType == QStringLiteral("wine")) {
        env.insert(QStringLiteral("WINEPREFIX"), prefix);
        env.insert(QStringLiteral("WINEDLLOVERRIDES"), QStringLiteral("mscoree=;mshtml="));
        runProcess(gameId, runtimePath, QStringList{installerExe} + args, env, workingDir);
        return;
    }

    QString umuBin = m_umuPath;
    if (umuBin.isEmpty())
        umuBin = QStandardPaths::findExecutable(QStringLiteral("umu-run"));

    if (!umuBin.isEmpty()) {
        env.insert(QStringLiteral("PROTONPATH"), runtimePath);
        env.insert(QStringLiteral("STEAM_COMPAT_DATA_PATH"), prefix);
        env.insert(QStringLiteral("GAMEID"), QStringLiteral("0"));
        env.insert(QStringLiteral("WINEPREFIX"), prefix);
        runProcess(gameId, umuBin, QStringList{installerExe} + args, env, workingDir);
    } else {
        env.insert(QStringLiteral("STEAM_COMPAT_CLIENT_INSTALL_PATH"), QDir::homePath() + QStringLiteral("/.steam/steam"));
        env.insert(QStringLiteral("STEAM_COMPAT_DATA_PATH"), prefix);
        runProcess(gameId, runtimePath + QStringLiteral("/proton"), QStringList{QStringLiteral("run"), installerExe} + args, env, workingDir);
    }
}

void GogInstaller::installLinux(const QString &gameId, const QString &installerSh, const QString &destDir)
{
    if (!QFileInfo::exists(installerSh)) {
        Q_EMIT installError(gameId, QStringLiteral("Installer not found: %1").arg(installerSh));
        return;
    }
    QDir().mkpath(destDir);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    runProcess(gameId,
               QStringLiteral("unzip"),
               {QStringLiteral("-o"), QStringLiteral("-qq"), installerSh, QStringLiteral("-d"), destDir},
               env,
               QFileInfo(installerSh).absolutePath());
}

static QVariantMap entryToMap(const GogEntry &e)
{
    QVariantMap m;
    m[QStringLiteral("name")] = e.name;
    m[QStringLiteral("exePath")] = e.exePath;
    m[QStringLiteral("iconPath")] = e.iconPath;
    m[QStringLiteral("launchOptions")] = e.arguments;
    m[QStringLiteral("isWindows")] = e.isWindows;
    return m;
}

QVariantMap GogInstaller::findInstalledWindowsGame(const QString &prefix, const QString &gameName, const QString &runtimeType) const
{
    // Proton keeps the prefix under pfx/, plain Wine uses the prefix directly.
    QString driveC = (runtimeType == QStringLiteral("wine")) ? prefix + QStringLiteral("/drive_c") : prefix + QStringLiteral("/pfx/drive_c");
    const QStringList roots = {
        driveC + QStringLiteral("/game"),
        driveC + QStringLiteral("/GOG Games"),
        driveC + QStringLiteral("/Program Files (x86)/GOG.com"),
        driveC + QStringLiteral("/Program Files/GOG.com"),
        driveC,
    };

    QVector<GogEntry> all;
    for (const QString &root : roots) {
        for (const GogEntry &e : GogLibrary::scan(root)) {
            if (e.isWindows && !e.exePath.isEmpty())
                all.append(e);
        }
        if (!all.isEmpty())
            break;
    }
    if (all.isEmpty())
        return {};

    // Prefer a title that matches the GOG library name.
    QString want = gameName.toLower();
    for (const GogEntry &e : std::as_const(all)) {
        if (e.name.toLower() == want)
            return entryToMap(e);
    }
    for (const GogEntry &e : std::as_const(all)) {
        if (e.name.toLower().contains(want) || want.contains(e.name.toLower()))
            return entryToMap(e);
    }
    return entryToMap(all.first());
}

QVariantMap GogInstaller::findInstalledLinuxGame(const QString &destDir) const
{
    // unzip puts the playable game under <destDir>/data/noarch.
    const QStringList roots = {
        destDir + QStringLiteral("/data/noarch"),
        destDir,
    };
    for (const QString &root : roots) {
        for (const GogEntry &e : GogLibrary::scan(root)) {
            if (!e.isWindows && !e.exePath.isEmpty())
                return entryToMap(e);
        }
    }
    return {};
}
