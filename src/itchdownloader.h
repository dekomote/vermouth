#pragma once

#include "downloader.h"

class QNetworkReply;
class QFile;

class ItchDownloader : public Downloader
{
    Q_OBJECT

public:
    explicit ItchDownloader(QObject *parent = nullptr);

    void setCacheDir(const QString &dir);

    Q_INVOKABLE void download(const QString &gameId, const QString &url, const QString &fileName, bool isWindows);
    Q_INVOKABLE void cancel();
    // Remove the cached download for a game (call after a successful install).
    Q_INVOKABLE void clearDownload(const QString &gameId);

Q_SIGNALS:
    void downloadFinished(const QString &gameId, const QString &filePath, bool isWindows);
    void downloadError(const QString &gameId, const QString &message);

private:
    void cleanupReply();

    QString m_cacheDir;
    QString m_gameId;
    bool m_isWindows = false;
    QString m_saveDir;
    QString m_fileName;
    QNetworkReply *m_reply = nullptr;
    QFile *m_file = nullptr;
    QString m_tempPath;
    qint64 m_existingBytes = 0;
    bool m_cancelled = false;
};
