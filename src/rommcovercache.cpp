#include "rommcovercache.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

RommCoverCache::RommCoverCache(QObject *parent)
    : QObject(parent)
{
}

void RommCoverCache::setCacheDir(const QString &dir)
{
    m_cacheDir = dir;
    scanExisting();
}

void RommCoverCache::setApiKey(const QString &key)
{
    m_apiKey = key;
}

QString RommCoverCache::cachedPath(int romId) const
{
    return m_cache.value(romId);
}

void RommCoverCache::scanExisting()
{
    m_cache.clear();
    QDir dir(m_cacheDir);
    if (!dir.exists())
        return;
    for (const QFileInfo &fi : dir.entryInfoList(QDir::Files)) {
        bool ok = false;
        int id = fi.baseName().toInt(&ok);
        if (ok)
            m_cache.insert(id, fi.absoluteFilePath());
    }
}

QString RommCoverCache::buildLocalPath(int romId, const QString &url) const
{
    QString ext = QFileInfo(QUrl(url).path()).suffix();
    if (ext.isEmpty() || ext.length() > 5)
        ext = QStringLiteral("jpg");
    return m_cacheDir + QLatin1Char('/') + QString::number(romId) + QLatin1Char('.') + ext;
}

void RommCoverCache::requestCover(int romId, const QString &coverUrl)
{
    if (coverUrl.isEmpty())
        return;

    if (m_cache.contains(romId)) {
        Q_EMIT coverReady(romId, m_cache[romId]);
        return;
    }

    if (m_inFlight.contains(romId))
        return;

    m_inFlight.insert(romId);
    QString savePath = buildLocalPath(romId, coverUrl);

    QUrl resolvedUrl(coverUrl);
    QNetworkRequest req(resolvedUrl);
    if (!m_apiKey.isEmpty())
        req.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + m_apiKey.toUtf8());
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, romId, savePath, reply]() {
        reply->deleteLater();
        m_inFlight.remove(romId);
        if (reply->error() != QNetworkReply::NoError)
            return;

        QDir().mkpath(m_cacheDir);
        QFile f(savePath);
        if (!f.open(QIODevice::WriteOnly))
            return;
        f.write(reply->readAll());
        f.close();

        m_cache.insert(romId, savePath);
        Q_EMIT coverReady(romId, savePath);
    });
}
