#include "gogclient.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

namespace
{
// Public GOG Galaxy client credentials (same ones minigalaxy/heroic use).
const QString kClientId = QStringLiteral("46899977096215655");
const QString kClientSecret = QStringLiteral("9d85c43b1482497dbbce61f6e4aa173a433796eeae2ca8c5f6129f2dc4de46d9");
const QString kRedirectUri = QStringLiteral("https://embed.gog.com/on_login_success?origin=client");

QJsonObject pickInstaller(const QJsonArray &installers, bool preferLinux, bool &isWindows)
{
    auto pickFor = [&](const QString &os) -> QJsonObject {
        QJsonObject fallback;
        for (const auto &val : installers) {
            auto inst = val.toObject();
            if (inst[QStringLiteral("os")].toString() != os)
                continue;
            if (fallback.isEmpty())
                fallback = inst;
            QString lang = inst[QStringLiteral("language")].toString().toLower();
            if (lang == QStringLiteral("en") || lang.contains(QStringLiteral("english")))
                return inst;
        }
        return fallback;
    };

    isWindows = false;
    QJsonObject chosen;
    if (preferLinux)
        chosen = pickFor(QStringLiteral("linux"));
    if (chosen.isEmpty()) {
        chosen = pickFor(QStringLiteral("windows"));
        isWindows = true;
    }
    return chosen;
}
}

GogClient::GogClient(QObject *parent)
    : Downloader(parent)
{
}

bool GogClient::authenticated() const
{
    return m_authenticated;
}

QString GogClient::username() const
{
    return m_username;
}

QString GogClient::refreshToken() const
{
    return m_refreshToken;
}

void GogClient::setRefreshToken(const QString &token)
{
    m_refreshToken = token;
}

void GogClient::setAuthenticated(bool a)
{
    if (m_authenticated == a)
        return;
    m_authenticated = a;
    Q_EMIT authenticatedChanged();
}

void GogClient::setUsername(const QString &name)
{
    if (m_username == name)
        return;
    m_username = name;
    Q_EMIT usernameChanged();
}

QString GogClient::loginUrl() const
{
    QUrl url(QStringLiteral("https://auth.gog.com/auth"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("client_id"), kClientId);
    q.addQueryItem(QStringLiteral("redirect_uri"), kRedirectUri);
    q.addQueryItem(QStringLiteral("response_type"), QStringLiteral("code"));
    q.addQueryItem(QStringLiteral("layout"), QStringLiteral("client2"));
    url.setQuery(q);
    return url.toString();
}

QString GogClient::extractCode(const QString &redirectUrl) const
{
    QString trimmed = redirectUrl.trimmed();
    if (trimmed.isEmpty())
        return {};
    QUrl url(trimmed);
    if (url.hasQuery()) {
        QUrlQuery q(url);
        if (q.hasQueryItem(QStringLiteral("code")))
            return q.queryItemValue(QStringLiteral("code"));
    }
    // Allow pasting a bare code with no URL around it.
    if (!trimmed.contains(QLatin1Char('/')) && !trimmed.contains(QLatin1Char('?')))
        return trimmed;
    return {};
}

void GogClient::authenticateWithRedirect(const QString &redirectUrl)
{
    QString code = extractCode(redirectUrl);
    if (code.isEmpty()) {
        Q_EMIT loginFailed(QStringLiteral("Could not find a login code in the pasted URL"));
        return;
    }

    setBusy(true);
    setStatusText(QStringLiteral("Logging in to GOG…"));

    QUrl url(QStringLiteral("https://auth.gog.com/token"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("client_id"), kClientId);
    q.addQueryItem(QStringLiteral("client_secret"), kClientSecret);
    q.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("authorization_code"));
    q.addQueryItem(QStringLiteral("code"), code);
    q.addQueryItem(QStringLiteral("redirect_uri"), kRedirectUri);
    url.setQuery(q);

    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *reply = nam().get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        setBusy(false);
        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT loginFailed(reply->errorString());
            return;
        }
        parseTokenResponse(reply->readAll(), false);
    });
}

void GogClient::refreshSession()
{
    if (m_refreshToken.isEmpty())
        return;
    refreshAccessToken([this](bool ok) {
        if (ok)
            fetchUserInfo();
    });
}

void GogClient::refreshAccessToken(std::function<void(bool)> done)
{
    if (m_refreshToken.isEmpty()) {
        if (done)
            done(false);
        return;
    }

    QUrl url(QStringLiteral("https://auth.gog.com/token"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("client_id"), kClientId);
    q.addQueryItem(QStringLiteral("client_secret"), kClientSecret);
    q.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("refresh_token"));
    q.addQueryItem(QStringLiteral("refresh_token"), m_refreshToken);
    url.setQuery(q);

    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *reply = nam().get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, done]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            setAuthenticated(false);
            if (done)
                done(false);
            return;
        }
        parseTokenResponse(reply->readAll(), true);
        if (done)
            done(m_authenticated);
    });
}

void GogClient::parseTokenResponse(const QByteArray &data, bool fromRefresh)
{
    auto obj = QJsonDocument::fromJson(data).object();
    QString access = obj[QStringLiteral("access_token")].toString();
    QString refresh = obj[QStringLiteral("refresh_token")].toString();
    if (access.isEmpty()) {
        setAuthenticated(false);
        if (!fromRefresh)
            Q_EMIT loginFailed(QStringLiteral("GOG did not return an access token"));
        return;
    }
    m_accessToken = access;
    if (!refresh.isEmpty() && refresh != m_refreshToken) {
        m_refreshToken = refresh;
        Q_EMIT refreshTokenObtained(m_refreshToken);
    }
    setAuthenticated(true);
    if (!fromRefresh) {
        Q_EMIT loginSucceeded();
        fetchUserInfo();
    }
}

void GogClient::fetchUserInfo()
{
    authedGet(
        QUrl(QStringLiteral("https://embed.gog.com/userData.json")),
        [this](const QByteArray &data) {
            auto obj = QJsonDocument::fromJson(data).object();
            setUsername(obj[QStringLiteral("username")].toString());
        },
        [](const QString &) {});
}

void GogClient::authedGet(const QUrl &url, std::function<void(const QByteArray &)> onData, std::function<void(const QString &)> onError, bool allowRetry)
{
    QNetworkRequest req(url);
    if (!m_accessToken.isEmpty())
        req.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + m_accessToken.toUtf8());
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    auto *reply = nam().get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url, onData, onError, allowRetry]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status == 401 && allowRetry && !m_refreshToken.isEmpty()) {
                // Access token likely expired - refresh once and retry.
                refreshAccessToken([this, url, onData, onError](bool ok) {
                    if (ok)
                        authedGet(url, onData, onError, false);
                    else if (onError)
                        onError(QStringLiteral("GOG session expired - please log in again"));
                });
                return;
            }
            if (onError)
                onError(reply->errorString());
            return;
        }
        if (onData)
            onData(reply->readAll());
    });
}

void GogClient::fetchLibrary(const QString &search, int page)
{
    if (!m_authenticated) {
        Q_EMIT error(QStringLiteral("Not logged in to GOG"));
        return;
    }
    setBusy(true);
    setStatusText(QStringLiteral("Fetching GOG library…"));

    QUrl url(QStringLiteral("https://embed.gog.com/account/getFilteredProducts"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("mediaType"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("page"), QString::number(page));
    if (!search.trimmed().isEmpty())
        q.addQueryItem(QStringLiteral("search"), search.trimmed());
    url.setQuery(q);

    authedGet(
        url,
        [this, page](const QByteArray &data) {
            setBusy(false);
            setStatusText({});
            auto root = QJsonDocument::fromJson(data).object();
            int totalPages = root[QStringLiteral("totalPages")].toInt();
            QVariantList result;
            for (const auto &val : root[QStringLiteral("products")].toArray()) {
                auto obj = val.toObject();
                QVariantMap g;
                qlonglong id = obj[QStringLiteral("id")].toVariant().toLongLong();
                g[QStringLiteral("gameId")] = QString::number(id);
                g[QStringLiteral("title")] = obj[QStringLiteral("title")].toString();

                QString image = obj[QStringLiteral("image")].toString();
                if (!image.isEmpty()) {
                    if (image.startsWith(QStringLiteral("//")))
                        image = QStringLiteral("https:") + image;
                    QString lower = image.toLower();
                    bool hasExt = lower.endsWith(QStringLiteral(".jpg")) || lower.endsWith(QStringLiteral(".jpeg")) || lower.endsWith(QStringLiteral(".png"))
                        || lower.endsWith(QStringLiteral(".webp")) || lower.endsWith(QStringLiteral(".gif"));
                    g[QStringLiteral("coverUrl")] = hasExt ? image : QString(image + QStringLiteral(".jpg"));
                } else {
                    g[QStringLiteral("coverUrl")] = QString();
                }

                auto worksOn = obj[QStringLiteral("worksOn")].toObject();
                g[QStringLiteral("worksOnWindows")] = worksOn[QStringLiteral("Windows")].toBool();
                g[QStringLiteral("worksOnLinux")] = worksOn[QStringLiteral("Linux")].toBool();
                result << g;
            }
            Q_EMIT libraryFetched(result, totalPages, page);
        },
        [this](const QString &msg) {
            setBusy(false);
            setStatusText({});
            Q_EMIT error(msg);
        });
}

void GogClient::fetchDownloadInfo(const QString &gameId, bool preferLinux)
{
    if (!m_authenticated) {
        Q_EMIT error(QStringLiteral("Not logged in to GOG"));
        return;
    }
    setBusy(true);
    setStatusText(QStringLiteral("Resolving download…"));

    QUrl url(QStringLiteral("https://api.gog.com/products/") + gameId);
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("expand"), QStringLiteral("downloads"));
    q.addQueryItem(QStringLiteral("locale"), QStringLiteral("en-US"));
    url.setQuery(q);

    authedGet(
        url,
        [this, gameId, preferLinux](const QByteArray &data) {
            auto root = QJsonDocument::fromJson(data).object();
            auto installers = root[QStringLiteral("downloads")].toObject()[QStringLiteral("installers")].toArray();

            bool isWindows = false;
            QJsonObject chosen = pickInstaller(installers, preferLinux, isWindows);

            if (chosen.isEmpty()) {
                setBusy(false);
                setStatusText({});
                Q_EMIT error(QStringLiteral("No suitable installer found for this game"));
                return;
            }

            QStringList downlinks;
            for (const auto &fval : chosen[QStringLiteral("files")].toArray()) {
                QString dl = fval.toObject()[QStringLiteral("downlink")].toString();
                if (!dl.isEmpty())
                    downlinks << dl;
            }
            if (downlinks.isEmpty()) {
                setBusy(false);
                setStatusText({});
                Q_EMIT error(QStringLiteral("Installer has no downloadable files"));
                return;
            }
            resolveNextDownlink(gameId, downlinks, {}, isWindows);
        },
        [this](const QString &msg) {
            setBusy(false);
            setStatusText({});
            Q_EMIT error(msg);
        });
}

void GogClient::fetchGameSize(const QString &gameId, bool preferLinux)
{
    if (!m_authenticated)
        return;

    QUrl url(QStringLiteral("https://api.gog.com/products/") + gameId);
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("expand"), QStringLiteral("downloads"));
    q.addQueryItem(QStringLiteral("locale"), QStringLiteral("en-US"));
    url.setQuery(q);

    // Runs quietly in the background (no busy/status flips) like cover fetching.
    // Always emits exactly one sizeFetched (bytes < 0 means unknown) so callers
    // can throttle a queue without it stalling on errors.
    authedGet(
        url,
        [this, gameId, preferLinux](const QByteArray &data) {
            auto root = QJsonDocument::fromJson(data).object();
            auto installers = root[QStringLiteral("downloads")].toObject()[QStringLiteral("installers")].toArray();
            bool isWindows = false;
            QJsonObject chosen = pickInstaller(installers, preferLinux, isWindows);
            double bytes = chosen[QStringLiteral("total_size")].toVariant().toDouble();
            if (bytes <= 0) {
                for (const auto &fval : chosen[QStringLiteral("files")].toArray())
                    bytes += fval.toObject()[QStringLiteral("size")].toVariant().toDouble();
            }
            Q_EMIT sizeFetched(gameId, bytes > 0 ? bytes : -1);
        },
        [this, gameId](const QString &) {
            Q_EMIT sizeFetched(gameId, -1);
        });
}

void GogClient::resolveNextDownlink(const QString &gameId, QStringList pending, QStringList resolved, bool isWindows)
{
    if (pending.isEmpty()) {
        setBusy(false);
        setStatusText({});
        Q_EMIT downloadInfoReady(gameId, resolved, isWindows);
        return;
    }
    QString next = pending.takeFirst();
    authedGet(
        QUrl(next),
        [this, gameId, pending, resolved, isWindows](const QByteArray &data) mutable {
            auto obj = QJsonDocument::fromJson(data).object();
            QString real = obj[QStringLiteral("downlink")].toString();
            if (!real.isEmpty())
                resolved << real;
            resolveNextDownlink(gameId, pending, resolved, isWindows);
        },
        [this](const QString &msg) {
            setBusy(false);
            setStatusText({});
            Q_EMIT error(msg);
        });
}

void GogClient::logout()
{
    m_accessToken.clear();
    m_refreshToken.clear();
    setUsername({});
    setAuthenticated(false);
    Q_EMIT refreshTokenObtained(QString());
}
