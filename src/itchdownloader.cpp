#include "itchdownloader.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

ItchDownloader::ItchDownloader(QObject *parent)
    : Downloader(parent)
{
}

void ItchDownloader::setCacheDir(const QString &dir)
{
    m_cacheDir = dir;
}

static QString fileNameFromReply(QNetworkReply *reply, const QString &fallback)
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
    return fallback.isEmpty() ? QStringLiteral("download") : fallback;
}

void ItchDownloader::download(const QString &gameId, const QString &url, const QString &fileName, bool isWindows)
{
    if (busy()) {
        Q_EMIT downloadError(gameId, QStringLiteral("A download is already in progress"));
        return;
    }
    if (url.isEmpty()) {
        Q_EMIT downloadError(gameId, QStringLiteral("No download URL provided"));
        return;
    }
    if (m_cacheDir.isEmpty()) {
        Q_EMIT downloadError(gameId, QStringLiteral("Download cache directory not configured"));
        return;
    }

    m_gameId = gameId;
    m_isWindows = isWindows;
    m_fileName = fileName;
    m_cancelled = false;
    m_saveDir = m_cacheDir + QStringLiteral("/downloads/") + gameId;
    if (!QDir().mkpath(m_saveDir)) {
        Q_EMIT downloadError(gameId, QStringLiteral("Cannot create directory: %1").arg(m_saveDir));
        return;
    }

    if (!fileName.isEmpty()) {
        QString existingPath = m_saveDir + QLatin1Char('/') + fileName;
        if (QFileInfo::exists(existingPath)) {
            setBusy(false);
            setProgress(1.0);
            Q_EMIT downloadFinished(gameId, existingPath, isWindows);
            return;
        }
    }

    setBusy(true);
    setProgress(0.0);
    setStatusText(QStringLiteral("Downloading…"));

    // Stable temp name so a partial download can be resumed later.
    m_tempPath = m_saveDir + QStringLiteral("/.download.part");
    m_existingBytes = QFileInfo::exists(m_tempPath) ? QFileInfo(m_tempPath).size() : 0;

    QNetworkRequest req{QUrl(url)};
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    if (m_existingBytes > 0)
        req.setRawHeader("Range", QByteArrayLiteral("bytes=") + QByteArray::number(m_existingBytes) + "-");
    m_reply = nam().get(req);

    // File is opened lazily once we know whether the server honoured the Range
    // request (HTTP 206 = resume/append, anything else = restart/truncate).
    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0)
            setProgress(static_cast<double>(m_existingBytes + received) / (m_existingBytes + total));
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
    connect(m_reply, &QNetworkReply::finished, this, [this]() {
        if (m_cancelled) {
            cleanupReply();
            return;
        }
        QNetworkReply *reply = m_reply;
        bool ok = reply->error() == QNetworkReply::NoError;
        QString errStr = reply->errorString();
        QString name = ok ? fileNameFromReply(reply, m_fileName) : QString();

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

        QString finalPath = m_saveDir + QLatin1Char('/') + name;
        QFile::remove(finalPath);
        if (!QFile::rename(m_tempPath, finalPath)) {
            setBusy(false);
            Q_EMIT downloadError(m_gameId, QStringLiteral("Cannot finalize %1").arg(finalPath));
            return;
        }
        setBusy(false);
        setProgress(1.0);
        Q_EMIT downloadFinished(m_gameId, finalPath, m_isWindows);
    });
}

void ItchDownloader::clearDownload(const QString &gameId)
{
    if (m_cacheDir.isEmpty() || gameId.isEmpty())
        return;
    QDir(m_cacheDir + QStringLiteral("/downloads/") + gameId).removeRecursively();
}

void ItchDownloader::cleanupReply()
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

void ItchDownloader::cancel()
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
