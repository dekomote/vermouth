#pragma once

#include "downloader.h"
#include <QNetworkReply>

class QProcess;
class QTemporaryFile;

struct ProtonBuildConfig {
    const char *key;
    const char *releaseUrl;
    const char *downloadUrl;
    const char *archivePattern;
    const char *tarFlag;
    const char *tmpExt;
};

static const ProtonBuildConfig protonBuildConfigs[] = {
    {"ge",
     "https://github.com/GloriousEggroll/proton-ge-custom/releases/latest",
     "https://github.com/GloriousEggroll/proton-ge-custom/releases/download/%1/%2",
     "%1-x86_64.tar.gz",
     "-xzf",
     ".tar.gz"},
    {"cachyos",
     "https://github.com/CachyOS/proton-cachyos/releases/latest",
     "https://github.com/CachyOS/proton-cachyos/releases/download/%1/%2",
     "proton-%1-x86_64.tar.xz",
     "-xf",
     ".tar.xz"},
};

class ProtonDownloader : public Downloader
{
    Q_OBJECT

public:
    explicit ProtonDownloader(QObject *parent = nullptr);

    void setLocalProtonPath(const QString &path);

    Q_INVOKABLE void downloadLatest(const QString &buildType = QStringLiteral("ge"));

Q_SIGNALS:
    void finished(const QString &path);
    void error(const QString &message);

private:
    static const ProtonBuildConfig *findConfig(const QString &buildType);
    void onReleaseFetched(QNetworkReply *reply, const ProtonBuildConfig *config);
    void startExtraction(QTemporaryFile *archiveFile, const QStringList &tarArgs);

    QString m_localProtonPath;
    QProcess *m_extractProc = nullptr;
};
