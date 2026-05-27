#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <functional>

class SteamGridDB : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(bool autoDownloading READ autoDownloading NOTIFY autoDownloadingChanged)

public:
    explicit SteamGridDB(QObject *parent = nullptr);

    bool busy() const;
    QString statusText() const;
    bool autoDownloading() const
    {
        return m_autoBusy;
    }

    Q_INVOKABLE void searchGames(const QString &query, const QString &apiKey);
    Q_INVOKABLE void autoDownloadFromBottles(const QString &gameName, const QString &assetsPath);
    Q_INVOKABLE void fetchGrids(int gameId, const QString &apiKey);
    Q_INVOKABLE void fetchHeroes(int gameId, const QString &apiKey);
    Q_INVOKABLE void fetchIcons(int gameId, const QString &apiKey);
    Q_INVOKABLE void fetchLogos(int gameId, const QString &apiKey);
    Q_INVOKABLE void downloadImage(const QString &url, const QString &savePath);
    Q_INVOKABLE void autoDownloadAll(const QString &gameName, const QString &assetsPath, const QString &apiKey);
    Q_INVOKABLE void autoDownloadAllById(int gameId, const QString &gameName, const QString &assetsPath, const QString &apiKey);
    Q_INVOKABLE void autoDownloadAllBySteamId(int steamId, const QString &gameName, const QString &assetsPath, const QString &apiKey);

Q_SIGNALS:
    void busyChanged();
    void statusTextChanged();
    void searchFinished(const QVariantList &games);
    void gridsFinished(const QVariantList &items);
    void heroesFinished(const QVariantList &items);
    void iconsFinished(const QVariantList &items);
    void logosFinished(const QVariantList &items);
    void downloadFinished(const QString &savePath);
    void downloadProgress(double progress);
    void error(const QString &message);
    void autoDownloadingChanged();
    void autoDownloadProgress(const QString &step);
    void autoDownloadFinished(int gameId, const QString &iconPath, const QString &gridPath, const QString &heroPath, const QString &logoPath);

private:
    void setBusy(bool busy);
    void setStatusText(const QString &text);
    void makeRequest(const QUrl &url, const QString &apiKey, const std::function<void(const QJsonArray &)> &callback);
    void fetchBottlesArt(const QString &gameName, std::function<void(const QString &)> callback);
    void fetchArt(int gameId,
                  const QString &apiKey,
                  const QString &type,
                  const QString &query,
                  const QString &status,
                  std::function<void(const QVariantList &)> emitSignal);

    void autoMakeRequest(const QUrl &url, std::function<void(const QJsonArray &)> onSuccess);
    void autoDownloadFile(const QString &imgUrl, const QString &suffix, std::function<void(const QString &)> onDone);
    void initAutoDownload(const QString &gameName, const QString &assetsPath, const QString &apiKey);
    void autoRunStep(int step);
    void autoFinish();

    QNetworkAccessManager m_nam;
    bool m_busy = false;
    QString m_statusText;

    bool m_autoBusy = false;
    QString m_autoApiKey;
    QString m_autoAssetsPath;
    QString m_autoSafeName;
    int m_autoGameId = 0;
    QString m_autoIconPath;
    QString m_autoGridPath;
    QString m_autoHeroPath;
    QString m_autoLogoPath;
};
