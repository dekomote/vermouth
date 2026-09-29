#pragma once

#include "downloader.h"
#include <QHash>
#include <QJsonObject>
#include <QList>
#include <functional>

class QNetworkReply;

class ItchClient : public Downloader
{
    Q_OBJECT
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(QString username READ username NOTIFY usernameChanged)

public:
    explicit ItchClient(QObject *parent = nullptr);

    bool authenticated() const;
    QString username() const;

    // Stores the key and immediately tries to fetch the profile to validate it.
    Q_INVOKABLE void setApiKey(const QString &key);
    Q_INVOKABLE void logout();

    // itch.io has no "everything I own" endpoint that includes freely-downloaded
    // (unclaimed) games - only purchases/claimed keys are tracked server-side.
    // So the library is built by crawling every one of the user's collections
    // and merging (de-duplicating) the games in them. `page` is unused; each
    // call does a full crawl and emits the whole merged list at once.
    Q_INVOKABLE void fetchLibrary(const QString &search = {}, int page = 1);
    // Resolves a downloadable upload for the game and emits downloadInfoReady.
    Q_INVOKABLE void fetchDownloadInfo(const QString &gameId, bool preferLinux);

Q_SIGNALS:
    void authenticatedChanged();
    void usernameChanged();
    void libraryFetched(const QVariantList &items, bool hasMore, int page);
    void downloadInfoReady(const QString &gameId, const QString &url, const QString &fileName, bool isWindows);
    void error(const QString &message);

private:
    void setAuthenticated(bool a);
    void setUsername(const QString &name);
    void fetchUserInfo();
    void authedGet(const QUrl &url, std::function<void(const QByteArray &)> onData, std::function<void(const QString &)> onError);
    // The legacy per-key endpoints embed the API key in the path rather than an Authorization header.
    QUrl legacyUrl(const QString &path) const;

    void crawlCollections(QList<qlonglong> pendingCollectionIds, const QString &search, QHash<qlonglong, QJsonObject> accumulated);
    void crawlCollectionPage(qlonglong collectionId,
                             int page,
                             QList<qlonglong> remainingCollections,
                             const QString &search,
                             QHash<qlonglong, QJsonObject> accumulated);
    void finishLibraryCrawl(const QString &search, const QHash<qlonglong, QJsonObject> &accumulated);

    QString m_apiKey;
    QString m_username;
    bool m_authenticated = false;
};
