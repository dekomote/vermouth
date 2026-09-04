#include "launcherdownloader.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>

LauncherDownloader::LauncherDownloader(QObject *parent)
    : Downloader(parent)
{
}

QString LauncherDownloader::downloadDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/launcher-installers");
}

void LauncherDownloader::download(const QString &url, const QString &fileName)
{
    if (busy())
        return;

    QDir().mkpath(downloadDir());
    QString filePath = downloadDir() + QLatin1Char('/') + fileName;

    setBusy(true);
    setStatusText(tr("Downloading launcher…"));
    setProgress(0.0);

    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *reply = nam().get(req);
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0)
            setProgress(static_cast<double>(received) / static_cast<double>(total));
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, filePath]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            setStatusText(tr("Download failed: %1").arg(reply->errorString()));
            setBusy(false);
            Q_EMIT error(reply->errorString());
            return;
        }

        setProgress(1.0);
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            setStatusText(tr("Failed to write %1").arg(filePath));
            setBusy(false);
            Q_EMIT error(QStringLiteral("Could not write ") + filePath);
            return;
        }
        file.write(reply->readAll());
        file.close();

        setBusy(false);
        Q_EMIT finished(filePath);
    });
}