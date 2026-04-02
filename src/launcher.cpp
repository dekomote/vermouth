#include "launcher.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

Launcher::Launcher(QObject *parent)
    : QObject(parent)
{
    m_logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/logs");
    QDir().mkpath(m_logDir);
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
    connect(proc, &QProcess::finished, proc, &QProcess::deleteLater);
    connect(proc, &QProcess::finished, this, &Launcher::processFinished);

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

    if (app[QStringLiteral("runtimeType")].toString() == QStringLiteral("proton")) {
        QString prefix = app[QStringLiteral("protonPrefix")].toString();
        if (!prefix.isEmpty())
            QDir().mkpath(prefix);
        env.insert(QStringLiteral("STEAM_COMPAT_CLIENT_INSTALL_PATH"), QDir::homePath() + QStringLiteral("/.steam/steam"));
        env.insert(QStringLiteral("STEAM_COMPAT_DATA_PATH"), prefix);
        launch(app[QStringLiteral("protonPath")].toString() + QStringLiteral("/proton"), {QStringLiteral("run")}, exePath, env, opts, logging, name);
    } else {
        QString prefix = app[QStringLiteral("winePrefix")].toString();
        if (!prefix.isEmpty()) {
            QDir().mkpath(prefix);
            env.insert(QStringLiteral("WINEPREFIX"), prefix);
        }
        launch(app[QStringLiteral("wineBinary")].toString(), {}, exePath, env, opts, logging, name);
    }
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
        if (!QFileInfo::exists(pfxDir + QStringLiteral("/pfx.lock"))) {
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
