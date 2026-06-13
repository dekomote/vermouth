#pragma once

#include "downloader.h"
#include <QStringList>
#include <QVariantList>
#include <functional>

class QNetworkReply;

class GogClient : public Downloader
{
    Q_OBJECT
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(QString username READ username NOTIFY usernameChanged)

public:
    explicit GogClient(QObject *parent = nullptr);

    bool authenticated() const;
    QString username() const;
    QString refreshToken() const;

    void setRefreshToken(const QString &token);

    Q_INVOKABLE QString loginUrl() const;
    Q_INVOKABLE QString extractCode(const QString &redirectUrl) const;
    Q_INVOKABLE void authenticateWithRedirect(const QString &redirectUrl);
    Q_INVOKABLE void refreshSession();
    Q_INVOKABLE void logout();

    Q_INVOKABLE void fetchLibrary(const QString &search = {}, int page = 1);
    Q_INVOKABLE void fetchDownloadInfo(const QString &gameId, bool preferLinux);
    Q_INVOKABLE void fetchGameSize(const QString &gameId, bool preferLinux);

Q_SIGNALS:
    void authenticatedChanged();
    void usernameChanged();
    void loginSucceeded();
    void loginFailed(const QString &message);
    void refreshTokenObtained(const QString &token);
    void libraryFetched(const QVariantList &items, int totalPages, int page);
    void downloadInfoReady(const QString &gameId, const QStringList &urls, bool isWindows);
    void sizeFetched(const QString &gameId, double bytes);
    void error(const QString &message);

private:
    void setAuthenticated(bool a);
    void setUsername(const QString &name);
    void parseTokenResponse(const QByteArray &data, bool fromRefresh);
    void fetchUserInfo();
    void authedGet(const QUrl &url, std::function<void(const QByteArray &)> onData, std::function<void(const QString &)> onError, bool allowRetry = true);
    void refreshAccessToken(std::function<void(bool)> done);
    void resolveNextDownlink(const QString &gameId, QStringList pending, QStringList resolved, bool isWindows);

    QString m_accessToken;
    QString m_refreshToken;
    QString m_username;
    bool m_authenticated = false;
};
