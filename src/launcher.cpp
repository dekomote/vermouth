#include "launcher.h"
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
#include <QRegularExpression>
#include <QScreen>
#include <QStandardPaths>
#include <QUrl>
#include <unistd.h>

static bool isKde()
{
    return qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains(QLatin1String("KDE"), Qt::CaseInsensitive);
}

static bool isInsideFlatpak()
{
    return QFileInfo::exists(QStringLiteral("/.flatpak-info"));
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

void Launcher::setRommCoreMap(const QVariantMap &map)
{
    m_rommCoreMap = map;
}

void Launcher::setRommGameCoreMap(const QVariantMap &map)
{
    m_rommGameCoreMap = map;
}

QString Launcher::resolveRetroarchBinary() const
{
    if (!m_retroarchPath.isEmpty() && QFileInfo::exists(m_retroarchPath))
        return m_retroarchPath;

    QString found = QStandardPaths::findExecutable(QStringLiteral("retroarch"));
    if (!found.isEmpty())
        return found;

    // Check if RetroArch is installed as a flatpak
    QProcess check;
    check.start(QStringLiteral("flatpak"), {QStringLiteral("info"), QStringLiteral("org.libretro.RetroArch")});
    check.waitForFinished(3000);
    if (check.exitCode() == 0)
        return QStringLiteral("flatpak:org.libretro.RetroArch");

    return {};
}

void Launcher::cacheRetroarchBinary()
{
    if (!m_retroarchPath.isEmpty() && QFileInfo::exists(m_retroarchPath)) {
        m_retroarchBinary = m_retroarchPath;
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
    QStringList candidates = platformCoreMap().value(platformSlug);
    if (candidates.isEmpty())
        return {};
    for (const QString &dir : retroarchCoreDirs(m_retroarchBinary)) {
        for (const QString &core : candidates) {
            QString path = dir + QLatin1Char('/') + core;
            if (QFileInfo::exists(path))
                return path;
        }
    }
    return {};
}

QStringList Launcher::availableCoresForPlatform(const QString &platformSlug) const
{
    QStringList candidates = platformCoreMap().value(platformSlug);
    QStringList found;
    for (const QString &dir : retroarchCoreDirs(m_retroarchBinary)) {
        for (const QString &core : candidates) {
            QString path = dir + QLatin1Char('/') + core;
            if (QFileInfo::exists(path) && !found.contains(path))
                found << path;
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

    if (m_retroarchBinary == QStringLiteral("flatpak:org.libretro.RetroArch"))
        return QStringLiteral("flatpak run org.libretro.RetroArch -L ") + shellQuoted(corePath) + QStringLiteral(" --fullscreen ") + shellQuoted(romPath);
    return shellQuoted(m_retroarchBinary) + QStringLiteral(" -L ") + shellQuoted(corePath) + QStringLiteral(" --fullscreen ") + shellQuoted(romPath);
}

void Launcher::copyToClipboard(const QString &text) const
{
    QGuiApplication::clipboard()->setText(text);
}

QString Launcher::detectRetroarchPath() const
{
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

void Launcher::launchRom(const QVariantMap &rom, bool enableLogging, const QString &launchOptions)
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

    if (m_retroarchBinary.isEmpty()) {
        Q_EMIT launchError(name, QStringLiteral("RetroArch not found. Install it or set its path in Settings."));
        return;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QStringList baseFlags = {QStringLiteral("-L"), corePath, QStringLiteral("--fullscreen")};
    if (enableLogging)
        baseFlags << QStringLiteral("-v");
    if (m_retroarchBinary == QStringLiteral("flatpak:org.libretro.RetroArch"))
        launch(QStringLiteral("flatpak"),
               QStringList{QStringLiteral("run"), QStringLiteral("org.libretro.RetroArch")} + baseFlags,
               romPath,
               env,
               launchOptions,
               enableLogging,
               name);
    else
        launch(m_retroarchBinary, baseFlags, romPath, env, launchOptions, enableLogging, name);
}

void Launcher::setGlobalEnvVars(const QStringList &vars)
{
    m_globalEnvVars = vars;
}

QString Launcher::logDir() const
{
    return m_logDir;
}

static QString shellQuote(const QString &s)
{
    QString quoted = s;
    quoted.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + quoted + QLatin1Char('\'');
}

qint64 Launcher::launch(const QString &binary,
                        const QStringList &baseArgs,
                        const QString &exePath,
                        const QProcessEnvironment &env,
                        const QString &launchOptions,
                        bool enableLogging,
                        const QString &logName,
                        bool appendExe)
{
    auto *proc = new QProcess(this);
    auto *timer = new QElapsedTimer();
    timer->start();
    m_runningProcesses.insert(exePath, proc);
    Q_EMIT runningExePathsChanged();
    connect(proc, &QProcess::finished, this, [this, exePath, proc, timer, enableLogging](int exitCode) {
        m_runningProcesses.remove(exePath);
        Q_EMIT runningExePathsChanged();
        Q_EMIT processFinished(exitCode);
        if (exitCode != 0 && !enableLogging && timer->elapsed() < 5000) {
            QString out = QString::fromLocal8Bit(proc->readAllStandardOutput()).trimmed();
            if (!out.isEmpty()) {
                if (out.length() > 400)
                    out = QStringLiteral("...") + out.right(400);
                Q_EMIT launchError(exePath, out);
            }
        }
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
        QString baseCmd = shellQuote(binary);
        for (const auto &a : baseArgs)
            baseCmd += QStringLiteral(" ") + shellQuote(a);
        if (!exePath.isEmpty() && appendExe)
            baseCmd += QStringLiteral(" ") + shellQuote(exePath);

        QString opts = launchOptions.trimmed();
        QString fullCmd;
        if (opts.contains(QStringLiteral("%command%")))
            fullCmd = QString(opts).replace(QStringLiteral("%command%"), baseCmd);
        else if (opts.startsWith(QLatin1Char('-')))
            fullCmd = baseCmd + QLatin1Char(' ') + opts;
        else
            fullCmd = opts + QLatin1Char(' ') + baseCmd;

        proc->start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), fullCmd});
    } else {
        QStringList args = baseArgs;
        if (!exePath.isEmpty() && appendExe)
            args << exePath;
        proc->start(binary, args);
    }

    if (!proc->waitForStarted(5000)) {
        m_runningProcesses.remove(exePath);
        Q_EMIT runningExePathsChanged();
        Q_EMIT launchError(exePath, proc->errorString());
        proc->deleteLater();
        return -1;
    } else {
        Q_EMIT launched(exePath);
        qint64 pid = static_cast<qint64>(proc->processId());
        return pid;
    }
}

qint64 Launcher::launchEntry(const QVariantMap &app)
{
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

    if (m_hdrEnabled) {
        env.insert(QStringLiteral("PROTON_ENABLE_HDR"), QStringLiteral("1"));
        env.insert(QStringLiteral("PROTON_ENABLE_WAYLAND"), QStringLiteral("1"));
    }

    QString runtimeType = app[QStringLiteral("runtimeType")].toString();

    if (runtimeType == QStringLiteral("steam")) {
        int steamId = app[QStringLiteral("steamAppId")].toInt();
        if (steamId > 0)
            QDesktopServices::openUrl(QUrl(QStringLiteral("steam://rungameid/") + QString::number(steamId)));
        return -1;
    }

    if (runtimeType == QStringLiteral("retroarch")) {
        QString platformSlug = app[QStringLiteral("platformSlug")].toString();
        QString customCore = app[QStringLiteral("customCorePath")].toString();
        QVariantMap rom;
        rom[QStringLiteral("localRomPath")] = exePath;
        rom[QStringLiteral("name")] = name;
        rom[QStringLiteral("platformSlug")] = platformSlug;
        rom[QStringLiteral("customCorePath")] = customCore;
        rom[QStringLiteral("romId")] = 0;
        launchRom(rom, logging, opts);
        return -1;
    }

    if (runtimeType == QStringLiteral("proton")) {
        QString protonPath = app[QStringLiteral("protonPath")].toString();
        QString prefix = app[QStringLiteral("protonPrefix")].toString();
        if (!prefix.isEmpty())
            QDir().mkpath(prefix);

        QString umoBin = m_umuPath;
        if (umoBin.isEmpty())
            umoBin = QStandardPaths::findExecutable(QStringLiteral("umu-run"));

        if (!umoBin.isEmpty()) {
            env.insert(QStringLiteral("PROTONPATH"), protonPath);
            env.insert(QStringLiteral("STEAM_COMPAT_DATA_PATH"), prefix);
            env.insert(QStringLiteral("GAMEID"), QStringLiteral("0"));
            env.insert(QStringLiteral("WINEPREFIX"), prefix);
            return launch(umoBin, {}, exePath, env, opts, logging, name);
        } else {
            env.insert(QStringLiteral("STEAM_COMPAT_CLIENT_INSTALL_PATH"), QDir::homePath() + QStringLiteral("/.steam/steam"));
            env.insert(QStringLiteral("STEAM_COMPAT_DATA_PATH"), prefix);
            return launch(protonPath + QStringLiteral("/proton"), {QStringLiteral("run")}, exePath, env, opts, logging, name);
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
        return launch(binary, baseArgs, exePath, env, opts, logging, name, false);
    } else {
        QString prefix = app[QStringLiteral("winePrefix")].toString();
        if (!prefix.isEmpty()) {
            QDir().mkpath(prefix);
            env.insert(QStringLiteral("WINEPREFIX"), prefix);
        }
        return launch(app[QStringLiteral("wineBinary")].toString(), {}, exePath, env, opts, logging, name);
    }
    return -1;
}

void Launcher::stopEntry(const QVariantMap &app)
{
    QProcess *proc = m_runningProcesses.value(app[QStringLiteral("exePath")].toString(), nullptr);
    if (proc)
        proc->terminate();
}

qint64 Launcher::runInPrefix(const QVariantMap &app, const QString &exePath)
{
    QVariantMap copy = app;
    copy[QStringLiteral("exePath")] = exePath;
    return launchEntry(copy);
}

void Launcher::runWinecfg(const QVariantMap &app)
{
    QVariantMap copy = app;
    copy[QStringLiteral("launchOptions")] = QString();
    copy[QStringLiteral("enableLogging")] = false;
    copy[QStringLiteral("exePath")] = QStringLiteral("winecfg");
    launchEntry(copy);
}

void Launcher::runRegedit(const QVariantMap &app)
{
    QVariantMap copy = app;
    copy[QStringLiteral("launchOptions")] = QString();
    copy[QStringLiteral("enableLogging")] = false;
    copy[QStringLiteral("exePath")] = QStringLiteral("regedit");
    launchEntry(copy);
}

void Launcher::runWinetricks(const QVariantMap &app)
{
    auto *proc = new QProcess(this);
    connect(proc, &QProcess::finished, proc, &QProcess::deleteLater);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString prefix;

    if (app[QStringLiteral("runtimeType")].toString() == QStringLiteral("proton")) {
        prefix = app[QStringLiteral("protonPrefix")].toString();
        QString pfxDir = prefix + QStringLiteral("/pfx");
        if (!QFileInfo::exists(prefix + QStringLiteral("/pfx.lock"))) {
            Q_EMIT prefixNotReady(app[QStringLiteral("name")].toString());
            proc->deleteLater();
            return;
        }
        QString protonPath = app[QStringLiteral("protonPath")].toString();
        env.insert(QStringLiteral("WINEPREFIX"), pfxDir);
        QString wine64 = protonPath + QStringLiteral("/files/bin/wine64");
        QString wineBin = QFileInfo::exists(wine64) ? wine64 : protonPath + QStringLiteral("/files/bin/wine");
        env.insert(QStringLiteral("WINE"), wineBin);
        env.insert(QStringLiteral("WINESERVER"), protonPath + QStringLiteral("/files/bin/wineserver"));
    } else {
        prefix = app[QStringLiteral("winePrefix")].toString();
        env.insert(QStringLiteral("WINEPREFIX"), prefix);
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

    connect(proc, &QProcess::readyReadStandardOutput, proc, [proc, logFile]() {
        logFile->write(proc->readAllStandardOutput());
        logFile->flush();
    });

    connect(proc, &QProcess::readyReadStandardError, proc, [proc, logFile]() {
        logFile->write(QByteArrayLiteral("[stderr] "));
        logFile->write(proc->readAllStandardError());
        logFile->flush();
    });

    connect(proc, &QProcess::finished, proc, [logFile](int exitCode) {
        logFile->write(QStringLiteral("\n=== Exited with code %1 ===\n").arg(exitCode).toUtf8());
        logFile->close();
    });
}
