#pragma once

#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>

class GogCoverCache : public QObject
{
    Q_OBJECT

public:
    explicit GogCoverCache(QObject *parent = nullptr);

    void setCacheDir(const QString &dir);

    void requestCover(const QString &gameId, const QString &coverUrl);
    QString cachedPath(const QString &gameId) const;

Q_SIGNALS:
    void coverReady(const QString &gameId, const QString &localPath);

private:
    QString buildLocalPath(const QString &gameId, const QString &url) const;
    void scanExisting();

    QNetworkAccessManager m_nam;
    QString m_cacheDir;
    QSet<QString> m_inFlight;
    QHash<QString, QString> m_cache;
};
