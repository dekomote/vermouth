#pragma once

#include "downloader.h"
#include <QNetworkReply>

class QProcess;
class QTemporaryFile;

struct WineBuildConfig {
    const char *key;
    const char *releaseUrl;
    const char *downloadUrl;
    const char *suffix;
};

static const WineBuildConfig wineBuildConfigs[] = {
    {"wow64",
     "https://github.com/Kron4ek/Wine-Builds/releases/latest",
     "https://github.com/Kron4ek/Wine-Builds/releases/download/%1/wine-%1-%2.tar.xz",
     "amd64-wow64"},
    {"regular",
     "https://github.com/Kron4ek/Wine-Builds/releases/latest",
     "https://github.com/Kron4ek/Wine-Builds/releases/download/%1/wine-%1-%2.tar.xz",
     "amd64"},
    {"tkg",
     "https://github.com/Kron4ek/Wine-Builds/releases/latest",
     "https://github.com/Kron4ek/Wine-Builds/releases/download/%1/wine-%1-%2.tar.xz",
     "staging-tkg-amd64"},
    {"tkg-wow64",
     "https://github.com/Kron4ek/Wine-Builds/releases/latest",
     "https://github.com/Kron4ek/Wine-Builds/releases/download/%1/wine-%1-%2.tar.xz",
     "staging-tkg-amd64-wow64"},
};

class WineDownloader : public Downloader
{
    Q_OBJECT

public:
    explicit WineDownloader(QObject *parent = nullptr);

    void setLocalWinePath(const QString &path);

    Q_INVOKABLE void downloadLatest(const QString &buildType);

Q_SIGNALS:
    void finished(const QString &path);
    void error(const QString &message);

private:
    static const WineBuildConfig *findConfig(const QString &buildType);
    void onReleaseFetched(QNetworkReply *reply, const WineBuildConfig *config);
    void startExtraction(QTemporaryFile *archiveFile);

    QString m_localWinePath;
    QProcess *m_extractProc = nullptr;
};
