#pragma once

#include "downloader.h"
#include <QNetworkReply>

// Downloads the official DOSBox Staging Linux tarball from GitHub and unpacks it.
// The release is self-contained (bundled libs), so the extracted folder is kept whole and
// its "dosbox" binary is what gets launched.
class DosboxDownloader : public Downloader
{
    Q_OBJECT

public:
    explicit DosboxDownloader(QObject *parent = nullptr);

    void setInstallPath(const QString &path);
    Q_INVOKABLE QString installPath() const;

    Q_INVOKABLE void downloadLatest();

Q_SIGNALS:
    void finished(const QString &binaryPath);
    void error(const QString &message);

private:
    void onReleaseFetched(QNetworkReply *reply);
    void startDownload(const QUrl &url, const QString &tagName);
    void extract(const QString &archivePath, const QString &tagName);
    void fail(const QString &message);

    QString m_installPath;
};
