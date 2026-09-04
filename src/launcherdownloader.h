#pragma once

#include "downloader.h"
#include <QNetworkReply>

class LauncherDownloader : public Downloader
{
    Q_OBJECT

public:
    explicit LauncherDownloader(QObject *parent = nullptr);

    Q_INVOKABLE QString downloadDir() const;
    Q_INVOKABLE void download(const QString &url, const QString &fileName);

Q_SIGNALS:
    void finished(const QString &filePath);
    void error(const QString &message);
};
