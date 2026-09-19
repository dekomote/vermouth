#include "rommclient.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

RommClient::RommClient(QObject *parent)
    : Downloader(parent)
{
}

QString RommClient::serverUrl() const
{
    return m_serverUrl;
}

void RommClient::setServerUrl(const QString &url)
{
    QString trimmed = url.trimmed();
    while (trimmed.endsWith(QLatin1Char('/')))
        trimmed.chop(1);
    if (m_serverUrl == trimmed)
        return;
    m_serverUrl = trimmed;
    Q_EMIT serverUrlChanged();
}

QString RommClient::apiKey() const
{
    return m_apiKey;
}

void RommClient::setApiKey(const QString &key)
{
    if (m_apiKey == key)
        return;
    m_apiKey = key;
    Q_EMIT apiKeyChanged();
}

void RommClient::makeRequest(const QUrl &url, std::function<void(const QByteArray &)> onData)
{
    QNetworkRequest req(url);
    if (!m_apiKey.isEmpty())
        req.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + m_apiKey.toUtf8());
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *reply = nam().get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, onData]() {
        reply->deleteLater();
        setBusy(false);
        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT error(reply->errorString());
            return;
        }
        onData(reply->readAll());
    });
}

QString RommClient::resolveCoverUrl(const QJsonObject &obj) const
{
    // Prefer path_cover_l (served by ROMM itself), fall back to url_cover (external CDN)
    QString path = obj[QStringLiteral("path_cover_l")].toString();
    if (path.isEmpty())
        path = obj[QStringLiteral("url_cover")].toString();
    if (path.isEmpty())
        return {};
    if (path.startsWith(QStringLiteral("http")))
        return path;
    return m_serverUrl + path;
}

void RommClient::fetchPlatforms()
{
    if (m_serverUrl.isEmpty()) {
        Q_EMIT error(QStringLiteral("ROMM server URL not configured"));
        return;
    }
    setBusy(true);
    setStatusText(QStringLiteral("Fetching platforms…"));

    QUrl url(m_serverUrl + QStringLiteral("/api/platforms"));
    makeRequest(url, [this](const QByteArray &data) {
        auto doc = QJsonDocument::fromJson(data);
        if (!doc.isArray()) {
            Q_EMIT error(QStringLiteral("Unexpected response from ROMM /api/platforms"));
            return;
        }
        QVariantList result;
        for (const auto &val : doc.array()) {
            auto obj = val.toObject();
            QVariantMap p;
            p[QStringLiteral("id")] = obj[QStringLiteral("id")].toInt();
            p[QStringLiteral("name")] = obj[QStringLiteral("name")].toString();
            p[QStringLiteral("slug")] = obj[QStringLiteral("slug")].toString();
            p[QStringLiteral("romCount")] = obj[QStringLiteral("rom_count")].toInt();
            p[QStringLiteral("logoUrl")] = obj[QStringLiteral("url_logo")].toString();
            result << p;
        }
        setStatusText({});
        Q_EMIT platformsFetched(result);
    });
}

void RommClient::fetchRoms(int platformId, const QString &search, int page)
{
    if (m_serverUrl.isEmpty()) {
        Q_EMIT error(QStringLiteral("ROMM server URL not configured"));
        return;
    }
    setBusy(true);
    setStatusText(QStringLiteral("Fetching ROMs…"));

    QUrl url(m_serverUrl + QStringLiteral("/api/roms"));
    QUrlQuery q;
    if (platformId > 0)
        q.addQueryItem(QStringLiteral("platform_ids"), QString::number(platformId));
    if (!search.isEmpty())
        q.addQueryItem(QStringLiteral("search_term"), search);
    q.addQueryItem(QStringLiteral("limit"), QString::number(kPageSize));
    q.addQueryItem(QStringLiteral("offset"), QString::number((page - 1) * kPageSize));
    q.addQueryItem(QStringLiteral("with_files"), QString::fromUtf8("true"));
    url.setQuery(q);

    makeRequest(url, [this](const QByteArray &data) {
        auto doc = QJsonDocument::fromJson(data);
        auto root = doc.object();
        auto items = root[QStringLiteral("items")].toArray();
        int total = root[QStringLiteral("total")].toInt();

        QVariantList result;
        for (const auto &val : items) {
            auto obj = val.toObject();
            QVariantMap rom;
            rom[QStringLiteral("id")] = obj[QStringLiteral("id")].toInt();
            rom[QStringLiteral("name")] = obj[QStringLiteral("name")].toString();
            QStringList fileNames;
            for (const auto &fval : obj[QStringLiteral("files")].toArray())
                fileNames << fval.toObject()[QStringLiteral("file_name")].toString();
            rom[QStringLiteral("fileNames")] = fileNames;
            rom[QStringLiteral("platformId")] = obj[QStringLiteral("platform_id")].toInt();
            rom[QStringLiteral("platformSlug")] = obj[QStringLiteral("platform_slug")].toString();
            rom[QStringLiteral("coverUrl")] = resolveCoverUrl(obj);
            rom[QStringLiteral("fileSizeBytes")] = obj[QStringLiteral("file_size_bytes")].toVariant().toLongLong();
            result << rom;
        }
        setStatusText({});
        Q_EMIT romsFetched(result, total);
    });
}
