#include "winedownloader.h"
#include <QDir>
#include <QProcess>
#include <QTemporaryFile>

WineDownloader::WineDownloader(QObject *parent)
    : Downloader(parent)
{
}

void WineDownloader::setLocalWinePath(const QString &path)
{
    m_localWinePath = path;
}

const WineBuildConfig *WineDownloader::findConfig(const QString &buildType)
{
    for (const auto &cfg : wineBuildConfigs) {
        if (buildType == QLatin1String(cfg.key))
            return &cfg;
    }
    return &wineBuildConfigs[0];
}

void WineDownloader::downloadLatest(const QString &buildType)
{
    if (busy())
        return;

    const auto *config = findConfig(buildType);

    setBusy(true);
    setStatusText(tr("Checking latest release…"));
    setProgress(0.0);

    QNetworkRequest req(QUrl(QLatin1String(config->releaseUrl)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    auto *reply = nam().get(req, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply, config]() {
        onReleaseFetched(reply, config);
    });
}

void WineDownloader::onReleaseFetched(QNetworkReply *reply, const WineBuildConfig *config)
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

    QString suffix = QLatin1String(config->suffix);
    QString archiveName = QStringLiteral("wine-%1-%2.tar.xz").arg(tagName, suffix);
    QString expectedDir = QStringLiteral("wine-%1-%2").arg(tagName, suffix);

    QDir localDir(m_localWinePath);
    for (const auto &entry : localDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (entry == expectedDir) {
            setStatusText(tr("Latest version (%1) is already installed").arg(expectedDir));
            setBusy(false);
            Q_EMIT finished(localDir.absoluteFilePath(entry + QStringLiteral("/bin/wine")));
            return;
        }
    }

    QString downloadUrl = QString::fromLatin1(config->downloadUrl).arg(tagName, suffix);

    setStatusText(tr("Downloading %1…").arg(archiveName));

    auto *tmpFile = new QTemporaryFile(QDir::tempPath() + QStringLiteral("/vermouth-wine-XXXXXX.tar.xz"));
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
    connect(dlReply, &QNetworkReply::finished, this, [this, dlReply, tmpFile]() {
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
        startExtraction(tmpFile);
    });
}

void WineDownloader::startExtraction(QTemporaryFile *archiveFile)
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
        QString topDir = firstLine;
        if (topDir.endsWith(QLatin1Char('/')))
            topDir.chop(1);
        QString installedBin = topDir.isEmpty() ? QString() : m_localWinePath + QLatin1Char('/') + topDir + QStringLiteral("/bin/wine");
        Q_EMIT finished(installedBin);
    });

    QDir().mkpath(m_localWinePath);
    m_extractProc->setProgram(QStringLiteral("tar"));
    m_extractProc->setArguments({QStringLiteral("-xf"), archiveFile->fileName(), QStringLiteral("-C"), m_localWinePath, QStringLiteral("--verbose")});
    m_extractProc->start();
}
