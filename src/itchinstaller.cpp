#include "itchinstaller.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTimer>

namespace
{
bool isJunkName(const QString &lowerBaseName)
{
    static const QStringList junk = {
        QStringLiteral("unins"),
        QStringLiteral("vcredist"),
        QStringLiteral("vc_redist"),
        QStringLiteral("dotnetfx"),
        QStringLiteral("dxsetup"),
        QStringLiteral("crashpad"),
        QStringLiteral("crashhandler"),
        QStringLiteral("directx"),
    };
    for (const auto &j : junk) {
        if (lowerBaseName.contains(j))
            return true;
    }
    return false;
}

// Returns the archive extraction args for the given file, or an empty list if
// the file isn't a recognized archive (i.e. it's a bare runnable download).
struct ExtractCommand {
    QString program;
    QStringList args;
};

ExtractCommand archiveCommand(const QString &path, const QString &destDir)
{
    QString lower = path.toLower();
    if (lower.endsWith(QStringLiteral(".zip")))
        return {QStringLiteral("unzip"), {QStringLiteral("-o"), QStringLiteral("-qq"), path, QStringLiteral("-d"), destDir}};
    if (lower.endsWith(QStringLiteral(".tar.gz")) || lower.endsWith(QStringLiteral(".tgz")))
        return {QStringLiteral("tar"), {QStringLiteral("xzf"), path, QStringLiteral("-C"), destDir}};
    if (lower.endsWith(QStringLiteral(".tar.bz2")) || lower.endsWith(QStringLiteral(".tbz2")))
        return {QStringLiteral("tar"), {QStringLiteral("xjf"), path, QStringLiteral("-C"), destDir}};
    if (lower.endsWith(QStringLiteral(".tar.xz")) || lower.endsWith(QStringLiteral(".txz")))
        return {QStringLiteral("tar"), {QStringLiteral("xJf"), path, QStringLiteral("-C"), destDir}};
    if (lower.endsWith(QStringLiteral(".tar")))
        return {QStringLiteral("tar"), {QStringLiteral("xf"), path, QStringLiteral("-C"), destDir}};
    return {};
}
}

ItchInstaller::ItchInstaller(QObject *parent)
    : QObject(parent)
{
}

void ItchInstaller::setUmuPath(const QString &path)
{
    m_umuPath = path;
}

bool ItchInstaller::busy() const
{
    return m_busy;
}

bool ItchInstaller::isBareExecutable(const QString &downloadedPath) const
{
    return archiveCommand(downloadedPath, {}).program.isEmpty();
}

void ItchInstaller::runProcess(const QString &gameId,
                               const QString &program,
                               const QStringList &args,
                               const QProcessEnvironment &env,
                               const QString &workingDir)
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

void ItchInstaller::extractDownload(const QString &gameId, const QString &downloadedPath, const QString &destDir)
{
    if (!QFileInfo::exists(downloadedPath)) {
        Q_EMIT installError(gameId, QStringLiteral("Downloaded file not found: %1").arg(downloadedPath));
        return;
    }
    QDir().mkpath(destDir);

    auto cmd = archiveCommand(downloadedPath, destDir);
    if (!cmd.program.isEmpty()) {
        runProcess(gameId, cmd.program, cmd.args, QProcessEnvironment::systemEnvironment(), QFileInfo(downloadedPath).absolutePath());
        return;
    }

    // Bare runnable download (native binary, AppImage, or a rare non-archived exe):
    // just copy it into place. Mirrors runProcess's async signal shape.
    if (m_busy) {
        Q_EMIT installError(gameId, QStringLiteral("An installation is already in progress"));
        return;
    }
    m_busy = true;
    Q_EMIT busyChanged();
    Q_EMIT installStarted(gameId);

    QString dest = destDir + QLatin1Char('/') + QFileInfo(downloadedPath).fileName();
    bool ok = QFile::copy(downloadedPath, dest);
    if (ok)
        QFile::setPermissions(dest, QFile::permissions(dest) | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther);

    QTimer::singleShot(0, this, [this, gameId, ok]() {
        m_busy = false;
        Q_EMIT busyChanged();
        if (ok)
            Q_EMIT installFinished(gameId, 0);
        else
            Q_EMIT installError(gameId, QStringLiteral("Could not copy the downloaded file into place"));
    });
}

void ItchInstaller::runWindowsInstaller(const QString &gameId,
                                        const QString &installerExe,
                                        const QString &runtimeType,
                                        const QString &runtimePath,
                                        const QString &prefix)
{
    if (!QFileInfo::exists(installerExe)) {
        Q_EMIT installError(gameId, QStringLiteral("Installer not found: %1").arg(installerExe));
        return;
    }
    QDir().mkpath(prefix);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString workingDir = QFileInfo(installerExe).absolutePath();

    if (runtimeType == QStringLiteral("wine")) {
        env.insert(QStringLiteral("WINEPREFIX"), prefix);
        env.insert(QStringLiteral("WINEDLLOVERRIDES"), QStringLiteral("mscoree=;mshtml="));
        runProcess(gameId, runtimePath, {installerExe}, env, workingDir);
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
        runProcess(gameId, umuBin, {installerExe}, env, workingDir);
    } else {
        env.insert(QStringLiteral("STEAM_COMPAT_CLIENT_INSTALL_PATH"), QDir::homePath() + QStringLiteral("/.steam/steam"));
        env.insert(QStringLiteral("STEAM_COMPAT_DATA_PATH"), prefix);
        runProcess(gameId, runtimePath + QStringLiteral("/proton"), {QStringLiteral("run"), installerExe}, env, workingDir);
    }
}

QVariantMap ItchInstaller::findGame(const QString &destDir, const QString &gameName, bool isWindows) const
{
    struct Candidate {
        QString path;
        qint64 size;
    };
    QVector<Candidate> candidates;

    QDirIterator it(destDir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString path = it.next();
        QFileInfo fi(path);
        QString lowerName = fi.fileName().toLower();
        QString lowerPath = path.toLower();

        if (lowerPath.contains(QStringLiteral("/__macosx/")))
            continue;
        if (isJunkName(lowerName))
            continue;

        bool matches = false;
        if (isWindows) {
            matches = lowerName.endsWith(QStringLiteral(".exe"));
            if (matches && (lowerPath.contains(QStringLiteral("/redist/")) || lowerPath.contains(QStringLiteral("_commonredist"))))
                matches = false;
        } else {
            if (lowerName.endsWith(QStringLiteral(".x86_64")) || lowerName.endsWith(QStringLiteral(".x86")) || lowerName.endsWith(QStringLiteral(".appimage"))
                || lowerName.endsWith(QStringLiteral(".sh")))
                matches = true;
            else if (fi.isExecutable() && fi.suffix().isEmpty())
                matches = true;
        }

        if (matches)
            candidates.append({path, fi.size()});
    }

    if (candidates.isEmpty())
        return {};

    QString want = gameName.toLower();
    want.remove(QLatin1Char(' '));
    const Candidate *best = nullptr;
    for (const auto &c : std::as_const(candidates)) {
        QString base = QFileInfo(c.path).completeBaseName().toLower();
        base.remove(QLatin1Char(' '));
        if (base == want) {
            best = &c;
            break;
        }
    }
    if (!best) {
        for (const auto &c : std::as_const(candidates)) {
            QString base = QFileInfo(c.path).completeBaseName().toLower();
            base.remove(QLatin1Char(' '));
            if (base.contains(want) || want.contains(base)) {
                best = &c;
                break;
            }
        }
    }
    if (!best) {
        best = &candidates.first();
        for (const auto &c : std::as_const(candidates)) {
            if (c.size > best->size)
                best = &c;
        }
    }

    if (!isWindows)
        QFile::setPermissions(best->path, QFile::permissions(best->path) | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther);

    QVariantMap m;
    m[QStringLiteral("exePath")] = best->path;
    m[QStringLiteral("name")] = QFileInfo(best->path).completeBaseName();
    return m;
}

QString ItchInstaller::copyGridCover(const QString &sourcePath, const QString &assetsPath, const QString &safeName) const
{
    if (sourcePath.isEmpty() || !QFileInfo::exists(sourcePath))
        return {};

    QString ext = QFileInfo(sourcePath).suffix();
    if (ext.isEmpty())
        ext = QStringLiteral("png");

    QDir().mkpath(assetsPath);
    QString dest = assetsPath + QLatin1Char('/') + safeName + QStringLiteral("_grid.") + ext;
    QFile::remove(dest); // QFile::copy refuses to overwrite an existing file.
    return QFile::copy(sourcePath, dest) ? dest : QString();
}
