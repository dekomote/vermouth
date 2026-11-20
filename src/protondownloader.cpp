#include "protondownloader.h"
#include <QDir>
#include <QFile>
#include <QFutureWatcher>
#include <QProcess>
#include <QTemporaryFile>
#include <QtConcurrent>

static const QString releaseMarkerName()
{
    return QStringLiteral(".vermouth-release");
}

ProtonDownloader::ProtonDownloader(QObject *parent)
    : Downloader(parent)
{
}

void ProtonDownloader::setLocalProtonPath(const QString &path)
{
    m_localProtonPath = path;
}

const ProtonBuildConfig *ProtonDownloader::configForKey(const QString &key)
{
    for (const auto &cfg : protonBuildConfigs) {
        if (key == QLatin1String(cfg.key))
            return &cfg;
    }
    return nullptr;
}

const ProtonBuildConfig *ProtonDownloader::configForFolder(const QString &folderName)
{
    for (const auto &cfg : protonBuildConfigs) {
        if (folderName == QLatin1String(cfg.latestName))
            return &cfg;
    }
    struct Prefix {
        const char *prefix;
        const char *key;
    };
    static const Prefix prefixes[] =
        {{"GE-Proton", "ge"}, {"proton-cachyos", "cachyos"}, {"proton-em", "em"}, {"umu-proton", "umu"}, {"dwproton", "dw"}, {"proton-wineland", "wineland"}};
    for (const auto &p : prefixes) {
        if (folderName.startsWith(QLatin1String(p.prefix), Qt::CaseInsensitive))
            return configForKey(QLatin1String(p.key));
    }
    return nullptr;
}

QString ProtonDownloader::installedTag(const QString &installDir)
{
    QFile f(installDir + QLatin1Char('/') + releaseMarkerName());
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(f.readAll()).trimmed();
}

void ProtonDownloader::writeInstalledTag(const QString &installDir, const QString &tag)
{
    QFile f(installDir + QLatin1Char('/') + releaseMarkerName());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(tag.toUtf8());
}

void ProtonDownloader::downloadLatest(const QString &buildType)
{
    if (busy())
        return;

    const auto *config = configForKey(buildType);
    if (!config)
        config = &protonBuildConfigs[0];

    setBusy(true);
    setStatusText(tr("Checking latest release…"));
    setProgress(0.0);

    QString releaseUrl = QLatin1String(config->releaseUrl);
    QNetworkRequest req{QUrl(releaseUrl)};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    auto *reply = nam().get(req, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply, config]() {
        onReleaseFetched(reply, config);
    });
}

void ProtonDownloader::onReleaseFetched(QNetworkReply *reply, const ProtonBuildConfig *config)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        setStatusText(tr("Failed to fetch release info: %1").arg(reply->errorString()));
        setBusy(false);
        Q_EMIT error(reply->errorString());
        return;
    }

    QUrl redirectUrl = reply->header(QNetworkRequest::LocationHeader).toUrl();
    QString path = redirectUrl.path();
    QString tagName = path.mid(path.lastIndexOf(QLatin1Char('/')) + 1);

    if (tagName.isEmpty()) {
        setStatusText(tr("Could not determine latest version"));
        setBusy(false);
        Q_EMIT error(QStringLiteral("No tag found in redirect URL"));
        return;
    }

    const QString latestDir = m_localProtonPath + QLatin1Char('/') + QLatin1String(config->latestName);
    if (installedTag(latestDir) == tagName) {
        setStatusText(tr("Latest version (%1) is already installed").arg(tagName));
        setBusy(false);
        Q_EMIT finished(latestDir);
        return;
    }

    QString archiveName = QString::fromLatin1(config->archivePattern).arg(tagName);
    QString downloadUrl = QString::fromLatin1(config->downloadUrl).arg(tagName, archiveName);
    const QString stagingDir = m_localProtonPath + QStringLiteral("/.staging");
    QStringList tarArgs = {QLatin1String(config->tarFlag), QString(), QStringLiteral("-C"), stagingDir};

    setStatusText(tr("Downloading %1…").arg(tagName));

    auto *tmpFile = new QTemporaryFile(QDir::tempPath() + QStringLiteral("/vermouth-proton-XXXXXX") + QLatin1String(config->tmpExt));
    if (!tmpFile->open()) {
        setStatusText(tr("Failed to create temp file"));
        setBusy(false);
        Q_EMIT error(QStringLiteral("Could not create temporary file"));
        delete tmpFile;
        return;
    }

    QUrl dlUrl(downloadUrl);
    QNetworkRequest dlReq(dlUrl);
    dlReq.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
    dlReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *dlReply = nam().get(dlReq, QByteArray());
    connect(dlReply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0)
            setProgress(static_cast<double>(received) / static_cast<double>(total));
    });
    connect(dlReply, &QNetworkReply::readyRead, this, [dlReply, tmpFile]() {
        tmpFile->write(dlReply->readAll());
    });
    connect(dlReply, &QNetworkReply::finished, this, [this, dlReply, tmpFile, tarArgs, config, tagName]() {
        tmpFile->flush();
        tmpFile->close();
        if (dlReply->error() != QNetworkReply::NoError) {
            setStatusText(tr("Download failed"));
            setBusy(false);
            Q_EMIT error(dlReply->errorString());
            delete tmpFile;
            dlReply->deleteLater();
            return;
        }
        setStatusText(tr("Extracting…"));
        setProgress(1.0);
        dlReply->deleteLater();
        startExtraction(tmpFile, tarArgs, config, tagName);
    });
}

// Replaces the previous "Latest" folder with the freshly extracted build, so games pointing at it keep working.
static QString installExtracted(const QString &stagingDir, const QString &localDir, const QString &latestName, const QString &tag)
{
    const QStringList dirs = QDir(stagingDir).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (dirs.isEmpty())
        return {};

    const QString latestDir = localDir + QLatin1Char('/') + latestName;
    const QString oldDir = localDir + QStringLiteral("/.") + latestName + QStringLiteral(".old");
    QDir(oldDir).removeRecursively();
    const bool hadOld = QDir(latestDir).exists();
    if (hadOld && !QDir().rename(latestDir, oldDir))
        return {};
    if (!QDir().rename(stagingDir + QLatin1Char('/') + dirs.first(), latestDir)) {
        if (hadOld)
            QDir().rename(oldDir, latestDir);
        return {};
    }
    ProtonDownloader::writeInstalledTag(latestDir, tag);
    QDir(oldDir).removeRecursively();
    QDir(stagingDir).removeRecursively();
    return latestDir;
}

void ProtonDownloader::startExtraction(QTemporaryFile *archiveFile, const QStringList &tarArgs, const ProtonBuildConfig *config, const QString &tag)
{
    delete m_extractProc;
    m_extractProc = new QProcess(this);
    connect(m_extractProc, &QProcess::finished, this, [this, archiveFile, config, tag](int exitCode) {
        delete archiveFile;
        if (exitCode != 0) {
            setStatusText(tr("Extraction failed"));
            setBusy(false);
            Q_EMIT error(QStringLiteral("tar extraction failed"));
            return;
        }
        setStatusText(tr("Installing…"));
        const QString stagingDir = m_localProtonPath + QStringLiteral("/.staging");
        const QString localDir = m_localProtonPath;
        const QString latestName = QString::fromLatin1(config->latestName);
        auto *watcher = new QFutureWatcher<QString>(this);
        connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher]() {
            const QString installedPath = watcher->result();
            watcher->deleteLater();
            if (installedPath.isEmpty()) {
                setStatusText(tr("Installing failed"));
                setBusy(false);
                Q_EMIT error(QStringLiteral("Could not install the extracted build"));
                return;
            }
            setStatusText(tr("Done!"));
            setBusy(false);
            Q_EMIT finished(installedPath);
        });
        watcher->setFuture(QtConcurrent::run(installExtracted, stagingDir, localDir, latestName, tag));
    });

    const QString stagingDir = m_localProtonPath + QStringLiteral("/.staging");
    QDir(stagingDir).removeRecursively();
    QDir().mkpath(stagingDir);
    QStringList args = tarArgs;
    args[1] = archiveFile->fileName();
    m_extractProc->setProgram(QStringLiteral("tar"));
    m_extractProc->setArguments(args);
    m_extractProc->start();
}
