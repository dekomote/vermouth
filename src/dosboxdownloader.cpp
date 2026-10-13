#include "dosboxdownloader.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

// Same GitHub flow as UzdoomDownloader: /releases/latest redirect -> tag name -> deterministic asset URL.
static const char *dosboxReleaseUrl = "https://github.com/dosbox-staging/dosbox-staging/releases/latest";
static const char *dosboxDownloadUrl = "https://github.com/dosbox-staging/dosbox-staging/releases/download/%1/dosbox-staging-linux-x86_64-%1.tar.xz";

DosboxDownloader::DosboxDownloader(QObject *parent)
    : Downloader(parent)
{
}

void DosboxDownloader::setInstallPath(const QString &path)
{
    m_installPath = path;
}

QString DosboxDownloader::installPath() const
{
    return m_installPath;
}

void DosboxDownloader::fail(const QString &message)
{
    setStatusText(message);
    setBusy(false);
    Q_EMIT error(message);
}

void DosboxDownloader::downloadLatest()
{
    if (busy())
        return;

    setBusy(true);
    setStatusText(tr("Checking latest release…"));
    setProgress(0.0);

    QNetworkRequest req{QUrl(QLatin1String(dosboxReleaseUrl))};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    auto *reply = nam().get(req, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReleaseFetched(reply);
    });
}

void DosboxDownloader::onReleaseFetched(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        fail(tr("Failed to fetch release info: %1").arg(reply->errorString()));
        return;
    }

    const QString path = reply->header(QNetworkRequest::LocationHeader).toUrl().path();
    const QString tagName = path.mid(path.lastIndexOf(QLatin1Char('/')) + 1);
    if (tagName.isEmpty()) {
        fail(tr("Could not determine latest version"));
        return;
    }

    // Already have this exact version installed?
    const QString binary = m_installPath + QStringLiteral("/dosbox");
    QFile vf(m_installPath + QStringLiteral("/.version"));
    if (QFileInfo::exists(binary) && vf.open(QIODevice::ReadOnly)) {
        if (QString::fromUtf8(vf.readAll()).trimmed() == tagName) {
            setStatusText(tr("Latest version (%1) is already installed").arg(tagName));
            setBusy(false);
            Q_EMIT finished(binary);
            return;
        }
    }

    setStatusText(tr("Downloading DOSBox Staging %1…").arg(tagName));
    startDownload(QUrl(QString::fromLatin1(dosboxDownloadUrl).arg(tagName)), tagName);
}

void DosboxDownloader::startDownload(const QUrl &url, const QString &tagName)
{
    QNetworkRequest dlReq(url);
    dlReq.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
    dlReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *dlReply = nam().get(dlReq, QByteArray());
    connect(dlReply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0)
            setProgress(static_cast<double>(received) / static_cast<double>(total));
    });
    connect(dlReply, &QNetworkReply::finished, this, [this, dlReply, tagName]() {
        dlReply->deleteLater();
        if (dlReply->error() != QNetworkReply::NoError) {
            fail(tr("Download failed: %1").arg(dlReply->errorString()));
            return;
        }
        setStatusText(tr("Installing…"));
        setProgress(1.0);
        QDir().mkpath(m_installPath);
        const QString archivePath = m_installPath + QStringLiteral("/dosbox-staging.tar.xz");
        QFile archive(archivePath);
        if (!archive.open(QIODevice::WriteOnly)) {
            fail(tr("Failed to write DOSBox Staging archive"));
            return;
        }
        archive.write(dlReply->readAll());
        archive.close();
        extract(archivePath, tagName);
    });
}

void DosboxDownloader::extract(const QString &archivePath, const QString &tagName)
{
    const QString tar = QStandardPaths::findExecutable(QStringLiteral("tar"));
    if (tar.isEmpty()) {
        fail(tr("tar is needed to unpack DOSBox Staging"));
        return;
    }

    // Replace the previous install wholesale, so files dropped by a newer release don't linger.
    const QString dir = m_installPath;
    for (const QString &entry : QDir(dir).entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        QDir(dir + QLatin1Char('/') + entry).removeRecursively();
    QFile::remove(dir + QStringLiteral("/dosbox"));

    // The tarball has a single versioned top folder. Strip it so the binary always lands at <install>/dosbox.
    auto *proc = new QProcess(this);
    connect(proc, &QProcess::finished, this, [this, proc, archivePath, tagName](int exitCode, QProcess::ExitStatus status) {
        proc->deleteLater();
        QFile::remove(archivePath);
        const QString binary = m_installPath + QStringLiteral("/dosbox");
        if (status != QProcess::NormalExit || exitCode != 0 || !QFileInfo::exists(binary)) {
            fail(tr("Failed to unpack DOSBox Staging"));
            return;
        }
        QFile::setPermissions(binary, QFile(binary).permissions() | QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther);

        QFile vf(m_installPath + QStringLiteral("/.version"));
        if (vf.open(QIODevice::WriteOnly | QIODevice::Truncate))
            vf.write(tagName.toUtf8());

        setStatusText(tr("DOSBox Staging installed!"));
        setBusy(false);
        Q_EMIT finished(binary);
    });
    proc->start(tar, {QStringLiteral("-xf"), archivePath, QStringLiteral("--strip-components=1"), QStringLiteral("-C"), dir});
}
