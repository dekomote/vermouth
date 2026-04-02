#include "protondownloader.h"
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryFile>

ProtonDownloader::ProtonDownloader(QObject *parent)
    : QObject(parent)

{
    m_statusClearTimer.setSingleShot(true);
    m_statusClearTimer.setInterval(6000);
    connect(&m_statusClearTimer, &QTimer::timeout, this, [this]() {
        setStatusText(QString());
    });
}

void ProtonDownloader::setLocalProtonPath(const QString &path)
{
    m_localProtonPath = path;
}

bool ProtonDownloader::busy() const
{
    return m_busy;
}
QString ProtonDownloader::statusText() const
{
    return m_statusText;
}
double ProtonDownloader::progress() const
{
    return m_progress;
}

void ProtonDownloader::setBusy(bool busy)
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

void ProtonDownloader::setStatusText(const QString &text)
{
    if (m_statusText != text) {
        m_statusText = text;
        Q_EMIT statusTextChanged();
    }
}

void ProtonDownloader::setProgress(double progress)
{
    if (m_progress != progress) {
        m_progress = progress;
        Q_EMIT progressChanged();
    }
}

void ProtonDownloader::downloadLatest()
{
    if (m_busy)
        return;

    setBusy(true);
    setStatusText(tr("Checking latest release…"));
    setProgress(0.0);

    // Use the redirect from /releases/latest to discover the tag
    QNetworkRequest req(QUrl(QStringLiteral("https://github.com/GloriousEggroll/proton-ge-custom/releases/latest")));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    auto *reply = m_nam.get(req, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReleaseFetched(reply);
    });
}

void ProtonDownloader::onReleaseFetched(QNetworkReply *reply)
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
            Q_EMIT finished();
            return;
        }
    }

    // Construct download URL directly: https://github.com/.../releases/download/<tag>/<tag>.tar.gz
    QString downloadUrl = QStringLiteral("https://github.com/GloriousEggroll/proton-ge-custom/releases/download/%1/%1.tar.gz").arg(tagName);

    setStatusText(tr("Downloading %1…").arg(tagName));

    QUrl dlUrl(downloadUrl);
    QNetworkRequest dlReq(dlUrl);
    dlReq.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
    dlReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *dlReply = m_nam.get(dlReq, QByteArray());
    connect(dlReply, &QNetworkReply::downloadProgress, this, &ProtonDownloader::onDownloadProgress);
    connect(dlReply, &QNetworkReply::finished, this, [this, dlReply]() {
        onDownloadFinished(dlReply);
    });
}

void ProtonDownloader::onDownloadProgress(qint64 received, qint64 total)
{
    if (total > 0)
        setProgress(static_cast<double>(received) / static_cast<double>(total));
}

void ProtonDownloader::onDownloadFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        setStatusText(tr("Download failed"));
        setBusy(false);
        Q_EMIT error(reply->errorString());
        return;
    }

    setStatusText(tr("Extracting…"));
    setProgress(1.0);

    QTemporaryFile tmpFile;
    tmpFile.setFileTemplate(QDir::tempPath() + QStringLiteral("/vermouth-proton-XXXXXX.tar.gz"));
    if (!tmpFile.open()) {
        setStatusText(tr("Failed to create temp file"));
        setBusy(false);
        Q_EMIT error(QStringLiteral("Could not create temporary file"));
        return;
    }

    tmpFile.write(reply->readAll());
    tmpFile.flush();

    if (!extractTarGz(tmpFile.fileName(), m_localProtonPath)) {
        setStatusText(tr("Extraction failed"));
        setBusy(false);
        Q_EMIT error(QStringLiteral("tar extraction failed"));
        return;
    }

    setStatusText(tr("Done!"));
    setBusy(false);
    Q_EMIT finished();
}

bool ProtonDownloader::extractTarGz(const QString &archivePath, const QString &destDir)
{
    QDir().mkpath(destDir);
    QProcess proc;
    proc.setProgram(QStringLiteral("tar"));
    proc.setArguments({QStringLiteral("-xzf"), archivePath, QStringLiteral("-C"), destDir});
    proc.start();
    proc.waitForFinished(300000);
    return proc.exitCode() == 0;
}
