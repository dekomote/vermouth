#include "launcher.h"
#include "flatpakutils.h"
#include "platformcores.h"
#include <QClipboard>
#include <QCursor>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMultiHash>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QScreen>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <csignal>
#include <unistd.h>

static bool isKde()
{
    return qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains(QLatin1String("KDE"), Qt::CaseInsensitive);
}

static QStringList kscreenDoctorArgs(const QStringList &args)
{
    if (isInsideFlatpak())
        return QStringList{QStringLiteral("--host"), QStringLiteral("kscreen-doctor")} + args;
    return args;
}

static QString kscreenDoctorBin()
{
    return isInsideFlatpak() ? QStringLiteral("flatpak-spawn") : QStringLiteral("kscreen-doctor");
}

static QString currentScreenName()
{
    QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    return screen ? screen->name() : QString();
}

Launcher::Launcher(QObject *parent)
    : QObject(parent)
{
    m_logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/logs");
    QDir().mkpath(m_logDir);

    refreshHdrState();
    cacheRetroarchBinary();
}

void Launcher::setUmuPath(const QString &path)
{
    m_umuPath = path;
}

void Launcher::setRetroarchPath(const QString &path)
{
    m_retroarchPath = path;
    cacheRetroarchBinary();
}

void Launcher::setUzdoomPath(const QString &path)
{
    m_uzdoomPath = path;
}

void Launcher::setLsfgDllPath(const QString &path)
{
    m_lsfgDllPath = path;
}

void Launcher::setRommCoreMap(const QVariantMap &map)
{
    m_rommCoreMap = map;
}

void Launcher::setRommGameCoreMap(const QVariantMap &map)
{
    m_rommGameCoreMap = map;
}

void Launcher::cacheRetroarchBinary()
{
    if (!m_retroarchPath.isEmpty() && QFileInfo::exists(m_retroarchPath)) {
        m_retroarchBinary = m_retroarchPath;
        return;
    }

    if (isInsideFlatpak()) {
        m_retroarchBinary.clear();
        auto *which = new QProcess(this);
        connect(which, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, which](int exitCode) {
            if (exitCode == 0) {
                const QString found = QString::fromUtf8(which->readAllStandardOutput()).trimmed();
                if (!found.isEmpty()) {
                    m_retroarchBinary = found;
                    Q_EMIT retroarchBinaryChanged();
                    which->deleteLater();
                    return;
                }
            }
            auto *info = new QProcess(this);
            connect(info, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, info](int exitCode) {
                if (exitCode == 0) {
                    m_retroarchBinary = QStringLiteral("flatpak:org.libretro.RetroArch");
                    Q_EMIT retroarchBinaryChanged();
                }
                info->deleteLater();
            });
            info->start(QStringLiteral("flatpak-spawn"),
                        {QStringLiteral("--host"), QStringLiteral("flatpak"), QStringLiteral("info"), QStringLiteral("org.libretro.RetroArch")});
            which->deleteLater();
        });
        which->start(QStringLiteral("flatpak-spawn"), {QStringLiteral("--host"), QStringLiteral("which"), QStringLiteral("retroarch")});
        return;
    }

    QString found = QStandardPaths::findExecutable(QStringLiteral("retroarch"));
    if (!found.isEmpty()) {
        m_retroarchBinary = found;
        return;
    }
    m_retroarchBinary.clear();
    auto *check = new QProcess(this);
    connect(check, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, check](int exitCode) {
        if (exitCode == 0)
            m_retroarchBinary = QStringLiteral("flatpak:org.libretro.RetroArch");
        check->deleteLater();
    });
    check->start(QStringLiteral("flatpak"), {QStringLiteral("info"), QStringLiteral("org.libretro.RetroArch")});
}

QString Launcher::autoDetectCore(const QString &platformSlug) const
{
    const QStringList cores = availableCoresForPlatform(platformSlug);
    return cores.isEmpty() ? QString() : cores.constFirst();
}

QStringList Launcher::availableCoresForPlatform(const QString &platformSlug) const
{
    QStringList candidates = platformCoreMap().value(platformSlug);
    if (candidates.isEmpty())
        return {};

    QStringList dirs = retroarchCoreDirs(m_retroarchBinary);
    const bool isFlatpakRetroarch = m_retroarchBinary == QStringLiteral("flatpak:org.libretro.RetroArch");

    QStringList found;
    for (const QString &dir : dirs) {
        QStringList installed;

        if (isInsideFlatpak() && isFlatpakRetroarch) {
            // Can't see another Flatpak app's data dir from inside the sandbox;
            // probe the host via flatpak-spawn instead.
            QProcess ls;
            ls.start(QStringLiteral("flatpak-spawn"), {QStringLiteral("--host"), QStringLiteral("ls"), dir});
            ls.waitForFinished(3000);
            if (ls.exitCode() == 0) {
                const QStringList files = QString::fromUtf8(ls.readAllStandardOutput()).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                for (const QString &f : files)
                    installed << f.trimmed();
            }
        } else {
            QDir d(dir);
            installed = d.entryList(QDir::Files);
        }

        for (const QString &core : candidates) {
            if (installed.contains(core)) {
                const QString entry = isFlatpakRetroarch ? core : (dir + QLatin1Char('/') + core);
                if (!found.contains(entry))
                    found << entry;
            }
        }
    }
    return found;
}

static QString shellQuoted(const QString &s)
{
    return QLatin1Char('\'') + QString(s).replace(QLatin1Char('\''), QStringLiteral("'\\''")) + QLatin1Char('\'');
}

QString Launcher::buildRomLaunchCommand(const QVariantMap &rom) const
{
    if (m_retroarchBinary.isEmpty())
        return {};

    QString platformSlug = rom[QStringLiteral("platformSlug")].toString();
    int romId = rom[QStringLiteral("romId")].toInt();
    QString romPath = rom[QStringLiteral("localRomPath")].toString();

    QString corePath = rom[QStringLiteral("customCorePath")].toString();
    if (corePath.isEmpty())
        corePath = m_rommGameCoreMap.value(QString::number(romId)).toString();
    if (corePath.isEmpty())
        corePath = m_rommCoreMap.value(platformSlug).toString();
    if (corePath.isEmpty())
        corePath = autoDetectCore(platformSlug);
    if (corePath.isEmpty())
        corePath = QStringLiteral("<core.so>");
    if (romPath.isEmpty())
        romPath = QStringLiteral("<rom_path>");

    if (m_retroarchBinary == QStringLiteral("flatpak:org.libretro.RetroArch")) {
        if (isInsideFlatpak())
            return QStringLiteral("flatpak-spawn --host flatpak run org.libretro.RetroArch -L ") + shellQuoted(corePath) + QStringLiteral(" --fullscreen ")
                + shellQuoted(romPath);
        return QStringLiteral("flatpak run org.libretro.RetroArch -L ") + shellQuoted(corePath) + QStringLiteral(" --fullscreen ") + shellQuoted(romPath);
    }
    return shellQuoted(m_retroarchBinary) + QStringLiteral(" -L ") + shellQuoted(corePath) + QStringLiteral(" --fullscreen ") + shellQuoted(romPath);
}

void Launcher::copyToClipboard(const QString &text) const
{
    QGuiApplication::clipboard()->setText(text);
}

void Launcher::openExternalUrl(const QString &url) const
{
    QDesktopServices::openUrl(QUrl(url));
}

QString Launcher::detectRetroarchPath() const
{
    if (isInsideFlatpak()) {
        QProcess which;
        which.start(QStringLiteral("flatpak-spawn"), {QStringLiteral("--host"), QStringLiteral("which"), QStringLiteral("retroarch")});
        which.waitForFinished(3000);
        if (which.exitCode() == 0) {
            const QString found = QString::fromUtf8(which.readAllStandardOutput()).trimmed();
            if (!found.isEmpty())
                return found;
        }

        QProcess info;
        info.start(QStringLiteral("flatpak-spawn"),
                   {QStringLiteral("--host"), QStringLiteral("flatpak"), QStringLiteral("info"), QStringLiteral("org.libretro.RetroArch")});
        info.waitForFinished(3000);
        if (info.exitCode() == 0)
            return QStringLiteral("flatpak:org.libretro.RetroArch");

        return {};
    }

    QString found = QStandardPaths::findExecutable(QStringLiteral("retroarch"));
    if (!found.isEmpty())
        return found;

    QProcess check;
    check.start(QStringLiteral("flatpak"), {QStringLiteral("info"), QStringLiteral("org.libretro.RetroArch")});
    check.waitForFinished(3000);
    if (check.exitCode() == 0)
        return QStringLiteral("flatpak:org.libretro.RetroArch");

    return {};
}

void Launcher::launchRom(const QVariantMap &rom, bool enableLogging, const QString &launchOptions, bool autoHdr)
{
    QString romPath = rom[QStringLiteral("localRomPath")].toString();
    QString name = rom[QStringLiteral("name")].toString();
    QString platformSlug = rom[QStringLiteral("platformSlug")].toString();

    if (romPath.isEmpty()) {
        Q_EMIT launchError(name, QStringLiteral("ROM file not downloaded."));
        return;
    }

    int romId = rom[QStringLiteral("romId")].toInt();
    QString corePath = rom[QStringLiteral("customCorePath")].toString();
    if (corePath.isEmpty())
        corePath = m_rommGameCoreMap.value(QString::number(romId)).toString();
    if (corePath.isEmpty())
        corePath = m_rommCoreMap.value(platformSlug).toString();
    if (corePath.isEmpty()) {
        corePath = autoDetectCore(platformSlug);
        if (!corePath.isEmpty()) {
            m_rommCoreMap[platformSlug] = corePath;
            Q_EMIT coreAutoDetected(platformSlug, corePath);
        }
    }
    if (corePath.isEmpty()) {
        Q_EMIT romCoreMissing(platformSlug, rom);
        return;
    }

    if (m_retroarchBinary.isEmpty())
        m_retroarchBinary = detectRetroarchPath();
    if (m_retroarchBinary.isEmpty()) {
        Q_EMIT launchError(name, QStringLiteral("RetroArch not found. Install it or set its path in Settings."));
        return;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QStringList baseFlags = {QStringLiteral("-L"), corePath, QStringLiteral("--fullscreen")};
    if (enableLogging)
        baseFlags << QStringLiteral("-v");
    if (m_retroarchBinary == QStringLiteral("flatpak:org.libretro.RetroArch")) {
        if (isInsideFlatpak())
            launch(QStringLiteral("flatpak-spawn"),
                   QStringList{QStringLiteral("--host"), QStringLiteral("flatpak"), QStringLiteral("run"), QStringLiteral("org.libretro.RetroArch")}
                       + baseFlags,
                   romPath,
                   env,
                   launchOptions,
                   enableLogging,
                   name,
                   true,
                   {},
                   autoHdr);
        else
            launch(QStringLiteral("flatpak"),
                   QStringList{QStringLiteral("run"), QStringLiteral("org.libretro.RetroArch")} + baseFlags,
                   romPath,
                   env,
                   launchOptions,
                   enableLogging,
                   name,
                   true,
                   {},
                   autoHdr);
    } else
        launch(m_retroarchBinary, baseFlags, romPath, env, launchOptions, enableLogging, name, true, {}, autoHdr);
}

void Launcher::setGlobalEnvVars(const QStringList &vars)
{
    m_globalEnvVars = vars;
}

void Launcher::setDefaultRuntimeType(const QString &type)
{
    m_defaultRuntimeType = type;
}

void Launcher::setDefaultProtonPath(const QString &path)
{
    m_defaultProtonPath = path;
}

void Launcher::setDefaultWineBinary(const QString &path)
{
    m_defaultWineBinary = path;
}

QString Launcher::logDir() const
{
    return m_logDir;
}

qint64 Launcher::launch(const QString &binary,
                        const QStringList &baseArgs,
                        const QString &exePath,
                        const QProcessEnvironment &env,
                        const QString &launchOptions,
                        bool enableLogging,
                        const QString &logName,
                        bool appendExe,
                        const QStringList &commandWrappers,
                        bool autoHdr)
{
    if (binary.isEmpty()) {
        Q_EMIT launchError(exePath, QStringLiteral("No runtime is set for this game."));
        return -1;
    }
    const bool binaryFound = binary.contains(QLatin1Char('/')) ? QFileInfo::exists(binary) : !QStandardPaths::findExecutable(binary).isEmpty();
    if (!binaryFound) {
        Q_EMIT launchError(exePath, QStringLiteral("Runtime binary not found: %1").arg(binary));
        return -1;
    }

    auto *proc = new QProcess(this);
    auto *timer = new QElapsedTimer();
    timer->start();
    m_runningProcesses.insert(exePath, proc);
    Q_EMIT runningExePathsChanged();
    connect(proc, &QProcess::finished, this, [this, exePath, proc, timer, enableLogging, autoHdr](int exitCode) {
        m_runningProcesses.remove(exePath);
        Q_EMIT runningExePathsChanged();
        Q_EMIT processFinished(exitCode);
        if (autoHdr && m_hdrEnabled)
            toggleHdr();
        if (exitCode != 0 && !enableLogging && timer->elapsed() < 5000) {
            QString out = QString::fromLocal8Bit(proc->readAllStandardOutput()).trimmed();
            if (!out.isEmpty()) {
                if (out.length() > 400)
                    out = QStringLiteral("...") + out.right(400);
                Q_EMIT launchError(exePath, out);
            }
        }
        if (!exePath.startsWith(QStringLiteral("steam:")))
            finishLaunch(exePath);
        delete timer;
        proc->deleteLater();
    });

    proc->setProcessEnvironment(env);
    proc->setWorkingDirectory(QFileInfo(exePath).absolutePath());

    if (enableLogging) {
        proc->setProcessChannelMode(QProcess::SeparateChannels);
        setupLogging(proc, logName.isEmpty() ? QFileInfo(exePath).baseName() : logName);
    }

    if (!launchOptions.trimmed().isEmpty()) {
        QString baseCmd = shellQuoted(binary);
        for (const auto &a : baseArgs)
            baseCmd += QStringLiteral(" ") + shellQuoted(a);
        if (!exePath.isEmpty() && appendExe)
            baseCmd += QStringLiteral(" ") + shellQuoted(exePath);

        QString opts = launchOptions.trimmed();
        QString fullCmd;
        if (opts.contains(QStringLiteral("%command%")))
            fullCmd = QString(opts).replace(QStringLiteral("%command%"), baseCmd);
        else if (opts.startsWith(QLatin1Char('-')))
            fullCmd = baseCmd + QLatin1Char(' ') + opts;
        else
            fullCmd = opts + QLatin1Char(' ') + baseCmd;

        // Prepend command wrappers to the full command
        if (!commandWrappers.isEmpty()) {
            QString wrappers = commandWrappers.join(QLatin1Char(' '));
            fullCmd = wrappers + QLatin1Char(' ') + fullCmd;
        }

        proc->start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), fullCmd});
    } else {
        QStringList args = baseArgs;
        if (!exePath.isEmpty() && appendExe)
            args << exePath;

        // Prepend command wrappers (gamemoderun, mangohud) to the binary
        if (!commandWrappers.isEmpty()) {
            // Build the full command with wrappers
            QStringList fullArgs = commandWrappers;
            fullArgs << binary << args;
            proc->start(fullArgs.takeFirst(), fullArgs);
        } else {
            proc->start(binary, args);
        }
    }

    if (!proc->waitForStarted(5000)) {
        m_runningProcesses.remove(exePath);
        Q_EMIT runningExePathsChanged();
        Q_EMIT launchError(exePath, proc->errorString());
        delete timer;
        proc->deleteLater();
        return -1;
    } else {
        Q_EMIT launched(exePath);
        qint64 pid = static_cast<qint64>(proc->processId());
        if (!m_watchCandidate.isEmpty())
            trackLaunch(exePath, m_watchCandidate);
        return pid;
    }
}

static QList<qint64> steamReaperPids(int appId)
{
    QList<qint64> pids;
    const QString prefix = QStringLiteral("SteamLaunch AppId=%1").arg(appId);
    const QDir procDir(QStringLiteral("/proc"));
    for (const QString &entry : procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        bool isPid = false;
        const qint64 pid = entry.toLongLong(&isPid);
        if (!isPid)
            continue;
        QFile cmdline(procDir.filePath(entry + QStringLiteral("/cmdline")));
        if (!cmdline.open(QIODevice::ReadOnly))
            continue;
        const QString line = QString::fromUtf8(cmdline.readAll()).replace(QChar(0), QLatin1Char(' '));
        const int idx = line.indexOf(prefix);
        if (idx < 0 || !line.contains(QStringLiteral("reaper")))
            continue;
        const int end = idx + prefix.size();
        if (end == line.size() || line.at(end) == QLatin1Char(' '))
            pids << pid;
    }
    return pids;
}

static QList<qint64> steamClientPids()
{
    QList<qint64> pids;
    const QDir procDir(QStringLiteral("/proc"));
    for (const QString &entry : procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        bool isPid = false;
        const qint64 pid = entry.toLongLong(&isPid);
        if (!isPid)
            continue;
        QFile comm(procDir.filePath(entry + QStringLiteral("/comm")));
        if (comm.open(QIODevice::ReadOnly) && comm.readAll().trimmed() == QByteArrayLiteral("steam"))
            pids << pid;
    }
    return pids;
}

static bool steamClientRunning()
{
    return !steamClientPids().isEmpty();
}

static bool steamFlatpakInstalled()
{
    return QDir(QStringLiteral("/var/lib/flatpak/app/com.valvesoftware.Steam")).exists()
        || QDir(QDir::homePath() + QStringLiteral("/.local/share/flatpak/app/com.valvesoftware.Steam")).exists();
}

static QString steamOverrideMessage(bool steamRunning)
{
    if (steamRunning)
        return QStringLiteral("Steam is running, so prefix commands and environment variables won't work. Run the game without them?");
    if (steamFlatpakInstalled())
        return QStringLiteral("Prefix commands and environment variables don't work with Flatpak Steam. Run the game without them?");
    return QStringLiteral("Prefix commands and environment variables need native Steam. Run the game without them?");
}

static void splitSteamOptions(const QVariantMap &app, QString &preCommand, QString &gameArgs)
{
    const QString opts = app[QStringLiteral("launchOptions")].toString().trimmed();
    const int commandIdx = opts.indexOf(QStringLiteral("%command%"));
    if (commandIdx >= 0) {
        preCommand = opts.left(commandIdx).trimmed();
        gameArgs = opts.mid(commandIdx + 9).trimmed();
    } else if (opts.startsWith(QLatin1Char('-'))) {
        preCommand.clear();
        gameArgs = opts;
    } else {
        preCommand = opts;
        gameArgs.clear();
    }
}

void Launcher::launchSteamUrl(const QVariantMap &steamApp)
{
    const int steamId = steamApp[QStringLiteral("steamAppId")].toInt();
    QString preCommand;
    QString gameArgs;
    splitSteamOptions(steamApp, preCommand, gameArgs);
    if (gameArgs.isEmpty())
        QDesktopServices::openUrl(QUrl(QStringLiteral("steam://rungameid/") + QString::number(steamId)));
    else
        QDesktopServices::openUrl(QUrl(QStringLiteral("steam://run/") + QString::number(steamId) + QStringLiteral("//") + gameArgs + QStringLiteral("/")));
    trackLaunch(QStringLiteral("steam:") + QString::number(steamId), steamApp);
    startSteamPoll();
}

void Launcher::launchSteamWithoutOptions()
{
    if (m_pendingSteamLaunch.isEmpty())
        return;
    const QVariantMap steamApp = m_pendingSteamLaunch;
    m_pendingSteamLaunch.clear();
    launchSteamUrl(steamApp);
}

void Launcher::cancelSteamLaunch()
{
    m_pendingSteamLaunch.clear();
}

qint64 Launcher::startSteamGame(const QVariantMap &app, const QProcessEnvironment &env)
{
    const int steamId = app[QStringLiteral("steamAppId")].toInt();
    if (steamId <= 0)
        return -1;

    QString preCommand;
    QString gameArgs;
    splitSteamOptions(app, preCommand, gameArgs);
    const bool hasPrefix = !preCommand.isEmpty();
    const bool hasEnv = !m_globalEnvVars.isEmpty() || !app[QStringLiteral("envVars")].toStringList().isEmpty();

    QVariantMap steamApp = app;
    steamApp[QStringLiteral("steamStartedMs")] = QDateTime::currentMSecsSinceEpoch();

    if (!hasPrefix && !hasEnv) {
        launchSteamUrl(steamApp);
        return -1;
    }

    const QString name = app[QStringLiteral("name")].toString();
    const bool steamRunning = steamClientRunning();
    if (steamRunning) {
        if (hasPrefix) {
            m_pendingSteamLaunch = steamApp;
            Q_EMIT steamOverrideRequested(name, steamOverrideMessage(true));
            return -1;
        }
        launchSteamUrl(steamApp);
        return -1;
    }

    const QString steamBinary = QStandardPaths::findExecutable(QStringLiteral("steam"));
    if (steamBinary.isEmpty()) {
        m_pendingSteamLaunch = steamApp;
        Q_EMIT steamOverrideRequested(name, steamOverrideMessage(false));
        return -1;
    }

    const QString key = QStringLiteral("steam:") + QString::number(steamId);
    m_watchCandidate = steamApp;
    QStringList wrappers = QProcess::splitCommand(preCommand);
    QProcessEnvironment steamEnv = env;
    while (!wrappers.isEmpty() && wrappers.first().indexOf(QLatin1Char('=')) > 0) {
        const QString assignment = wrappers.takeFirst();
        const int eq = assignment.indexOf(QLatin1Char('='));
        steamEnv.insert(assignment.left(eq), assignment.mid(eq + 1));
    }

    QStringList steamArgs{QStringLiteral("-applaunch"), QString::number(steamId)};
    steamArgs << QProcess::splitCommand(gameArgs);
    const qint64 pid = launch(steamBinary, steamArgs, key, steamEnv, QString(), app[QStringLiteral("enableLogging")].toBool(), name, false, wrappers, false);
    startSteamPoll();
    return pid;
}

qint64 Launcher::launchEntry(const QVariantMap &app)
{
    m_watchCandidate = app;
    auto clearWatchCandidate = qScopeGuard([this] {
        m_watchCandidate.clear();
    });

    QString exePath = app[QStringLiteral("exePath")].toString();
    QString opts = app[QStringLiteral("launchOptions")].toString();
    bool logging = app[QStringLiteral("enableLogging")].toBool();
    QString name = app[QStringLiteral("name")].toString();

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    for (const QString &kv : std::as_const(m_globalEnvVars)) {
        int sep = kv.indexOf(QLatin1Char('='));
        if (sep > 0)
            env.insert(kv.left(sep), kv.mid(sep + 1));
    }

    // Per-game env vars override global ones (not applicable to RetroArch)
    const QString rtForEnvVars = app[QStringLiteral("runtimeType")].toString();
    if (rtForEnvVars != QStringLiteral("retroarch")) {
        const QStringList gameEnvVars = app[QStringLiteral("envVars")].toStringList();
        for (const QString &kv : gameEnvVars) {
            int sep = kv.indexOf(QLatin1Char('='));
            if (sep > 0)
                env.insert(kv.left(sep), kv.mid(sep + 1));
        }
    }

    env.insert(QStringLiteral("VERMOUTH_APP_ID"), app[QStringLiteral("id")].toString());

    // Steam manages its own process, so we can turn HDR on before handing off to it, but
    // have no way to detect the game closing to turn it back off (no launch()/processFinished for it).
    const bool autoHdr = app[QStringLiteral("enableAutoHdr")].toBool();
    if (autoHdr && m_hdrSupported && !m_hdrEnabled)
        toggleHdr();

    if (m_hdrEnabled) {
        env.insert(QStringLiteral("PROTON_ENABLE_HDR"), QStringLiteral("1"));
        env.insert(QStringLiteral("PROTON_ENABLE_WAYLAND"), QStringLiteral("1"));
    }

    if (app[QStringLiteral("enablePreferSdl")].toBool()) {
        env.insert(QStringLiteral("PROTON_PREFER_SDL"), QStringLiteral("1"));
    }
    if (app[QStringLiteral("enableLsfg")].toBool()) {
        env.insert(QStringLiteral("LSFG_LEGACY"), QStringLiteral("1"));
        env.insert(QStringLiteral("LSFGVK_ENV"), QStringLiteral("1"));

        // Set LSFG DLL paths from settings
        QString lsfgDllPath = m_lsfgDllPath;
        if (!lsfgDllPath.isEmpty()) {
            env.insert(QStringLiteral("LSFG_DLL_PATH"), lsfgDllPath);
            env.insert(QStringLiteral("LSFGVK_DLL_PATH"), lsfgDllPath);
        }

        int lsfgMultiplier = app[QStringLiteral("lsfgMultiplier")].toInt();
        if (lsfgMultiplier > 0) {
            env.insert(QStringLiteral("LSFG_MULTIPLIER"), QString::number(lsfgMultiplier));
            env.insert(QStringLiteral("LSFGVK_MULTIPLIER"), QString::number(lsfgMultiplier));
        }

        int lsfgFlowScale = app[QStringLiteral("lsfgFlowScale")].toInt();
        if (lsfgFlowScale > 0) {
            double flowScale = lsfgFlowScale / 100.0;
            env.insert(QStringLiteral("LSFG_FLOW_SCALE"), QString::number(flowScale, 'f', 2));
            env.insert(QStringLiteral("LSFGVK_FLOW_SCALE"), QString::number(flowScale, 'f', 2));
        }

        bool lsfgPerformanceMode = app[QStringLiteral("lsfgPerformanceMode")].toBool();
        env.insert(QStringLiteral("LSFG_PERFORMANCE_MODE"), lsfgPerformanceMode ? QStringLiteral("1") : QStringLiteral("0"));
        env.insert(QStringLiteral("LSFGVK_PERFORMANCE_MODE"), lsfgPerformanceMode ? QStringLiteral("1") : QStringLiteral("0"));

        QString lsfgPresentMode = app[QStringLiteral("lsfgPresentMode")].toString();
        if (!lsfgPresentMode.isEmpty()) {
            env.insert(QStringLiteral("LSFG_EXPERIMENTAL_PRESENT_MODE"), lsfgPresentMode);
        }
    }

    if (!app[QStringLiteral("protonGameId")].toString().isEmpty()) {
        env.insert(QStringLiteral("GAMEID"), app[QStringLiteral("protonGameId")].toString());
    }

    // Build command wrappers (gamemoderun, mangohud) based on settings
    QStringList commandWrappers;
    if (app[QStringLiteral("enableGamemode")].toBool()) {
        QString gamemoderunPath = QStandardPaths::findExecutable(QStringLiteral("gamemoderun"));
        if (!gamemoderunPath.isEmpty()) {
            commandWrappers << gamemoderunPath;
        }
    }
    if (app[QStringLiteral("enableMangohud")].toBool()) {
        QString mangohudPath = QStandardPaths::findExecutable(QStringLiteral("mangohud"));
        if (!mangohudPath.isEmpty()) {
            commandWrappers << mangohudPath;
        }
    }

    QString runtimeType = app[QStringLiteral("runtimeType")].toString();

    // "default" runtime is inferred from settings at launch time - never copied into the entry.
    if (runtimeType == QStringLiteral("default")) {
        if (m_defaultRuntimeType.isEmpty()) {
            Q_EMIT launchError(name, QStringLiteral("No default runtime is set in Settings."));
            return -1;
        }
        QVariantMap resolved = app;
        resolved[QStringLiteral("runtimeType")] = m_defaultRuntimeType;
        // Always follow the current Settings default, even if this entry has a
        // path baked in from a previous save - "default" means it always tracks
        // Settings, not just the first time it was empty.
        resolved[QStringLiteral("protonPath")] = m_defaultProtonPath;
        resolved[QStringLiteral("wineBinary")] = m_defaultWineBinary;
        return launchEntry(resolved);
    }

    if (runtimeType == QStringLiteral("steam"))
        return startSteamGame(app, env);

    if (runtimeType == QStringLiteral("retroarch")) {
        QString platformSlug = app[QStringLiteral("platformSlug")].toString();
        QString customCore = app[QStringLiteral("customCorePath")].toString();
        QVariantMap rom;
        rom[QStringLiteral("localRomPath")] = exePath;
        rom[QStringLiteral("name")] = name;
        rom[QStringLiteral("platformSlug")] = platformSlug;
        rom[QStringLiteral("customCorePath")] = customCore;
        rom[QStringLiteral("romId")] = 0;
        launchRom(rom, logging, opts, autoHdr);
        return -1;
    }

    if (runtimeType == QStringLiteral("uzdoom")) {
        QString binary = app[QStringLiteral("uzdoomPath")].toString();
        if (binary.isEmpty())
            binary = m_uzdoomPath;
        if (binary.isEmpty()) {
            Q_EMIT launchError(name, QStringLiteral("UZDOOM AppImage not set. Download it or pick it in the game settings."));
            return -1;
        }
        QFileInfo fi(binary);
        if (!fi.isExecutable())
            QFile::setPermissions(binary, fi.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther);
        QStringList baseArgs = {QStringLiteral("-iwad"), exePath};
        const QStringList mods = app[QStringLiteral("uzdoomMods")].toStringList();
        for (const QString &mod : mods) {
            baseArgs << QStringLiteral("-file") << mod;
        }
        if (isInsideFlatpak())
            baseArgs.prepend(QStringLiteral("--appimage-extract-and-run"));
        return launch(binary, baseArgs, exePath, env, opts, logging, name, false, commandWrappers, autoHdr);
    }

    if (runtimeType == QStringLiteral("proton")) {
        QString protonPath = app[QStringLiteral("protonPath")].toString();
        if (protonPath.isEmpty()) {
            Q_EMIT launchError(name, QStringLiteral("Proton is not set. Download it or set its path in Settings."));
            return -1;
        }
        QString prefix = app[QStringLiteral("protonPrefix")].toString();
        if (!prefix.isEmpty())
            QDir().mkpath(prefix);

        QString umuBin;
        if (!m_umuPath.isEmpty() && QFileInfo::exists(m_umuPath))
            umuBin = m_umuPath;
        if (umuBin.isEmpty())
            umuBin = QStandardPaths::findExecutable(QStringLiteral("umu-run"));

        if (!umuBin.isEmpty()) {
            if (!QFileInfo::exists(protonPath + QStringLiteral("/proton"))) {
                Q_EMIT launchError(name, QStringLiteral("Proton not found at %1. Download it or set its path in Settings.").arg(protonPath));
                return -1;
            }
            env.insert(QStringLiteral("PROTONPATH"), protonPath);
            env.insert(QStringLiteral("STEAM_COMPAT_DATA_PATH"), prefix);
            const QString protonGameIdForUmu = app[QStringLiteral("protonGameId")].toString();
            env.insert(QStringLiteral("GAMEID"), protonGameIdForUmu.isEmpty() ? QStringLiteral("0") : protonGameIdForUmu);
            env.insert(QStringLiteral("WINEPREFIX"), prefix);
            env.insert(QStringLiteral("UMU_CONTAINER_NSENTER"), QStringLiteral("1"));
            return launch(umuBin, {}, exePath, env, opts, logging, name, true, commandWrappers, autoHdr);
        } else {
            env.insert(QStringLiteral("STEAM_COMPAT_CLIENT_INSTALL_PATH"), QDir::homePath() + QStringLiteral("/.steam/steam"));
            env.insert(QStringLiteral("STEAM_COMPAT_DATA_PATH"), prefix);
            return launch(protonPath + QStringLiteral("/proton"), {QStringLiteral("run")}, exePath, env, opts, logging, name, true, commandWrappers, autoHdr);
        }
    } else if (runtimeType == QStringLiteral("native")) {
        QString binary = exePath;
        QStringList baseArgs;
        if (exePath.endsWith(QStringLiteral(".desktop"), Qt::CaseInsensitive)) {
            QFile desktop(exePath);
            if (desktop.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString section;
                QTextStream in(&desktop);
                while (!in.atEnd()) {
                    QString line = in.readLine().trimmed();
                    if (line.startsWith(QLatin1Char('[')))
                        section = line;
                    if (section == QStringLiteral("[Desktop Entry]") && line.startsWith(QStringLiteral("Exec="))) {
                        QString exec = line.mid(5).trimmed();
                        exec.replace(QRegularExpression(QStringLiteral("\\s*%[fFuUdDnNickvm]")), QString());
                        exec.replace(QStringLiteral("%%"), QStringLiteral("%"));
                        binary = QStringLiteral("/bin/sh");
                        baseArgs = {QStringLiteral("-c"), exec};
                        break;
                    }
                }
            }
        }
        QFileInfo fi(binary);
        if (!exePath.endsWith(QStringLiteral(".desktop"), Qt::CaseInsensitive) && !fi.isExecutable())
            QFile::setPermissions(binary, fi.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther);
        env.insert(QStringLiteral("APPIMAGE"), exePath);
        return launch(binary, baseArgs, exePath, env, opts, logging, name, false, commandWrappers, autoHdr);
    } else {
        QString wineBinary = app[QStringLiteral("wineBinary")].toString();
        if (wineBinary.isEmpty()) {
            Q_EMIT launchError(name, QStringLiteral("Wine is not set. Set its path in Settings."));
            return -1;
        }
        QString prefix = app[QStringLiteral("winePrefix")].toString();
        if (!prefix.isEmpty()) {
            QDir().mkpath(prefix);
            env.insert(QStringLiteral("WINEPREFIX"), prefix);
        }
        return launch(wineBinary, {}, exePath, env, opts, logging, name, true, commandWrappers, autoHdr);
    }
}

static QList<qint64> descendantPids(qint64 root)
{
    QMultiHash<qint64, qint64> children;
    const QDir procDir(QStringLiteral("/proc"));
    for (const QString &entry : procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        bool isPid = false;
        const qint64 pid = entry.toLongLong(&isPid);
        if (!isPid)
            continue;
        QFile stat(procDir.filePath(entry + QStringLiteral("/stat")));
        if (!stat.open(QIODevice::ReadOnly))
            continue;
        const QByteArray line = stat.readAll();
        const QList<QByteArray> fields = line.mid(line.lastIndexOf(')') + 2).split(' ');
        if (fields.size() > 1)
            children.insert(fields.at(1).toLongLong(), pid);
    }

    QList<qint64> result;
    QList<qint64> pending{root};
    while (!pending.isEmpty()) {
        const qint64 pid = pending.takeFirst();
        result << pid;
        pending += children.values(pid);
    }
    return result;
}

static QList<qint64> pidsWithEnvEntry(const QByteArray &entry)
{
    QList<qint64> pids;
    const QDir procDir(QStringLiteral("/proc"));
    for (const QString &dir : procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        bool isPid = false;
        const qint64 pid = dir.toLongLong(&isPid);
        if (!isPid)
            continue;
        QFile environ(procDir.filePath(dir + QStringLiteral("/environ")));
        if (!environ.open(QIODevice::ReadOnly))
            continue;
        if (environ.readAll().split('\0').contains(entry))
            pids << pid;
    }
    return pids;
}

void Launcher::stopEntry(const QVariantMap &app)
{
    QProcess *proc = m_runningProcesses.value(app[QStringLiteral("exePath")].toString(), nullptr);
    if (!proc)
        return;
    if (proc->processId() <= 0) {
        proc->terminate();
        return;
    }

    const QList<qint64> treePids = descendantPids(proc->processId());
    for (qint64 pid : treePids)
        kill(static_cast<pid_t>(pid), SIGTERM);

    const QString appId = app[QStringLiteral("id")].toString();
    if (appId.isEmpty())
        return;
    const QByteArray marker = QByteArrayLiteral("VERMOUTH_APP_ID=") + appId.toUtf8();
    for (qint64 pid : pidsWithEnvEntry(marker))
        kill(static_cast<pid_t>(pid), SIGTERM);
    QTimer::singleShot(5000, this, [treePids, marker] {
        QList<qint64> pids = pidsWithEnvEntry(marker);
        for (qint64 pid : treePids) {
            if (QFile::exists(QStringLiteral("/proc/%1").arg(pid)))
                pids << pid;
        }
        for (qint64 pid : pids)
            kill(static_cast<pid_t>(pid), SIGKILL);
    });
}

void Launcher::stopLaunch(const QString &key)
{
    if (!m_activeLaunches.contains(key))
        return;
    const QVariantMap app = m_activeLaunches.value(key);
    finishLaunch(key);
    if (app[QStringLiteral("runtimeType")].toString() == QStringLiteral("steam"))
        stopSteamGame(app[QStringLiteral("steamAppId")].toInt());
    else
        stopEntry(app);
}

void Launcher::stopSteamGame(int appId)
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("steam://close/") + QString::number(appId)));
    for (qint64 reaper : steamReaperPids(appId)) {
        for (qint64 pid : descendantPids(reaper))
            kill(static_cast<pid_t>(pid), SIGTERM);
    }
    for (qint64 pid : steamClientPids())
        kill(static_cast<pid_t>(pid), SIGTERM);
}

void Launcher::startSteamPoll()
{
    if (!m_steamPoll) {
        m_steamPoll = new QTimer(this);
        m_steamPoll->setInterval(2000);
        connect(m_steamPoll, &QTimer::timeout, this, &Launcher::pollSteamLaunches);
    }
    m_steamPoll->start();
}

void Launcher::pollSteamLaunches()
{
    constexpr qint64 kSteamStartTimeoutMs = 120000;
    constexpr qint64 kSteamGoneGraceMs = 15000;
    bool anySteam = false;
    for (const QString &key : m_activeLaunches.keys()) {
        QVariantMap app = m_activeLaunches.value(key);
        if (app[QStringLiteral("runtimeType")].toString() != QStringLiteral("steam"))
            continue;
        anySteam = true;
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (!steamReaperPids(app[QStringLiteral("steamAppId")].toInt()).isEmpty()) {
            app[QStringLiteral("steamSeen")] = true;
            app[QStringLiteral("steamLastSeenMs")] = now;
            m_activeLaunches.insert(key, app);
            continue;
        }
        const bool seen = app.value(QStringLiteral("steamSeen")).toBool();
        const bool gone = seen && now - app.value(QStringLiteral("steamLastSeenMs")).toLongLong() > kSteamGoneGraceMs;
        const bool timedOut = !seen && now - app.value(QStringLiteral("steamStartedMs")).toLongLong() > kSteamStartTimeoutMs;
        if (gone || timedOut)
            finishLaunch(key);
    }
    if (!anySteam)
        m_steamPoll->stop();
}

void Launcher::trackLaunch(const QString &key, const QVariantMap &app)
{
    m_activeLaunches.insert(key, app);
    Q_EMIT activeLaunchesChanged();
}

void Launcher::finishLaunch(const QString &key)
{
    if (m_activeLaunches.remove(key))
        Q_EMIT activeLaunchesChanged();
}

qint64 Launcher::runInPrefix(const QVariantMap &app, const QString &exePath)
{
    QVariantMap copy = app;
    copy[QStringLiteral("exePath")] = exePath;
    copy[QStringLiteral("enableAutoHdr")] = false;
    return launchEntry(copy);
}

void Launcher::runWinecfg(const QVariantMap &app)
{
    QVariantMap copy = app;
    copy[QStringLiteral("launchOptions")] = QString();
    copy[QStringLiteral("enableLogging")] = false;
    copy[QStringLiteral("exePath")] = QStringLiteral("winecfg");
    copy[QStringLiteral("enableAutoHdr")] = false;
    launchEntry(copy);
}

void Launcher::runRegedit(const QVariantMap &app)
{
    QVariantMap copy = app;
    copy[QStringLiteral("launchOptions")] = QString();
    copy[QStringLiteral("enableLogging")] = false;
    copy[QStringLiteral("exePath")] = QStringLiteral("regedit");
    copy[QStringLiteral("enableAutoHdr")] = false;
    launchEntry(copy);
}

void Launcher::runWinetricks(const QVariantMap &app)
{
    auto *proc = new QProcess(this);
    connect(proc, &QProcess::finished, proc, &QProcess::deleteLater);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString prefix;

    QVariantMap resolved = app;
    if (resolved[QStringLiteral("runtimeType")].toString() == QStringLiteral("default")) {
        resolved[QStringLiteral("runtimeType")] = m_defaultRuntimeType;
        resolved[QStringLiteral("protonPath")] = m_defaultProtonPath;
        resolved[QStringLiteral("wineBinary")] = m_defaultWineBinary;
    }

    if (resolved[QStringLiteral("runtimeType")].toString() == QStringLiteral("proton")) {
        prefix = resolved[QStringLiteral("protonPrefix")].toString();
        QString pfxDir = prefix + QStringLiteral("/pfx");
        if (!QFileInfo::exists(prefix + QStringLiteral("/pfx.lock"))) {
            Q_EMIT prefixNotReady(resolved[QStringLiteral("name")].toString());
            proc->deleteLater();
            return;
        }
        env.insert(QStringLiteral("WINEPREFIX"), pfxDir);
        QString protonPath = resolved[QStringLiteral("protonPath")].toString();
        QString wine64 = protonPath + QStringLiteral("/files/bin/wine64");
        QString wineBin = QFileInfo::exists(wine64) ? wine64 : protonPath + QStringLiteral("/files/bin/wine");
        env.insert(QStringLiteral("WINE"), wineBin);
        env.insert(QStringLiteral("WINESERVER"), protonPath + QStringLiteral("/files/bin/wineserver"));
    } else {
        prefix = resolved[QStringLiteral("winePrefix")].toString();
        env.insert(QStringLiteral("WINE"), resolved[QStringLiteral("wineBinary")].toString());
        env.insert(QStringLiteral("WINEPREFIX"), prefix);
    }

    if (env.value(QStringLiteral("DISPLAY")).isEmpty() && !env.value(QStringLiteral("WAYLAND_DISPLAY")).isEmpty()) {
        env.insert(QStringLiteral("DISPLAY"), QStringLiteral(":0"));
    }

    proc->setProcessEnvironment(env);

    proc->start(QStringLiteral("winetricks"), {QStringLiteral("--gui")});

    if (!proc->waitForStarted(3000)) {
        Q_EMIT launchError(QStringLiteral("winetricks"), proc->errorString());
        proc->deleteLater();
    }
}

qint64 Launcher::runningPidForExe(const QString &exePath) const
{
    QProcess *proc = m_runningProcesses.value(exePath, nullptr);
    if (!proc)
        return -1;
    return static_cast<qint64>(proc->processId());
}

bool Launcher::isWinetricksAvailable() const
{
    return !QStandardPaths::findExecutable(QStringLiteral("winetricks")).isEmpty();
}

bool Launcher::isMangohudAvailable() const
{
    return !QStandardPaths::findExecutable(QStringLiteral("mangohud")).isEmpty();
}

bool Launcher::isGamemodeAvailable() const
{
    return !QStandardPaths::findExecutable(QStringLiteral("gamemoderun")).isEmpty();
}

QString Launcher::autoDetectLsfgDll() const
{
    // Common locations for Lossless Scaling DLL
    const QStringList candidates = {
        QDir::homePath() + QStringLiteral("/.local/share/Steam/steamapps/common/Lossless Scaling/Lossless.dll"),
        QDir::homePath() + QStringLiteral("/.steam/steam/steamapps/common/Lossless Scaling/Lossless.dll"),
        QDir::homePath() + QStringLiteral("/.var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/common/Lossless Scaling/Lossless.dll"),
        QDir::homePath() + QStringLiteral("/.var/app/com.valvesoftware.Steam/data/Steam/steamapps/common/Lossless Scaling/Lossless.dll"),
    };

    for (const QString &path : candidates) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }

    return QString();
}

QStringList Launcher::platformSlugs() const
{
    QStringList slugs = platformCoreMap().keys();
    slugs.sort();
    return slugs;
}

bool Launcher::sleepInhibited() const
{
    return m_inhibitFd >= 0 || !m_inhibitPortalRequestPath.isEmpty();
}

void Launcher::toggleSleepInhibit()
{
    if (sleepInhibited()) {
        if (!m_inhibitPortalRequestPath.isEmpty()) {
            QDBusInterface request(QStringLiteral("org.freedesktop.portal.Desktop"),
                                   m_inhibitPortalRequestPath,
                                   QStringLiteral("org.freedesktop.portal.Request"),
                                   QDBusConnection::sessionBus());
            request.call(QStringLiteral("Close"));
            m_inhibitPortalRequestPath.clear();
        }
        if (m_inhibitFd >= 0) {
            ::close(m_inhibitFd);
            m_inhibitFd = -1;
        }
        Q_EMIT sleepInhibitedChanged();
        return;
    }

    // Try the portal first (works in Flatpak and on any modern desktop with xdg-desktop-portal)
    {
        QDBusInterface portal(QStringLiteral("org.freedesktop.portal.Desktop"),
                              QStringLiteral("/org/freedesktop/portal/desktop"),
                              QStringLiteral("org.freedesktop.portal.Inhibit"),
                              QDBusConnection::sessionBus());

        QDBusMessage portalReply = portal.call(QStringLiteral("Inhibit"),
                                               QStringLiteral(""),
                                               (quint32)(4 | 8), // SUSPEND | IDLE
                                               QVariantMap{{QStringLiteral("reason"), QStringLiteral("User requested sleep inhibition")}});

        if (portalReply.type() == QDBusMessage::ReplyMessage) {
            const QList<QVariant> args = portalReply.arguments();
            if (!args.isEmpty()) {
                m_inhibitPortalRequestPath = args[0].value<QDBusObjectPath>().path();
            }
        }
        if (!m_inhibitPortalRequestPath.isEmpty()) {
            Q_EMIT sleepInhibitedChanged();
            return;
        }
    }

    // Portal unavailable — fall back to logind on the system bus
    QDBusInterface manager(QStringLiteral("org.freedesktop.login1"),
                           QStringLiteral("/org/freedesktop/login1"),
                           QStringLiteral("org.freedesktop.login1.Manager"),
                           QDBusConnection::systemBus());

    QDBusReply<QDBusUnixFileDescriptor> reply = manager.call(QStringLiteral("Inhibit"),
                                                             QStringLiteral("idle:sleep"),
                                                             QStringLiteral("Vermouth"),
                                                             QStringLiteral("User requested sleep inhibition"),
                                                             QStringLiteral("block"));

    if (reply.isValid()) {
        m_inhibitFd = ::dup(reply.value().fileDescriptor());
        Q_EMIT sleepInhibitedChanged();
    }
}

bool Launcher::hdrSupported() const
{
    return m_hdrSupported;
}

bool Launcher::hdrEnabled() const
{
    return m_hdrEnabled;
}

void Launcher::refreshHdrState()
{
    bool supported = false;
    bool hdrEnabled = false;
    bool wcgEnabled = false;

    if (isKde()) {
        QString screenName = currentScreenName();
        QProcess listProc;
        listProc.start(kscreenDoctorBin(), kscreenDoctorArgs({QStringLiteral("-j")}));
        listProc.waitForFinished(3000);
        QJsonDocument doc = QJsonDocument::fromJson(listProc.readAllStandardOutput());
        for (const QJsonValue &val : doc.object()[QStringLiteral("outputs")].toArray()) {
            QJsonObject out = val.toObject();
            if (out[QStringLiteral("name")].toString() == screenName && out[QStringLiteral("connected")].toBool() && out.contains(QStringLiteral("hdr"))) {
                supported = true;
                hdrEnabled = out[QStringLiteral("hdr")].toBool();
                wcgEnabled = out[QStringLiteral("wcg")].toBool();
                break;
            }
        }
    }

    if (m_hdrSupported != supported) {
        m_hdrSupported = supported;
        Q_EMIT hdrSupportedChanged();
    }
    if (m_hdrEnabled != hdrEnabled) {
        m_hdrEnabled = hdrEnabled;
        Q_EMIT hdrEnabledChanged();
    }
    m_wcgEnabled = wcgEnabled;
}

void Launcher::toggleHdr()
{
    bool enable = !m_hdrEnabled;
    QString screenName = currentScreenName();

    if (enable) {
        m_wcgEnabledBeforeHdr = m_wcgEnabled;
        QProcess::execute(kscreenDoctorBin(), kscreenDoctorArgs({QStringLiteral("output.") + screenName + QStringLiteral(".hdr.enable")}));
        QProcess::execute(kscreenDoctorBin(), kscreenDoctorArgs({QStringLiteral("output.") + screenName + QStringLiteral(".wcg.enable")}));
        m_hdrEnabledByUs = true;
    } else {
        QProcess::execute(kscreenDoctorBin(), kscreenDoctorArgs({QStringLiteral("output.") + screenName + QStringLiteral(".hdr.disable")}));
        QString wcgAction = m_wcgEnabledBeforeHdr ? QStringLiteral("wcg.enable") : QStringLiteral("wcg.disable");
        QProcess::execute(kscreenDoctorBin(), kscreenDoctorArgs({QStringLiteral("output.") + screenName + QLatin1Char('.') + wcgAction}));
        m_hdrEnabledByUs = false;
    }

    refreshHdrState();
}

void Launcher::restoreHdrState()
{
    if (!m_hdrEnabledByUs)
        return;
    QString screenName = currentScreenName();
    QProcess::execute(kscreenDoctorBin(), kscreenDoctorArgs({QStringLiteral("output.") + screenName + QStringLiteral(".hdr.disable")}));
    QString wcgAction = m_wcgEnabledBeforeHdr ? QStringLiteral("wcg.enable") : QStringLiteral("wcg.disable");
    QProcess::execute(kscreenDoctorBin(), kscreenDoctorArgs({QStringLiteral("output.") + screenName + QLatin1Char('.') + wcgAction}));
}

void Launcher::setupLogging(QProcess *proc, const QString &name)
{
    QString safeName = name;
    safeName.replace(QRegularExpression(QStringLiteral("[^a-zA-Z0-9_-]")), QStringLiteral("_"));
    QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss"));
    QString logPath = m_logDir + QStringLiteral("/") + safeName + QStringLiteral("_") + timestamp + QStringLiteral(".log");

    auto *logFile = new QFile(logPath, proc);
    if (!logFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
        delete logFile;
        return;
    }

    logFile->write(QStringLiteral("=== Vermouth log: %1 ===\n").arg(name).toUtf8());
    logFile->write(QStringLiteral("=== Started: %1 ===\n\n").arg(QDateTime::currentDateTime().toString()).toUtf8());

    Q_EMIT logSessionStarted(logPath, name);

    connect(proc, &QProcess::readyReadStandardOutput, proc, [this, proc, logFile, logPath]() {
        const QByteArray data = proc->readAllStandardOutput();
        logFile->write(data);
        logFile->flush();
        Q_EMIT logOutput(logPath, QString::fromUtf8(data));
    });

    connect(proc, &QProcess::readyReadStandardError, proc, [this, proc, logFile, logPath]() {
        const QByteArray data = QByteArrayLiteral("[stderr] ") + proc->readAllStandardError();
        logFile->write(data);
        logFile->flush();
        Q_EMIT logOutput(logPath, QString::fromUtf8(data));
    });

    connect(proc, &QProcess::finished, proc, [this, logFile, logPath](int exitCode) {
        logFile->write(QStringLiteral("\n=== Exited with code %1 ===\n").arg(exitCode).toUtf8());
        logFile->close();
        Q_EMIT logSessionFinished(logPath, exitCode);
    });
}
