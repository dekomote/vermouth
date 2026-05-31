#include "protondownloader.h"
#include <QDir>
#include <QProcess>
#include <QTemporaryFile>

ProtonDownloader::ProtonDownloader(QObject *parent)
    : Downloader(parent)
{
}

void ProtonDownloader::setLocalProtonPath(const QString &path)
{
    m_localProtonPath = path;
}

const ProtonBuildConfig *ProtonDownloader::findConfig(const QString &buildType)
{
    for (const auto &cfg : protonBuildConfigs) {
        if (buildType == QLatin1String(cfg.key))
            return &cfg;
    }
    return &protonBuildConfigs[0];
}

void ProtonDownloader::downloadLatest(const QString &buildType)
{
    if (busy())
        return;

    const auto *config = findConfig(buildType);

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

    QDir localDir(m_localProtonPath);
    for (const auto &entry : localDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (entry.contains(tagName, Qt::CaseInsensitive)) {
            setStatusText(tr("Latest version (%1) is already installed").arg(tagName));
            setBusy(false);
            Q_EMIT finished(localDir.absoluteFilePath(entry));
            return;
        }
    }

    QString archiveName = QString::fromLatin1(config->archivePattern).arg(tagName);
    QString downloadUrl = QString::fromLatin1(config->downloadUrl).arg(tagName, archiveName);
    QStringList tarArgs = {QLatin1String(config->tarFlag), QString(), QStringLiteral("-C"), m_localProtonPath, QStringLiteral("--verbose")};

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
    connect(dlReply, &QNetworkReply::finished, this, [this, dlReply, tmpFile, tarArgs]() {
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
        startExtraction(tmpFile, tarArgs);
    });
}

void ProtonDownloader::startExtraction(QTemporaryFile *archiveFile, const QStringList &tarArgs)
{
    delete m_extractProc;
    m_extractProc = new QProcess(this);
    connect(m_extractProc, &QProcess::finished, this, [this, archiveFile](int exitCode) {
        const QByteArray output = m_extractProc->readAllStandardOutput();
        QString firstLine;
        for (const QByteArray &line : output.split('\n')) {
            QString s = QString::fromUtf8(line).trimmed();
            if (!s.isEmpty() && !s.startsWith(QLatin1Char('.')) && !s.startsWith(QStringLiteral("PaxHeaders"))) {
                firstLine = s;
                break;
            }
        }
        delete archiveFile;
        if (exitCode != 0) {
            setStatusText(tr("Extraction failed"));
            setBusy(false);
            Q_EMIT error(QStringLiteral("tar extraction failed"));
            return;
        }
        setStatusText(tr("Done!"));
        setBusy(false);
        // firstLine is the first entry tar printed, e.g. "GE-Proton9-27/" — strip trailing slash
        QString topDir = firstLine;
        if (topDir.endsWith(QLatin1Char('/')))
            topDir.chop(1);
        QString installedPath = topDir.isEmpty() ? QString() : m_localProtonPath + QLatin1Char('/') + topDir;
        Q_EMIT finished(installedPath);
    });

    QDir().mkpath(m_localProtonPath);
    QStringList args = tarArgs;
    args[1] = archiveFile->fileName();
    m_extractProc->setProgram(QStringLiteral("tar"));
    m_extractProc->setArguments(args);
    m_extractProc->start();
}
