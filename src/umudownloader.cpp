#include "umudownloader.h"
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryFile>

UmuDownloader::UmuDownloader(QObject *parent)
    : QObject(parent)
{
    m_statusClearTimer.setSingleShot(true);
    m_statusClearTimer.setInterval(6000);
    connect(&m_statusClearTimer, &QTimer::timeout, this, [this]() {
        setStatusText(QString());
    });
}

void UmuDownloader::setInstallPath(const QString &path)
{
    m_installPath = path;
}

bool UmuDownloader::busy() const
{
    return m_busy;
}
QString UmuDownloader::statusText() const
{
    return m_statusText;
}
double UmuDownloader::progress() const
{
    return m_progress;
}

void UmuDownloader::setBusy(bool busy)
{
    if (m_busy != busy) {
        m_busy = busy;
        Q_EMIT busyChanged();
        if (!busy)
            m_statusClearTimer.start();
        else
            m_statusClearTimer.stop();
    }
}

void UmuDownloader::setStatusText(const QString &text)
{
    if (m_statusText != text) {
        m_statusText = text;
        Q_EMIT statusTextChanged();
    }
}

void UmuDownloader::setProgress(double progress)
{
    if (m_progress != progress) {
        m_progress = progress;
        Q_EMIT progressChanged();
    }
}

void UmuDownloader::downloadLatest()
{
    if (m_busy)
        return;

    setBusy(true);
    setStatusText(tr("Checking latest umu-launcher release…"));
    setProgress(0.0);

    // Use the redirect from /releases/latest to discover the tag, same as ProtonDownloader
    QNetworkRequest req(QUrl(QStringLiteral("https://github.com/Open-Wine-Components/umu-launcher/releases/latest")));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReleaseFetched(reply);
    });
}

void UmuDownloader::onReleaseFetched(QNetworkReply *reply)
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

    // Try standalone binary first, then versioned tarball as fallback
    // umu-launcher releases a standalone umu-run binary at:
    // releases/download/{tag}/umu-run
    QString downloadUrl = QStringLiteral("https://github.com/Open-Wine-Components/umu-launcher/releases/download/%1/umu-run").arg(tagName);
    QString assetName = QStringLiteral("umu-run");

    setStatusText(tr("Downloading umu-launcher %1…").arg(tagName));

    QNetworkRequest dlReq{QUrl(downloadUrl)};
    dlReq.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
    dlReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *dlReply = m_nam.get(dlReq);
    connect(dlReply, &QNetworkReply::downloadProgress, this, &UmuDownloader::onDownloadProgress);
    connect(dlReply, &QNetworkReply::finished, this, [this, dlReply, tagName, assetName]() {
        // If the binary URL 404s, fall back to the versioned tarball
        if (dlReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404) {
            dlReply->deleteLater();
            QString tarUrl =
                QStringLiteral("https://github.com/Open-Wine-Components/umu-launcher/releases/download/%1/umu-launcher-%1-zipapp.tar").arg(tagName);
            QString tarName = QStringLiteral("umu-launcher-%1-zipapp.tar").arg(tagName);
            QNetworkRequest tarReq{QUrl(tarUrl)};
            tarReq.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
            tarReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
            auto *tarReply = m_nam.get(tarReq);
            connect(tarReply, &QNetworkReply::downloadProgress, this, &UmuDownloader::onDownloadProgress);
            connect(tarReply, &QNetworkReply::finished, this, [this, tarReply, tarName]() {
                onDownloadFinished(tarReply, tarName);
            });
            return;
        }
        onDownloadFinished(dlReply, assetName);
    });
}

void UmuDownloader::onDownloadProgress(qint64 received, qint64 total)
{
    if (total > 0)
        setProgress(static_cast<double>(received) / static_cast<double>(total));
}

void UmuDownloader::onDownloadFinished(QNetworkReply *reply, const QString &assetName)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        setStatusText(tr("Download failed: %1").arg(reply->errorString()));
        setBusy(false);
        Q_EMIT error(reply->errorString());
        return;
    }

    setStatusText(tr("Installing…"));
    setProgress(1.0);

    QDir().mkpath(m_installPath);
    QString umuBinPath = m_installPath + QStringLiteral("/umu-run");

    if (assetName == QStringLiteral("umu-run")) {
        // Standalone binary — write directly
        QFile binFile(umuBinPath);
        if (!binFile.open(QIODevice::WriteOnly)) {
            setStatusText(tr("Failed to write umu-run"));
            setBusy(false);
            Q_EMIT error(QStringLiteral("Could not write file"));
            return;
        }
        binFile.write(reply->readAll());
        binFile.close();
        binFile.setPermissions(binFile.permissions() | QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther);
    } else {
        // Tar archive — extract umu/umu-run directly
        QTemporaryFile tmpFile;
        tmpFile.setFileTemplate(QDir::tempPath() + QStringLiteral("/vermouth-umu-XXXXXX.tar"));
        if (!tmpFile.open()) {
            setStatusText(tr("Failed to create temp file"));
            setBusy(false);
            Q_EMIT error(QStringLiteral("Could not create temporary file"));
            return;
        }
        tmpFile.write(reply->readAll());
        tmpFile.flush();

        QString tmpExtractDir = QDir::tempPath() + QStringLiteral("/vermouth-umu-extract");
        QDir().mkpath(tmpExtractDir);
        QProcess tar;
        tar.setProgram(QStringLiteral("tar"));
        tar.setArguments({QStringLiteral("-xf"), tmpFile.fileName(), QStringLiteral("-C"), tmpExtractDir});
        tar.start();
        tar.waitForFinished(120000);

        QString foundPath = tmpExtractDir + QStringLiteral("/umu/umu-run");
        if (!QFile::exists(foundPath)) {
            setStatusText(tr("umu-run not found in archive"));
            setBusy(false);
            Q_EMIT error(QStringLiteral("umu-run not found in tarball"));
            QDir(tmpExtractDir).removeRecursively();
            return;
        }
        QFile::remove(umuBinPath);
        QFile::copy(foundPath, umuBinPath);
        QFile(umuBinPath).setPermissions(QFile::permissions(umuBinPath) | QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther);
        QDir(tmpExtractDir).removeRecursively();
    }

    setStatusText(tr("umu-launcher installed!"));
    setBusy(false);
    Q_EMIT finished(umuBinPath);
}
