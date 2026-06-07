#include "umudownloader.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryFile>

UmuDownloader::UmuDownloader(QObject *parent)
    : Downloader(parent)
{
}

void UmuDownloader::setInstallPath(const QString &path)
{
    m_installPath = path;
}

void UmuDownloader::downloadLatest()
{
    if (busy())
        return;

    setBusy(true);
    setStatusText(tr("Checking latest umu-launcher release…"));
    setProgress(0.0);

    QNetworkRequest req(QUrl(QStringLiteral("https://github.com/Open-Wine-Components/umu-launcher/releases/latest")));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    auto *reply = nam().get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReleaseFetched(reply);
    });
}

void UmuDownloader::onReleaseFetched(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        setStatusText(tr("Failed to fetch release info: %1").arg(reply->errorString()));
        setBusy(false);
        Q_EMIT error(reply->errorString());
        return;
    }

    QUrl redirectUrl = reply->header(QNetworkRequest::LocationHeader).toUrl();
    QString path = redirectUrl.path();
    QString tagName = path.mid(path.lastIndexOf(QLatin1Char('/')) + 1);

    if (tagName.isEmpty()) {
        setStatusText(tr("Could not determine latest version"));
        setBusy(false);
        Q_EMIT error(QStringLiteral("No tag found in redirect URL"));
        return;
    }

    QString downloadUrl = QStringLiteral("https://github.com/Open-Wine-Components/umu-launcher/releases/download/%1/umu-run").arg(tagName);

    setStatusText(tr("Downloading umu-launcher %1…").arg(tagName));

    QNetworkRequest dlReq{QUrl(downloadUrl)};
    dlReq.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
    dlReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *dlReply = nam().get(dlReq);
    connect(dlReply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0)
            setProgress(static_cast<double>(received) / static_cast<double>(total));
    });
    connect(dlReply, &QNetworkReply::finished, this, [this, dlReply, tagName]() {
        if (dlReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404) {
            dlReply->deleteLater();
            QString tarUrl =
                QStringLiteral("https://github.com/Open-Wine-Components/umu-launcher/releases/download/%1/umu-launcher-%1-zipapp.tar").arg(tagName);
            setStatusText(tr("Downloading umu-launcher %1 (tar)…").arg(tagName));
            auto *tmpFile = new QTemporaryFile(QDir::tempPath() + QStringLiteral("/vermouth-umu-XXXXXX.tar"));
            if (!tmpFile->open()) {
                setStatusText(tr("Failed to create temp file"));
                setBusy(false);
                Q_EMIT error(QStringLiteral("Could not create temporary file"));
                delete tmpFile;
                return;
            }
            QNetworkRequest tarReq{QUrl(tarUrl)};
            tarReq.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
            tarReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
            auto *tarReply = nam().get(tarReq);
            connect(tarReply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
                if (total > 0)
                    setProgress(static_cast<double>(received) / static_cast<double>(total));
            });
            connect(tarReply, &QNetworkReply::readyRead, this, [tarReply, tmpFile]() {
                tmpFile->write(tarReply->readAll());
            });
            connect(tarReply, &QNetworkReply::finished, this, [this, tarReply, tmpFile]() {
                tmpFile->flush();
                tmpFile->close();
                if (tarReply->error() != QNetworkReply::NoError) {
                    setStatusText(tr("Download failed: %1").arg(tarReply->errorString()));
                    setBusy(false);
                    Q_EMIT error(tarReply->errorString());
                    delete tmpFile;
                    tarReply->deleteLater();
                    return;
                }
                setStatusText(tr("Installing…"));
                setProgress(1.0);
                tarReply->deleteLater();
                startTarExtraction(tmpFile);
            });
            return;
        }
        dlReply->deleteLater();
        if (dlReply->error() != QNetworkReply::NoError) {
            setStatusText(tr("Download failed: %1").arg(dlReply->errorString()));
            setBusy(false);
            Q_EMIT error(dlReply->errorString());
            return;
        }
        setStatusText(tr("Installing…"));
        setProgress(1.0);
        QDir().mkpath(m_installPath);
        QString umuBinPath = m_installPath + QStringLiteral("/umu-run");
        QFile binFile(umuBinPath);
        if (!binFile.open(QIODevice::WriteOnly)) {
            setStatusText(tr("Failed to write umu-run"));
            setBusy(false);
            Q_EMIT error(QStringLiteral("Could not write file"));
            return;
        }
        binFile.write(dlReply->readAll());
        binFile.close();
        binFile.setPermissions(binFile.permissions() | QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther);

        setStatusText(tr("umu-launcher installed!"));
        setBusy(false);
        Q_EMIT finished(umuBinPath);
    });
}

void UmuDownloader::startTarExtraction(QTemporaryFile *archiveFile)
{
    delete m_tarProc;
    m_tarProc = new QProcess(this);
    auto *tmpExtractDir = new QString(QDir::tempPath() + QStringLiteral("/vermouth-umu-extract"));
    QDir().mkpath(*tmpExtractDir);

    connect(m_tarProc, &QProcess::finished, this, [this, tmpExtractDir, archiveFile](int exitCode) {
        delete archiveFile;
        if (exitCode != 0) {
            setStatusText(tr("Extraction failed"));
            setBusy(false);
            Q_EMIT error(QStringLiteral("tar extraction failed"));
            QDir(*tmpExtractDir).removeRecursively();
            delete tmpExtractDir;
            return;
        }
        QDir().mkpath(m_installPath);
        QString umuBinPath = m_installPath + QStringLiteral("/umu-run");
        QString foundPath = *tmpExtractDir + QStringLiteral("/umu/umu-run");
        if (QFile::exists(foundPath)) {
            QFile::remove(umuBinPath);
            QFile::copy(foundPath, umuBinPath);
            QFile(umuBinPath).setPermissions(QFileInfo(umuBinPath).permissions() | QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther);
        }
        QDir(*tmpExtractDir).removeRecursively();
        delete tmpExtractDir;

        setStatusText(tr("umu-launcher installed!"));
        setBusy(false);
        Q_EMIT finished(umuBinPath);
    });

    m_tarProc->setProgram(QStringLiteral("tar"));
    m_tarProc->setArguments({QStringLiteral("-xf"), archiveFile->fileName(), QStringLiteral("-C"), *tmpExtractDir});
    m_tarProc->start();
}
