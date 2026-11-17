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
    const char *latestName;
};

static const ProtonBuildConfig protonBuildConfigs[] = {
    {"ge",
     "https://github.com/GloriousEggroll/proton-ge-custom/releases/latest",
     "https://github.com/GloriousEggroll/proton-ge-custom/releases/download/%1/%2",
     "%1-x86_64.tar.gz",
     "-xzf",
     ".tar.gz",
     "GE-Proton-Latest"},
    {"cachyos",
     "https://github.com/CachyOS/proton-cachyos/releases/latest",
     "https://github.com/CachyOS/proton-cachyos/releases/download/%1/%2",
     "proton-%1-x86_64.tar.xz",
     "-xf",
     ".tar.xz",
     "Proton-CachyOS-Latest"},
    {"em",
     "https://github.com/BananaWorks07/Proton/releases/latest",
     "https://github.com/BananaWorks07/Proton/releases/download/%1/%2",
     "proton-%1.tar.xz",
     "-xf",
     ".tar.xz",
     "Proton-EM-Latest"},
    {"umu",
     "https://github.com/Open-Wine-Components/umu-proton/releases/latest",
     "https://github.com/Open-Wine-Components/umu-proton/releases/download/%1/%2",
     "%1.tar.gz",
     "-xzf",
     ".tar.gz",
     "UMU-Proton-Latest"},
    {"dw",
     "https://github.com/dawn-winery/dwproton-mirror/releases/latest",
     "https://github.com/dawn-winery/dwproton-mirror/releases/download/%1/%2",
     "%1-x86_64.tar.xz",
     "-xf",
     ".tar.xz",
     "DW-Proton-Latest"},
};

class ProtonDownloader : public Downloader
{
    Q_OBJECT

public:
    explicit ProtonDownloader(QObject *parent = nullptr);

    void setLocalProtonPath(const QString &path);

    // Release tag recorded when a "Latest" build was installed; empty if the folder has none.
    static QString installedTag(const QString &installDir);
    static void writeInstalledTag(const QString &installDir, const QString &tag);
    static const ProtonBuildConfig *configForFolder(const QString &folderName);
    static const ProtonBuildConfig *configForKey(const QString &key);

    Q_INVOKABLE void downloadLatest(const QString &buildType = QStringLiteral("ge"));

Q_SIGNALS:
    void finished(const QString &path);
    void error(const QString &message);

private:
    void onReleaseFetched(QNetworkReply *reply, const ProtonBuildConfig *config);
    void startExtraction(QTemporaryFile *archiveFile, const QStringList &tarArgs, const ProtonBuildConfig *config, const QString &tag);

    QString m_localProtonPath;
    QProcess *m_extractProc = nullptr;
};
