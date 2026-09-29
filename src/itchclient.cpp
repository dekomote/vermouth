#include "itchclient.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

ItchClient::ItchClient(QObject *parent)
    : Downloader(parent)
{
}

bool ItchClient::authenticated() const
{
    return m_authenticated;
}

QString ItchClient::username() const
{
    return m_username;
}

void ItchClient::setAuthenticated(bool a)
{
    if (m_authenticated == a)
        return;
    m_authenticated = a;
    Q_EMIT authenticatedChanged();
}

void ItchClient::setUsername(const QString &name)
{
    if (m_username == name)
        return;
    m_username = name;
    Q_EMIT usernameChanged();
}

void ItchClient::setApiKey(const QString &key)
{
    m_apiKey = key.trimmed();
    if (m_apiKey.isEmpty()) {
        setAuthenticated(false);
        setUsername({});
        return;
    }
    fetchUserInfo();
}

void ItchClient::logout()
{
    m_apiKey.clear();
    setUsername({});
    setAuthenticated(false);
}

QUrl ItchClient::legacyUrl(const QString &path) const
{
    return QUrl(QStringLiteral("https://itch.io/api/1/") + m_apiKey + QLatin1Char('/') + path);
}

void ItchClient::authedGet(const QUrl &url, std::function<void(const QByteArray &)> onData, std::function<void(const QString &)> onError)
{
    QNetworkRequest req(url);
    if (!m_apiKey.isEmpty())
        req.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + m_apiKey.toUtf8());
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    auto *reply = nam().get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, onData, onError]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (onError)
                onError(reply->errorString());
            return;
        }
        if (onData)
            onData(reply->readAll());
    });
}

void ItchClient::fetchUserInfo()
{
    authedGet(
        QUrl(QStringLiteral("https://api.itch.io/profile")),
        [this](const QByteArray &data) {
            auto root = QJsonDocument::fromJson(data).object();
            auto user = root[QStringLiteral("user")].toObject();
            QString name = user[QStringLiteral("username")].toString();
            if (name.isEmpty()) {
                setAuthenticated(false);
                Q_EMIT error(QStringLiteral("itch.io did not recognize this API key"));
                return;
            }
            setUsername(name);
            setAuthenticated(true);
        },
        [this](const QString &msg) {
            setAuthenticated(false);
            Q_EMIT error(QStringLiteral("itch.io login failed: %1").arg(msg));
        });
}

void ItchClient::fetchLibrary(const QString &search, int)
{
    if (!m_authenticated) {
        Q_EMIT error(QStringLiteral("Not logged in to itch.io"));
        return;
    }
    setBusy(true);
    setStatusText(QStringLiteral("Fetching your itch.io collections…"));

    authedGet(
        QUrl(QStringLiteral("https://api.itch.io/profile/collections")),
        [this, search](const QByteArray &data) {
            auto root = QJsonDocument::fromJson(data).object();
            QList<qlonglong> ids;
            for (const auto &val : root[QStringLiteral("collections")].toArray())
                ids << val.toObject()[QStringLiteral("id")].toVariant().toLongLong();
            crawlCollections(ids, search, {});
        },
        [this](const QString &msg) {
            setBusy(false);
            setStatusText({});
            Q_EMIT error(msg);
        });
}

void ItchClient::crawlCollections(QList<qlonglong> pendingCollectionIds, const QString &search, QHash<qlonglong, QJsonObject> accumulated)
{
    if (pendingCollectionIds.isEmpty()) {
        finishLibraryCrawl(search, accumulated);
        return;
    }
    qlonglong id = pendingCollectionIds.takeFirst();
    crawlCollectionPage(id, 1, pendingCollectionIds, search, accumulated);
}

void ItchClient::crawlCollectionPage(qlonglong collectionId,
                                     int page,
                                     QList<qlonglong> remainingCollections,
                                     const QString &search,
                                     QHash<qlonglong, QJsonObject> accumulated)
{
    QUrl url(QStringLiteral("https://api.itch.io/collections/%1/collection-games").arg(collectionId));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("page"), QString::number(page));
    url.setQuery(q);

    authedGet(
        url,
        [this, collectionId, page, remainingCollections, search, accumulated](const QByteArray &data) mutable {
            auto root = QJsonDocument::fromJson(data).object();
            auto items = root[QStringLiteral("collection_games")].toArray();
            int perPage = root[QStringLiteral("per_page")].toInt(30);

            for (const auto &val : items) {
                QJsonObject game = val.toObject()[QStringLiteral("game")].toObject();
                qlonglong gid = game[QStringLiteral("id")].toVariant().toLongLong();
                if (gid != 0)
                    accumulated.insert(gid, game);
            }

            if (!items.isEmpty() && items.size() >= perPage)
                crawlCollectionPage(collectionId, page + 1, remainingCollections, search, accumulated);
            else
                crawlCollections(remainingCollections, search, accumulated);
        },
        [this, remainingCollections, search, accumulated](const QString &) mutable {
            // Skip this collection on error rather than failing the whole library.
            crawlCollections(remainingCollections, search, accumulated);
        });
}

void ItchClient::finishLibraryCrawl(const QString &search, const QHash<qlonglong, QJsonObject> &accumulated)
{
    setBusy(false);
    setStatusText({});

    QVariantList result;
    QString needle = search.trimmed().toLower();
    for (auto it = accumulated.constBegin(); it != accumulated.constEnd(); ++it) {
        const QJsonObject &game = it.value();
        QString title = game[QStringLiteral("title")].toString();
        if (!needle.isEmpty() && !title.toLower().contains(needle))
            continue;

        QVariantMap g;
        g[QStringLiteral("gameId")] = QString::number(it.key());
        g[QStringLiteral("title")] = title;
        g[QStringLiteral("coverUrl")] = game[QStringLiteral("cover_url")].toString();
        g[QStringLiteral("worksOnWindows")] = game[QStringLiteral("p_windows")].toBool();
        g[QStringLiteral("worksOnLinux")] = game[QStringLiteral("p_linux")].toBool();
        result << g;
    }
    // Every collection is fully crawled up front, so there's never "more" to page in.
    Q_EMIT libraryFetched(result, false, 1);
}

void ItchClient::fetchDownloadInfo(const QString &gameId, bool preferLinux)
{
    if (!m_authenticated) {
        Q_EMIT error(QStringLiteral("Not logged in to itch.io"));
        return;
    }
    setBusy(true);
    setStatusText(QStringLiteral("Resolving download…"));

    authedGet(
        legacyUrl(QStringLiteral("game/") + gameId + QStringLiteral("/uploads")),
        [this, gameId, preferLinux](const QByteArray &data) {
            auto root = QJsonDocument::fromJson(data).object();
            auto uploads = root[QStringLiteral("uploads")].toArray();

            QJsonObject chosen;
            bool isWindows = false;
            auto pickFor = [&](const QString &platformField) -> QJsonObject {
                for (const auto &val : uploads) {
                    auto up = val.toObject();
                    if (!up[platformField].toBool())
                        continue;
                    // Skip demo/soundtrack side-uploads when a main build is available.
                    bool looksExtra = false;
                    for (const auto &trait : up[QStringLiteral("traits")].toArray()) {
                        QString t = trait.toString();
                        if (t == QStringLiteral("demo") || t == QStringLiteral("soundtrack"))
                            looksExtra = true;
                    }
                    if (!looksExtra)
                        return up;
                }
                return {};
            };

            if (preferLinux)
                chosen = pickFor(QStringLiteral("p_linux"));
            if (chosen.isEmpty()) {
                chosen = pickFor(QStringLiteral("p_windows"));
                isWindows = true;
            } else {
                isWindows = false;
            }
            if (chosen.isEmpty() && !uploads.isEmpty()) {
                chosen = uploads.first().toObject();
                isWindows = chosen[QStringLiteral("p_windows")].toBool();
            }

            if (chosen.isEmpty()) {
                setBusy(false);
                setStatusText({});
                Q_EMIT error(QStringLiteral("No suitable download found for this game"));
                return;
            }

            qlonglong uploadId = chosen[QStringLiteral("id")].toVariant().toLongLong();
            QString fileName = chosen[QStringLiteral("filename")].toString();

            // This endpoint returns JSON ({"url": "..."}), not a redirect - the
            // "url" is a presigned CDN link that expires in ~60s, so it's resolved
            // right before handing off to ItchDownloader rather than upfront.
            authedGet(
                legacyUrl(QStringLiteral("upload/") + QString::number(uploadId) + QStringLiteral("/download")),
                [this, gameId, fileName, isWindows](const QByteArray &data) {
                    setBusy(false);
                    setStatusText({});
                    QString url = QJsonDocument::fromJson(data).object()[QStringLiteral("url")].toString();
                    if (url.isEmpty()) {
                        Q_EMIT error(QStringLiteral("itch.io did not return a download link"));
                        return;
                    }
                    Q_EMIT downloadInfoReady(gameId, url, fileName, isWindows);
                },
                [this](const QString &msg) {
                    setBusy(false);
                    setStatusText({});
                    Q_EMIT error(msg);
                });
        },
        [this](const QString &msg) {
            setBusy(false);
            setStatusText({});
            Q_EMIT error(msg);
        });
}
