#pragma once

#include "downloader.h"
#include <QNetworkReply>

class UzdoomDownloader : public Downloader
{
    Q_OBJECT

public:
    explicit UzdoomDownloader(QObject *parent = nullptr);

    void setInstallPath(const QString &path);
    Q_INVOKABLE QString installPath() const;

    Q_INVOKABLE void downloadLatest();

Q_SIGNALS:
    void finished(const QString &appImagePath);
    void error(const QString &message);

private:
    void onReleaseFetched(QNetworkReply *reply);
    void startDownload(const QUrl &url, const QString &tagName);

    QString m_installPath;
    QString m_versionTag;
};
