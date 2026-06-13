#pragma once

#include "downloader.h"
#include <QStringList>

class QNetworkReply;
class QFile;

class GogDownloader : public Downloader
{
    Q_OBJECT

public:
    explicit GogDownloader(QObject *parent = nullptr);

    void setCacheDir(const QString &dir);

    Q_INVOKABLE void download(const QString &gameId, const QStringList &urls, bool isWindows);
    Q_INVOKABLE void cancel();
    // Remove the cached installer files for a game (call after a successful install).
    Q_INVOKABLE void clearDownload(const QString &gameId);

Q_SIGNALS:
    void downloadFinished(const QString &gameId, const QString &primaryFilePath, bool isWindows);
    void downloadError(const QString &gameId, const QString &message);

private:
    void startNextFile();
    void cleanupReply();

    QString m_cacheDir;
    QString m_gameId;
    bool m_isWindows = false;
    QStringList m_pending;
    QStringList m_done;
    QString m_saveDir;
    int m_totalFiles = 0;
    QNetworkReply *m_reply = nullptr;
    QFile *m_file = nullptr;
    QString m_tempPath;
    qint64 m_existingBytes = 0;
    bool m_cancelled = false;
};
