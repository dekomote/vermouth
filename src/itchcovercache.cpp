#include "itchcovercache.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

ItchCoverCache::ItchCoverCache(QObject *parent)
    : QObject(parent)
{
}

void ItchCoverCache::setCacheDir(const QString &dir)
{
    m_cacheDir = dir;
    scanExisting();
}

QString ItchCoverCache::cachedPath(const QString &gameId) const
{
    return m_cache.value(gameId);
}

void ItchCoverCache::scanExisting()
{
    m_cache.clear();
    QDir dir(m_cacheDir);
    if (!dir.exists())
        return;
    for (const QFileInfo &fi : dir.entryInfoList(QDir::Files))
        m_cache.insert(fi.completeBaseName(), fi.absoluteFilePath());
}

QString ItchCoverCache::buildLocalPath(const QString &gameId, const QString &url) const
{
    QString ext = QFileInfo(QUrl(url).path()).suffix();
    if (ext.isEmpty() || ext.length() > 5)
        ext = QStringLiteral("jpg");
    return m_cacheDir + QLatin1Char('/') + gameId + QLatin1Char('.') + ext;
}

void ItchCoverCache::requestCover(const QString &gameId, const QString &coverUrl)
{
    if (coverUrl.isEmpty())
        return;

    if (m_cache.contains(gameId)) {
        Q_EMIT coverReady(gameId, m_cache[gameId]);
        return;
    }
    if (m_inFlight.contains(gameId))
        return;

    m_inFlight.insert(gameId);
    QString savePath = buildLocalPath(gameId, coverUrl);

    QNetworkRequest req{QUrl(coverUrl)};
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, gameId, savePath, reply]() {
        reply->deleteLater();
        m_inFlight.remove(gameId);
        if (reply->error() != QNetworkReply::NoError)
            return;

        QDir().mkpath(m_cacheDir);
        QFile f(savePath);
        if (!f.open(QIODevice::WriteOnly))
            return;
        f.write(reply->readAll());
        f.close();

        m_cache.insert(gameId, savePath);
        Q_EMIT coverReady(gameId, savePath);
    });
}
