#include "uzdoomdownloader.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>

// Follows the same GitHub flow as ProtonDownloader:
// /releases/latest redirect -> tag name -> deterministic asset URL -> download.
static const char *uzdoomReleaseUrl = "https://github.com/UZDoom/UZDoom/releases/latest";
static const char *uzdoomDownloadUrl = "https://github.com/UZDoom/UZDoom/releases/download/%1/Linux-UZDoom-%1.AppImage";

UzdoomDownloader::UzdoomDownloader(QObject *parent)
    : Downloader(parent)
{
}

void UzdoomDownloader::setInstallPath(const QString &path)
{
    m_installPath = path;
}

QString UzdoomDownloader::installPath() const
{
    return m_installPath;
}

void UzdoomDownloader::downloadLatest()
{
    if (busy())
        return;

    setBusy(true);
    setStatusText(tr("Checking latest release…"));
    setProgress(0.0);

    QNetworkRequest req{QUrl(QLatin1String(uzdoomReleaseUrl))};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    auto *reply = nam().get(req, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReleaseFetched(reply);
    });
}

void UzdoomDownloader::onReleaseFetched(QNetworkReply *reply)
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

    // Already have this exact version installed?
    QString versionFile = m_installPath + QStringLiteral("/.version");
    QFile vf(versionFile);
    if (vf.open(QIODevice::ReadOnly)) {
        if (QString::fromUtf8(vf.readAll()).trimmed() == tagName) {
            vf.close();
            setStatusText(tr("Latest version (%1) is already installed").arg(tagName));
            setBusy(false);
            Q_EMIT finished(m_installPath + QStringLiteral("/UZDOOM.AppImage"));
            return;
        }
        vf.close();
    }

    QString downloadUrl = QString::fromLatin1(uzdoomDownloadUrl).arg(tagName);
    setStatusText(tr("Downloading UZDOOM %1…").arg(tagName));
    startDownload(QUrl(downloadUrl), tagName);
}

void UzdoomDownloader::startDownload(const QUrl &url, const QString &tagName)
{
    m_versionTag = tagName;
    QNetworkRequest dlReq(url);
    dlReq.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
    dlReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *dlReply = nam().get(dlReq, QByteArray());
    connect(dlReply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0)
            setProgress(static_cast<double>(received) / static_cast<double>(total));
    });
    connect(dlReply, &QNetworkReply::finished, this, [this, dlReply]() {
        dlReply->deleteLater();
        if (dlReply->error() != QNetworkReply::NoError) {
            setStatusText(tr("Download failed: %1").arg(dlReply->errorString()));
            setBusy(false);
            Q_EMIT error(dlReply->errorString());
            return;
        }
        setStatusText(tr("Installing…"));
        setProgress(1.0);
        QDir().mkpath(m_installPath);
        QString appImagePath = m_installPath + QStringLiteral("/UZDOOM.AppImage");
        QFile binFile(appImagePath);
        if (!binFile.open(QIODevice::WriteOnly)) {
            setStatusText(tr("Failed to write UZDOOM AppImage"));
            setBusy(false);
            Q_EMIT error(QStringLiteral("Could not write file"));
            return;
        }
        binFile.write(dlReply->readAll());
        binFile.close();
        binFile.setPermissions(binFile.permissions() | QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther);

        QFile vf(m_installPath + QStringLiteral("/.version"));
        if (vf.open(QIODevice::WriteOnly | QIODevice::Truncate))
            vf.write(m_versionTag.toUtf8());

        setStatusText(tr("UZDOOM installed!"));
        setBusy(false);
        Q_EMIT finished(appImagePath);
    });
}
