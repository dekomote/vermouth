#include "epicclient.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>

namespace
{
QString keyImageUrl(const QJsonObject &game)
{
    const QJsonArray images = game.value(QStringLiteral("metadata")).toObject().value(QStringLiteral("keyImages")).toArray();
    static const QStringList preferred = {QStringLiteral("DieselGameBoxTall"), QStringLiteral("Thumbnail"), QStringLiteral("DieselGameBox")};
    for (const QString &type : preferred) {
        for (const QJsonValue &v : images) {
            const QJsonObject o = v.toObject();
            if (o.value(QStringLiteral("type")).toString() == type)
                return o.value(QStringLiteral("url")).toString();
        }
    }
    return images.isEmpty() ? QString() : images.first().toObject().value(QStringLiteral("url")).toString();
}

// legendary logs to the same stream, so look for a line that starts a valid JSON document.
QJsonDocument parseJson(const QByteArray &data)
{
    int pos = 0;
    while (pos < data.size()) {
        if (data.at(pos) == '[' || data.at(pos) == '{') {
            QJsonParseError err;
            const QJsonDocument doc = QJsonDocument::fromJson(data.mid(pos), &err);
            if (err.error == QJsonParseError::NoError)
                return doc;
        }
        const int nl = data.indexOf('\n', pos);
        if (nl < 0)
            break;
        pos = nl + 1;
    }
    return {};
}

QJsonArray parseArray(const QByteArray &data)
{
    return parseJson(data).array();
}

QString lastLine(const QByteArray &data)
{
    const QStringList lines = QString::fromUtf8(data).split(QRegularExpression(QStringLiteral("[\r\n]+")), Qt::SkipEmptyParts);
    return lines.isEmpty() ? QString() : lines.last().trimmed();
}
}

EpicClient::EpicClient(QObject *parent)
    : Downloader(parent)
{
    m_legendary = findLegendary();
}

QString EpicClient::findLegendary()
{
    QString path = QStandardPaths::findExecutable(QStringLiteral("legendary"));
    if (path.isEmpty())
        path = QStandardPaths::findExecutable(QStringLiteral("legendary"), {QDir::homePath() + QStringLiteral("/.local/bin")});
    return path;
}

bool EpicClient::available() const
{
    return !m_legendary.isEmpty();
}

bool EpicClient::authenticated() const
{
    return m_authenticated;
}

QString EpicClient::username() const
{
    return m_username;
}

QString EpicClient::installingApp() const
{
    return m_installingApp;
}

void EpicClient::setAuthenticated(bool a)
{
    if (m_authenticated == a)
        return;
    m_authenticated = a;
    Q_EMIT authenticatedChanged();
}

void EpicClient::setUsername(const QString &name)
{
    if (m_username == name)
        return;
    m_username = name;
    Q_EMIT usernameChanged();
}

void EpicClient::detect()
{
    const QString found = findLegendary();
    if (found != m_legendary) {
        m_legendary = found;
        Q_EMIT availableChanged();
    }
    if (available())
        refreshStatus();
}

QProcess *EpicClient::startLegendary(const QStringList &args, const QProcessEnvironment &env)
{
    auto *proc = new QProcess(this);
    proc->setProcessEnvironment(env);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    proc->start(m_legendary, args);
    return proc;
}

void EpicClient::run(const QStringList &args, std::function<void(int, const QByteArray &)> done)
{
    if (!available()) {
        done(-1, QByteArrayLiteral("legendary is not installed"));
        return;
    }
    QProcess *proc = startLegendary(args);
    connect(proc, &QProcess::errorOccurred, this, [proc, done](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) {
            done(-1, proc->errorString().toUtf8());
            proc->deleteLater();
        }
    });
    connect(proc, &QProcess::finished, this, [proc, done](int code, QProcess::ExitStatus status) {
        done(status == QProcess::NormalExit ? code : -1, proc->readAll());
        proc->deleteLater();
    });
}

void EpicClient::refreshStatus()
{
    if (!available())
        return;
    run({QStringLiteral("status"), QStringLiteral("--offline"), QStringLiteral("--json")}, [this](int code, const QByteArray &out) {
        const QJsonObject obj = parseJson(out).object();
        const QString account = obj.value(QStringLiteral("account")).toString();
        const bool loggedIn = code == 0 && !account.isEmpty() && !account.startsWith(QLatin1Char('<'));
        setUsername(loggedIn ? account : QString());
        setAuthenticated(loggedIn);
    });
}

QString EpicClient::loginUrl() const
{
    return QStringLiteral("https://legendary.gl/epiclogin");
}

QString EpicClient::extractCode(const QString &text) const
{
    // Accept the login page's JSON (also as shown by a browser's JSON viewer), the redirect URL or the bare code.
    static const QRegularExpression key(QStringLiteral("authorizationCode\"?\\s*[:=]\\s*\"?([A-Za-z0-9]+)"));
    static const QRegularExpression param(QStringLiteral("[?&]code=([A-Za-z0-9]+)"));
    auto m = key.match(text);
    if (m.hasMatch())
        return m.captured(1);
    m = param.match(text);
    if (m.hasMatch())
        return m.captured(1);
    return text.trimmed().remove(QLatin1Char('"'));
}

void EpicClient::authenticateWithCode(const QString &text)
{
    const QString code = extractCode(text);
    if (code.isEmpty() || busy())
        return;
    setBusy(true);
    setStatusText(tr("Logging in…"));
    run({QStringLiteral("auth"), QStringLiteral("--code"), code}, [this](int exitCode, const QByteArray &out) {
        // Trust the saved session over the exit code, then report what legendary said if there is none.
        run({QStringLiteral("status"), QStringLiteral("--offline"), QStringLiteral("--json")}, [this, exitCode, out](int, const QByteArray &statusOut) {
            const QString account = parseJson(statusOut).object().value(QStringLiteral("account")).toString();
            const bool loggedIn = !account.isEmpty() && !account.startsWith(QLatin1Char('<'));
            setBusy(false);
            setStatusText({});
            if (!loggedIn) {
                const QStringList lines = QString::fromUtf8(out).split(QRegularExpression(QStringLiteral("[\r\n]+")), Qt::SkipEmptyParts);
                const QString detail = lines.mid(qMax<qsizetype>(0, lines.size() - 3)).join(QLatin1Char(' ')).trimmed();
                Q_EMIT loginFailed(detail.isEmpty() ? tr("legendary exited with code %1").arg(exitCode) : detail);
                return;
            }
            setUsername(account);
            setAuthenticated(true);
            Q_EMIT loginSucceeded();
        });
    });
}

void EpicClient::logout()
{
    run({QStringLiteral("auth"), QStringLiteral("--delete")}, [this](int, const QByteArray &) {
        setUsername({});
        setAuthenticated(false);
    });
}

void EpicClient::fetchLibrary()
{
    if (busy())
        return;
    setBusy(true);
    setStatusText(tr("Loading Epic library…"));
    run({QStringLiteral("list"), QStringLiteral("--json")}, [this](int code, const QByteArray &out) {
        const QJsonDocument doc = parseJson(out);
        const QJsonArray arr = doc.array();
        if (code != 0 || !doc.isArray()) {
            setBusy(false);
            setStatusText({});
            Q_EMIT error(lastLine(out));
            return;
        }
        QVariantList games;
        for (const QJsonValue &v : arr) {
            const QJsonObject o = v.toObject();
            QString title = o.value(QStringLiteral("app_title")).toString();
            if (title.isEmpty())
                title = o.value(QStringLiteral("metadata")).toObject().value(QStringLiteral("title")).toString();
            games.append(QVariantMap{{QStringLiteral("appName"), o.value(QStringLiteral("app_name")).toString()},
                                     {QStringLiteral("title"), title},
                                     {QStringLiteral("coverUrl"), keyImageUrl(o)}});
        }
        fetchInstalled(games);
    });
}

void EpicClient::fetchInstalled(const QVariantList &games)
{
    run({QStringLiteral("list-installed"), QStringLiteral("--json")}, [this, games](int, const QByteArray &out) {
        QVariantList installed;
        for (const QJsonValue &v : parseArray(out)) {
            const QJsonObject o = v.toObject();
            installed.append(QVariantMap{{QStringLiteral("appName"), o.value(QStringLiteral("app_name")).toString()},
                                         {QStringLiteral("installPath"), o.value(QStringLiteral("install_path")).toString()},
                                         {QStringLiteral("executable"), o.value(QStringLiteral("executable")).toString()},
                                         {QStringLiteral("version"), o.value(QStringLiteral("version")).toString()}});
        }
        setBusy(false);
        setStatusText({});
        Q_EMIT libraryFetched(games, installed);
        if (!installed.isEmpty())
            checkUpdates();
    });
}

void EpicClient::install(const QString &appName, const QString &installDir)
{
    if (!available() || m_installProc)
        return;
    QDir().mkpath(installDir);
    startInstallProcess(appName,
                        {QStringLiteral("install"), appName, QStringLiteral("--base-path"), installDir, QStringLiteral("-y"), QStringLiteral("--skip-sdl")});
}

void EpicClient::update(const QString &appName)
{
    if (!available() || m_installProc)
        return;
    startInstallProcess(appName, {QStringLiteral("update"), appName, QStringLiteral("-y"), QStringLiteral("--skip-sdl")});
}

void EpicClient::startInstallProcess(const QString &appName, const QStringList &args)
{
    m_installCancelled = false;
    m_installingApp = appName;
    Q_EMIT installingAppChanged();
    setProgress(0.0);
    setStatusText(tr("Starting download…"));

    m_installProc = startLegendary(args);
    auto *buffer = new QByteArray;
    connect(m_installProc, &QProcess::readyRead, this, [this, buffer]() {
        buffer->append(m_installProc->readAll());
        static const QRegularExpression progress(QStringLiteral("Progress:\\s*([0-9.]+)%"));
        auto it = progress.globalMatch(QString::fromUtf8(*buffer));
        QString last;
        while (it.hasNext())
            last = it.next().captured(1);
        if (!last.isEmpty()) {
            setProgress(last.toDouble() / 100.0);
            setStatusText(tr("Downloading %1%").arg(qRound(last.toDouble())));
        }
        if (buffer->size() > 20000)
            *buffer = buffer->right(8000);
    });
    connect(m_installProc, &QProcess::errorOccurred, this, [this, buffer](QProcess::ProcessError e) {
        if (e != QProcess::FailedToStart)
            return;
        delete buffer;
        const QString app = m_installingApp;
        m_installProc->deleteLater();
        m_installProc = nullptr;
        m_installingApp.clear();
        Q_EMIT installingAppChanged();
        Q_EMIT installFinished(app, false, tr("Could not start legendary"));
    });
    connect(m_installProc, &QProcess::finished, this, [this, buffer](int code, QProcess::ExitStatus status) {
        const QString app = m_installingApp;
        const bool ok = status == QProcess::NormalExit && code == 0;
        const QString message = m_installCancelled ? tr("Cancelled") : lastLine(*buffer);
        delete buffer;
        m_installProc->deleteLater();
        m_installProc = nullptr;
        m_installingApp.clear();
        setStatusText({});
        Q_EMIT installingAppChanged();
        Q_EMIT installFinished(app, ok, message);
    });
}

void EpicClient::checkUpdates()
{
    if (!available())
        return;
    // The tab separated list has an "Update available" column for each installed game.
    run({QStringLiteral("list-installed"), QStringLiteral("--check-updates"), QStringLiteral("--tsv")}, [this](int code, const QByteArray &out) {
        if (code != 0)
            return;
        QStringList outdated;
        const QStringList lines = QString::fromUtf8(out).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            const QStringList cols = line.split(QLatin1Char('\t'));
            if (cols.size() >= 8 && cols.at(0) != QLatin1String("App name") && cols.at(4).trimmed() == QLatin1String("True"))
                outdated << cols.at(0);
        }
        Q_EMIT updatesChecked(outdated);
    });
}

void EpicClient::cancelInstall()
{
    if (!m_installProc)
        return;
    m_installCancelled = true;
    m_installProc->terminate();
}

void EpicClient::uninstall(const QString &appName)
{
    run({QStringLiteral("uninstall"), appName, QStringLiteral("-y")}, [this, appName](int code, const QByteArray &out) {
        if (code != 0) {
            Q_EMIT error(lastLine(out));
            return;
        }
        Q_EMIT uninstalled(appName);
    });
}
