#include "steamgriddb.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QRegularExpression>

SteamGridDB::SteamGridDB(QObject *parent)
    : QObject(parent)
{
}

bool SteamGridDB::busy() const
{
    return m_busy;
}

QString SteamGridDB::statusText() const
{
    return m_statusText;
}

void SteamGridDB::setBusy(bool busy)
{
    if (m_busy != busy) {
        m_busy = busy;
        Q_EMIT busyChanged();
    }
}

void SteamGridDB::setStatusText(const QString &text)
{
    if (m_statusText != text) {
        m_statusText = text;
        Q_EMIT statusTextChanged();
    }
}

void SteamGridDB::makeRequest(const QUrl &url, const QString &apiKey, const std::function<void(const QJsonArray &)> &callback)
{
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
    req.setRawHeader(QByteArrayLiteral("Authorization"), QByteArrayLiteral("Bearer ") + apiKey.toUtf8());

    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT error(reply->errorString());
            setBusy(false);
            return;
        }

        auto doc = QJsonDocument::fromJson(reply->readAll());
        auto obj = doc.object();
        if (!obj.value(QStringLiteral("success")).toBool(false)) {
            auto errors = obj.value(QStringLiteral("errors")).toArray();
            QString errMsg = errors.isEmpty() ? tr("Unknown API error") : errors.first().toString();
            Q_EMIT error(errMsg);
            setBusy(false);
            return;
        }

        callback(obj.value(QStringLiteral("data")).toArray());
    });
}

void SteamGridDB::fetchArt(int gameId,
                           const QString &apiKey,
                           const QString &type,
                           const QString &query,
                           const QString &status,
                           std::function<void(const QVariantList &)> emitSignal)
{
    if (busy() || gameId <= 0 || apiKey.isEmpty())
        return;

    setBusy(true);
    setStatusText(status);

    auto urlStr = QStringLiteral("https://www.steamgriddb.com/api/v2/%1/game/%2").arg(type, QString::number(gameId));
    if (!query.isEmpty())
        urlStr += QLatin1Char('?') + query;

    makeRequest(QUrl(urlStr), apiKey, [this, emitSignal](const QJsonArray &data) {
        QVariantList items;
        for (const auto &val : data) {
            auto obj = val.toObject();
            QVariantMap item;
            item[QStringLiteral("id")] = obj.value(QStringLiteral("id")).toInt();
            item[QStringLiteral("url")] = obj.value(QStringLiteral("url")).toString();
            item[QStringLiteral("thumb")] = obj.value(QStringLiteral("thumb")).toString();
            item[QStringLiteral("score")] = obj.value(QStringLiteral("score")).toInt();
            item[QStringLiteral("style")] = obj.value(QStringLiteral("style")).toString();
            auto author = obj.value(QStringLiteral("author")).toObject();
            item[QStringLiteral("author")] = author.value(QStringLiteral("name")).toString();
            items.append(item);
        }
        setBusy(false);
        emitSignal(items);
    });
}

void SteamGridDB::searchGames(const QString &query, const QString &apiKey)
{
    if (busy() || query.isEmpty() || apiKey.isEmpty())
        return;

    setBusy(true);
    setStatusText(tr("Searching SteamGridDB…"));

    QUrl url(QStringLiteral("https://www.steamgriddb.com/api/v2/search/autocomplete/%1").arg(QString::fromUtf8(QUrl::toPercentEncoding(query))));
    makeRequest(url, apiKey, [this](const QJsonArray &data) {
        QVariantList games;
        for (const auto &val : data) {
            auto obj = val.toObject();
            QVariantMap game;
            game[QStringLiteral("id")] = obj.value(QStringLiteral("id")).toInt();
            game[QStringLiteral("name")] = obj.value(QStringLiteral("name")).toString();
            game[QStringLiteral("verified")] = obj.value(QStringLiteral("verified")).toBool();
            games.append(game);
        }
        setBusy(false);
        Q_EMIT searchFinished(games);
    });
}

void SteamGridDB::fetchGrids(int gameId, const QString &apiKey)
{
    fetchArt(gameId, apiKey, QStringLiteral("grids"), {}, tr("Fetching grids…"), [this](const QVariantList &items) {
        Q_EMIT gridsFinished(items);
    });
}

void SteamGridDB::fetchHeroes(int gameId, const QString &apiKey)
{
    fetchArt(gameId, apiKey, QStringLiteral("heroes"), {}, tr("Fetching heroes…"), [this](const QVariantList &items) {
        Q_EMIT heroesFinished(items);
    });
}

void SteamGridDB::fetchIcons(int gameId, const QString &apiKey)
{
    fetchArt(gameId,
             apiKey,
             QStringLiteral("icons"),
             QStringLiteral("dimensions=64,128,256,512&mimes=image/png"),
             tr("Fetching icons…"),
             [this](const QVariantList &items) {
                 Q_EMIT iconsFinished(items);
             });
}

void SteamGridDB::fetchLogos(int gameId, const QString &apiKey)
{
    fetchArt(gameId, apiKey, QStringLiteral("logos"), {}, tr("Fetching logos…"), [this](const QVariantList &items) {
        Q_EMIT logosFinished(items);
    });
}

void SteamGridDB::downloadImage(const QString &url, const QString &savePath)
{
    if (busy() || url.isEmpty())
        return;

    setBusy(true);
    setStatusText(tr("Downloading…"));

    QDir().mkpath(QFileInfo(savePath).path());

    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));

    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0)
            Q_EMIT downloadProgress(static_cast<double>(received) / static_cast<double>(total));
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, savePath]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT error(reply->errorString());
            setBusy(false);
            return;
        }

        QFile file(savePath);
        if (!file.open(QIODevice::WriteOnly)) {
            Q_EMIT error(tr("Failed to write file"));
            setBusy(false);
            return;
        }
        file.write(reply->readAll());
        file.close();

        Q_EMIT downloadFinished(savePath);
        setStatusText(tr("Downloaded!"));
        setBusy(false);
    });
}

void SteamGridDB::autoMakeRequest(const QUrl &url, std::function<void(const QJsonArray &)> onSuccess)
{
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
    req.setRawHeader(QByteArrayLiteral("Authorization"), QByteArrayLiteral("Bearer ") + m_autoApiKey.toUtf8());

    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            autoFinish();
            return;
        }

        auto doc = QJsonDocument::fromJson(reply->readAll());
        auto obj = doc.object();
        if (!obj.value(QStringLiteral("success")).toBool(false)) {
            onSuccess(QJsonArray());
            return;
        }

        onSuccess(obj.value(QStringLiteral("data")).toArray());
    });
}

void SteamGridDB::autoDownloadFile(const QString &imgUrl, const QString &suffix, std::function<void(const QString &)> onDone)
{
    QString ext = imgUrl.section(QLatin1Char('.'), -1);
    int qIdx = ext.indexOf(QLatin1Char('?'));
    if (qIdx >= 0)
        ext = ext.left(qIdx);
    if (ext.isEmpty())
        ext = QStringLiteral("png");

    QString savePath = m_autoAssetsPath + QLatin1Char('/') + m_autoSafeName + QLatin1Char('_') + suffix + QLatin1Char('.') + ext;
    QDir().mkpath(m_autoAssetsPath);

    QNetworkRequest req{QUrl(imgUrl)};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));

    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, savePath, onDone]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            onDone(QString());
            return;
        }

        QFile file(savePath);
        if (!file.open(QIODevice::WriteOnly)) {
            onDone(QString());
            return;
        }
        file.write(reply->readAll());
        file.close();
        onDone(savePath);
    });
}

void SteamGridDB::initAutoDownload(const QString &gameName, const QString &assetsPath, const QString &apiKey)
{
    m_autoBusy = true;
    Q_EMIT autoDownloadingChanged();
    m_autoApiKey = apiKey;
    m_autoAssetsPath = assetsPath;
    m_autoSafeName = QString(gameName).replace(QRegularExpression(QStringLiteral("[^a-zA-Z0-9_-]")), QStringLiteral("_")).toLower();
    m_autoGameId = 0;
    m_autoIconPath.clear();
    m_autoGridPath.clear();
    m_autoHeroPath.clear();
    m_autoLogoPath.clear();
}

void SteamGridDB::autoDownloadAll(const QString &gameName, const QString &assetsPath, const QString &apiKey)
{
    if (m_autoBusy)
        return;

    if (apiKey.isEmpty()) {
        autoDownloadFromBottles(gameName, assetsPath);
        return;
    }

    initAutoDownload(gameName, assetsPath, apiKey);
    Q_EMIT autoDownloadProgress(tr("Searching…"));

    QUrl url(QStringLiteral("https://www.steamgriddb.com/api/v2/search/autocomplete/%1").arg(QString::fromUtf8(QUrl::toPercentEncoding(gameName))));
    autoMakeRequest(url, [this](const QJsonArray &data) {
        if (data.isEmpty()) {
            autoFinish();
            return;
        }
        m_autoGameId = data.first().toObject().value(QStringLiteral("id")).toInt();
        autoRunStep(0);
    });
}

void SteamGridDB::autoDownloadAllBySteamId(int steamId, const QString &gameName, const QString &assetsPath, const QString &apiKey)
{
    if (m_autoBusy || steamId <= 0)
        return;

    if (apiKey.isEmpty()) {
        autoDownloadFromBottles(gameName, assetsPath);
        return;
    }

    initAutoDownload(gameName, assetsPath, apiKey);
    Q_EMIT autoDownloadProgress(tr("Looking up game…"));

    QUrl url(QStringLiteral("https://www.steamgriddb.com/api/v2/games/steam/%1").arg(steamId));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
    req.setRawHeader(QByteArrayLiteral("Authorization"), QByteArrayLiteral("Bearer ") + m_autoApiKey.toUtf8());

    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            autoFinish();
            return;
        }
        auto obj = QJsonDocument::fromJson(reply->readAll()).object();
        if (!obj.value(QStringLiteral("success")).toBool()) {
            autoFinish();
            return;
        }
        m_autoGameId = obj.value(QStringLiteral("data")).toObject().value(QStringLiteral("id")).toInt();
        if (m_autoGameId <= 0) {
            autoFinish();
            return;
        }
        autoRunStep(0);
    });
}

void SteamGridDB::autoDownloadAllById(int gameId, const QString &gameName, const QString &assetsPath, const QString &apiKey)
{
    if (m_autoBusy || gameId <= 0)
        return;

    initAutoDownload(gameName, assetsPath, apiKey);
    m_autoGameId = gameId;
    Q_EMIT autoDownloadProgress(tr("Downloading…"));
    autoRunStep(0);
}

void SteamGridDB::autoRunStep(int step)
{
    struct Step {
        const char *artType;
        const char *urlPath;
        const char *suffix;
        const char *query; // nullptr = no query string
        QString SteamGridDB::*member;
    };
    static const Step steps[] = {
        {"icon", "icons", "icon", "dimensions=64,128,256,512&mimes=image/png", &SteamGridDB::m_autoIconPath},
        {"grid", "grids", "grid", nullptr, &SteamGridDB::m_autoGridPath},
        {"hero", "heroes", "hero", nullptr, &SteamGridDB::m_autoHeroPath},
        {"logo", "logos", "logo", nullptr, &SteamGridDB::m_autoLogoPath},
    };
    constexpr int kStepCount = static_cast<int>(std::size(steps));

    if (step >= kStepCount) {
        autoFinish();
        return;
    }

    const auto &s = steps[step];
    Q_EMIT autoDownloadProgress(tr("Downloading %1…").arg(QLatin1String(s.artType)));

    auto urlStr = QStringLiteral("https://www.steamgriddb.com/api/v2/%1/game/%2").arg(QLatin1String(s.urlPath), QString::number(m_autoGameId));
    if (s.query)
        urlStr += QLatin1Char('?') + QLatin1String(s.query);

    auto member = s.member;
    auto suffix = QString::fromLatin1(s.suffix);

    autoMakeRequest(QUrl(urlStr), [this, step, member, suffix](const QJsonArray &data) {
        if (data.isEmpty()) {
            autoRunStep(step + 1);
            return;
        }
        QString imgUrl = data.first().toObject().value(QStringLiteral("url")).toString();
        autoDownloadFile(imgUrl, suffix, [this, step, member](const QString &path) {
            this->*member = path;
            autoRunStep(step + 1);
        });
    });
}

void SteamGridDB::autoFinish()
{
    m_autoBusy = false;
    Q_EMIT autoDownloadingChanged();
    Q_EMIT autoDownloadFinished(m_autoGameId, m_autoIconPath, m_autoGridPath, m_autoHeroPath, m_autoLogoPath);
}

void SteamGridDB::fetchBottlesArt(const QString &gameName, std::function<void(const QString &)> callback)
{
    QUrl url(QStringLiteral("https://steamgrid.usebottles.com/api/search/%1").arg(QString::fromUtf8(QUrl::toPercentEncoding(gameName))));

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));

    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            setBusy(false);
            callback(QString());
            return;
        }
        auto raw = QString::fromUtf8(reply->readAll()).trimmed();
        if (raw.size() >= 2 && raw.startsWith(QLatin1Char('"')) && raw.endsWith(QLatin1Char('"')))
            raw = raw.mid(1, raw.size() - 2);
        callback(raw);
    });
}

void SteamGridDB::autoDownloadFromBottles(const QString &gameName, const QString &assetsPath)
{
    if (m_autoBusy)
        return;

    m_autoBusy = true;
    Q_EMIT autoDownloadingChanged();
    m_autoAssetsPath = assetsPath;
    m_autoSafeName = QString(gameName).replace(QRegularExpression(QStringLiteral("[^a-zA-Z0-9_-]")), QStringLiteral("_")).toLower();
    m_autoGameId = 0;
    m_autoIconPath.clear();
    m_autoGridPath.clear();
    m_autoHeroPath.clear();
    m_autoLogoPath.clear();

    Q_EMIT autoDownloadProgress(tr("Searching cover art…"));

    // Fetch grid only — use same image for both grid and hero
    fetchBottlesArt(m_autoSafeName, [this](const QString &imgUrl) {
        if (imgUrl.isEmpty()) {
            m_autoBusy = false;
            Q_EMIT autoDownloadingChanged();
            Q_EMIT autoDownloadFinished(0, QString(), QString(), QString(), QString());
            return;
        }
        autoDownloadFile(imgUrl, QStringLiteral("grid"), [this](const QString &gridPath) {
            m_autoGridPath = gridPath;
            // Copy grid as hero
            QString ext = gridPath.section(QLatin1Char('.'), -1);
            m_autoHeroPath = m_autoAssetsPath + QLatin1Char('/') + m_autoSafeName + QStringLiteral("_hero.") + ext;
            // QFile::copy won't overwrite, so clear any stale hero first.
            QFile::remove(m_autoHeroPath);
            QFile::copy(gridPath, m_autoHeroPath);

            m_autoBusy = false;
            Q_EMIT autoDownloadingChanged();
            Q_EMIT autoDownloadFinished(0, QString(), m_autoGridPath, m_autoHeroPath, QString());
        });
    });
}
