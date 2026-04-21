#include "launcher.h"
#include <QCursor>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QScreen>
#include <QStandardPaths>
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
}

void Launcher::setUmuPath(const QString &path)
{
    m_umuPath = path;
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

void Launcher::launch(const QString &binary,
                      const QStringList &baseArgs,
                      const QString &exePath,
                      const QProcessEnvironment &env,
                      const QString &launchOptions,
                      bool enableLogging,
                      const QString &logName)
{
    auto *proc = new QProcess(this);
    m_runningProcesses.insert(exePath, proc);
    Q_EMIT runningExePathsChanged();
    connect(proc, &QProcess::finished, this, [this, exePath, proc](int exitCode) {
        m_runningProcesses.remove(exePath);
        Q_EMIT runningExePathsChanged();
        Q_EMIT processFinished(exitCode);
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
        baseCmd += QStringLiteral(" ") + shellQuote(exePath);

        QString opts = launchOptions.trimmed();
        QString fullCmd =
            opts.contains(QStringLiteral("%command%")) ? QString(opts).replace(QStringLiteral("%command%"), baseCmd) : opts + QStringLiteral(" ") + baseCmd;

        proc->start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), fullCmd});
    } else {
        QStringList args = baseArgs;
        args << exePath;
        proc->start(binary, args);
    }

    if (!proc->waitForStarted(5000)) {
        m_runningProcesses.remove(exePath);
        Q_EMIT runningExePathsChanged();
        Q_EMIT launchError(exePath, proc->errorString());
        proc->deleteLater();
    } else {
        Q_EMIT launched(exePath);
    }
}

void Launcher::launchEntry(const QVariantMap &app)
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

    if (app[QStringLiteral("runtimeType")].toString() == QStringLiteral("proton")) {
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
            launch(umoBin, {}, exePath, env, opts, logging, name);
        } else {
            env.insert(QStringLiteral("STEAM_COMPAT_CLIENT_INSTALL_PATH"), QDir::homePath() + QStringLiteral("/.steam/steam"));
            env.insert(QStringLiteral("STEAM_COMPAT_DATA_PATH"), prefix);
            launch(protonPath + QStringLiteral("/proton"), {QStringLiteral("run")}, exePath, env, opts, logging, name);
        }
    } else {
        QString prefix = app[QStringLiteral("winePrefix")].toString();
        if (!prefix.isEmpty()) {
            QDir().mkpath(prefix);
            env.insert(QStringLiteral("WINEPREFIX"), prefix);
        }
        launch(app[QStringLiteral("wineBinary")].toString(), {}, exePath, env, opts, logging, name);
    }
}

void Launcher::stopEntry(const QVariantMap &app)
{
    QProcess *proc = m_runningProcesses.value(app[QStringLiteral("exePath")].toString(), nullptr);
    if (proc)
        proc->terminate();
}

void Launcher::runInPrefix(const QVariantMap &app, const QString &exePath)
{
    QVariantMap copy = app;
    copy[QStringLiteral("exePath")] = exePath;
    launchEntry(copy);
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
        env.insert(QStringLiteral("WINE"), protonPath + QStringLiteral("/files/bin/wine64"));
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

bool Launcher::isWinetricksAvailable() const
{
    return !QStandardPaths::findExecutable(QStringLiteral("winetricks")).isEmpty();
}

bool Launcher::sleepInhibited() const
{
    return m_inhibitFd >= 0;
}

void Launcher::toggleSleepInhibit()
{
    if (sleepInhibited()) {
        ::close(m_inhibitFd);
        m_inhibitFd = -1;
        Q_EMIT sleepInhibitedChanged();
        return;
    }

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
