#include "gogdownloader.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

GogDownloader::GogDownloader(QObject *parent)
    : Downloader(parent)
{
}

void GogDownloader::setCacheDir(const QString &dir)
{
    m_cacheDir = dir;
}

static QString fileNameFromReply(QNetworkReply *reply, int index)
{
    QString disposition = QString::fromUtf8(reply->rawHeader("Content-Disposition"));
    int idx = disposition.indexOf(QStringLiteral("filename="), 0, Qt::CaseInsensitive);
    if (idx >= 0) {
        QString name = disposition.mid(idx + 9).trimmed();
        if (name.startsWith(QLatin1Char('"')))
            name = name.mid(1, name.indexOf(QLatin1Char('"'), 1) - 1);
        else if (name.contains(QLatin1Char(';')))
            name = name.left(name.indexOf(QLatin1Char(';'))).trimmed();
        name = QFileInfo(name).fileName();
        if (!name.isEmpty())
            return name;
    }
    QString fromUrl = QFileInfo(reply->url().path()).fileName();
    if (!fromUrl.isEmpty())
        return fromUrl;
    return QStringLiteral("installer_part_%1").arg(index);
}

void GogDownloader::download(const QString &gameId, const QStringList &urls, bool isWindows)
{
    if (busy()) {
        Q_EMIT downloadError(gameId, QStringLiteral("A download is already in progress"));
        return;
    }
    if (urls.isEmpty()) {
        Q_EMIT downloadError(gameId, QStringLiteral("No download URLs provided"));
        return;
    }
    if (m_cacheDir.isEmpty()) {
        Q_EMIT downloadError(gameId, QStringLiteral("Download cache directory not configured"));
        return;
    }

    m_gameId = gameId;
    m_isWindows = isWindows;
    m_pending = urls;
    m_done.clear();
    m_totalFiles = urls.size();
    m_cancelled = false;
    m_saveDir = m_cacheDir + QStringLiteral("/installers/") + gameId;
    if (!QDir().mkpath(m_saveDir)) {
        Q_EMIT downloadError(gameId, QStringLiteral("Cannot create directory: %1").arg(m_saveDir));
        return;
    }

    setBusy(true);
    setProgress(0.0);
    startNextFile();
}

void GogDownloader::startNextFile()
{
    if (m_cancelled)
        return;
    if (m_pending.isEmpty()) {
        setBusy(false);
        setProgress(1.0);
        setStatusText(QStringLiteral("Download complete"));
        Q_EMIT downloadFinished(m_gameId, m_done.isEmpty() ? QString() : m_done.first(), m_isWindows);
        return;
    }

    int completed = m_done.size();
    QString url = m_pending.takeFirst();

    QString existingName = QFileInfo(QUrl(url).path()).fileName();
    if (!existingName.isEmpty()) {
        QString existingPath = m_saveDir + QLatin1Char('/') + existingName;
        if (QFileInfo::exists(existingPath)) {
            m_done << existingPath;
            setProgress(static_cast<double>(m_done.size()) / m_totalFiles);
            startNextFile();
            return;
        }
    }

    // Stable per-part temp name so a partial download can be resumed later.
    m_tempPath = m_saveDir + QStringLiteral("/.part_%1.download").arg(completed);
    m_existingBytes = QFileInfo::exists(m_tempPath) ? QFileInfo(m_tempPath).size() : 0;

    QNetworkRequest req{QUrl(url)};
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    if (m_existingBytes > 0)
        req.setRawHeader("Range", QByteArrayLiteral("bytes=") + QByteArray::number(m_existingBytes) + "-");
    m_reply = nam().get(req);

    // File is opened lazily once we know whether the server honoured the Range
    // request (HTTP 206 = resume/append, anything else = restart/truncate).
    setStatusText(QStringLiteral("Downloading file %1 of %2…").arg(completed + 1).arg(m_totalFiles));

    connect(m_reply, &QNetworkReply::downloadProgress, this, [this, completed](qint64 received, qint64 total) {
        if (total > 0) {
            double frac = static_cast<double>(m_existingBytes + received) / (m_existingBytes + total);
            setProgress((completed + frac) / m_totalFiles);
        }
    });
    connect(m_reply, &QNetworkReply::readyRead, this, [this]() {
        if (!m_file) {
            int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            bool resuming = status == 206 && m_existingBytes > 0;
            if (!resuming)
                m_existingBytes = 0;
            m_file = new QFile(m_tempPath, this);
            QIODevice::OpenMode mode = resuming ? (QIODevice::WriteOnly | QIODevice::Append) : (QIODevice::WriteOnly | QIODevice::Truncate);
            if (!m_file->open(mode)) {
                QString err = m_file->errorString();
                m_file->deleteLater();
                m_file = nullptr;
                m_reply->abort();
                setBusy(false);
                Q_EMIT downloadError(m_gameId, QStringLiteral("Cannot write to %1: %2").arg(m_tempPath, err));
                return;
            }
        }
        m_file->write(m_reply->readAll());
    });
    connect(m_reply, &QNetworkReply::finished, this, [this, completed]() {
        if (m_cancelled) {
            cleanupReply();
            return;
        }
        QNetworkReply *reply = m_reply;
        bool ok = reply->error() == QNetworkReply::NoError;
        QString errStr = reply->errorString();
        QString fileName = ok ? fileNameFromReply(reply, completed) : QString();

        if (m_file) {
            m_file->write(reply->readAll());
            m_file->close();
        }
        cleanupReply();

        if (!ok) {
            // Keep the partial file so the next attempt can resume via Range.
            setBusy(false);
            Q_EMIT downloadError(m_gameId, errStr);
            return;
        }

        QString finalPath = m_saveDir + QLatin1Char('/') + fileName;
        QFile::remove(finalPath);
        if (!QFile::rename(m_tempPath, finalPath)) {
            setBusy(false);
            Q_EMIT downloadError(m_gameId, QStringLiteral("Cannot finalize %1").arg(finalPath));
            return;
        }
        m_done << finalPath;
        startNextFile();
    });
}

void GogDownloader::clearDownload(const QString &gameId)
{
    if (m_cacheDir.isEmpty() || gameId.isEmpty())
        return;
    QDir(m_cacheDir + QStringLiteral("/installers/") + gameId).removeRecursively();
}

void GogDownloader::cleanupReply()
{
    if (m_reply) {
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    if (m_file) {
        m_file->deleteLater();
        m_file = nullptr;
    }
}

void GogDownloader::cancel()
{
    if (!busy())
        return;
    m_cancelled = true;
    if (m_reply)
        m_reply->abort();
    cleanupReply();
    QFile::remove(m_tempPath);
    setBusy(false);
    setStatusText(QStringLiteral("Download cancelled"));
}
