#include "updatechecker.h"
#include "protondownloader.h"
#include "protonscanner.h"
#include "settingsmanager.h"
#include "umudownloader.h"
#include <KLocalizedString>
#include <QDir>
#include <QFileInfo>
#include <QNetworkReply>

static const char *umuReleaseUrl = "https://github.com/Open-Wine-Components/umu-launcher/releases/latest";

static QString displayName(const QString &key)
{
    if (key == QLatin1String("ge"))
        return QStringLiteral("GE Proton");
    if (key == QLatin1String("cachyos"))
        return QStringLiteral("Proton-CachyOS");
    if (key == QLatin1String("em"))
        return QStringLiteral("Proton-EM");
    if (key == QLatin1String("umu"))
        return QStringLiteral("UMU-Proton");
    if (key == QLatin1String("dw"))
        return QStringLiteral("DW Proton");
    return key;
}

UpdateChecker::UpdateChecker(SettingsManager *settings, ProtonScanner *scanner, ProtonDownloader *proton, UmuDownloader *umu, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_scanner(scanner)
    , m_proton(proton)
    , m_umu(umu)
{
    connect(m_proton, &Downloader::statusTextChanged, this, &UpdateChecker::progressChanged);
    connect(m_proton, &Downloader::progressChanged, this, &UpdateChecker::progressChanged);
    connect(m_umu, &Downloader::statusTextChanged, this, &UpdateChecker::progressChanged);
    connect(m_umu, &Downloader::progressChanged, this, &UpdateChecker::progressChanged);

    connect(m_proton, &ProtonDownloader::finished, this, [this](const QString &path) {
        if (!m_applying || m_activeKind != QLatin1String("proton"))
            return;
        const QString local = m_scanner->localProtonPath();
        if (m_settings->defaultProtonPath().startsWith(local + QLatin1Char('/')) && !path.isEmpty())
            m_settings->setDefaultProtonPath(path);
        applyNext();
    });
    connect(m_umu, &UmuDownloader::finished, this, [this](const QString &) {
        if (m_applying && m_activeKind == QLatin1String("umu"))
            applyNext();
    });
    auto failed = [this]() {
        if (m_applying)
            applyNext();
    };
    connect(m_proton, &ProtonDownloader::error, this, failed);
    connect(m_umu, &UmuDownloader::error, this, failed);
}

QString UpdateChecker::statusText() const
{
    if (m_activeKind == QLatin1String("umu"))
        return m_umu->statusText();
    return m_proton->statusText();
}

double UpdateChecker::progress() const
{
    if (m_activeKind == QLatin1String("umu"))
        return m_umu->progress();
    return m_proton->progress();
}

void UpdateChecker::fetchTag(const QString &releaseUrl, std::function<void(const QString &)> done)
{
    QNetworkRequest req{QUrl(releaseUrl)};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Vermouth"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, done]() {
        reply->deleteLater();
        QString tag;
        if (reply->error() == QNetworkReply::NoError) {
            const QString path = reply->header(QNetworkRequest::LocationHeader).toUrl().path();
            tag = path.mid(path.lastIndexOf(QLatin1Char('/')) + 1);
        }
        done(tag);
    });
}

void UpdateChecker::addUpdate(const QVariantMap &update)
{
    if (m_settings->skippedUpdates().contains(update.value(QStringLiteral("skipKey")).toString()))
        return;
    m_updates.append(update);
}

void UpdateChecker::check()
{
    if (m_applying || m_pendingChecks > 0)
        return;
    m_updates.clear();

    const QString localProton = m_scanner->localProtonPath();

    // umu-launcher, when it's the copy Vermouth downloaded
    const QString umuPath = m_settings->umuPath();
    if (SettingsManager::isDownloadedUmu(umuPath) && QFileInfo::exists(umuPath)) {
        const QString installed = ProtonDownloader::installedTag(QFileInfo(umuPath).absolutePath());
        ++m_pendingChecks;
        fetchTag(QString::fromLatin1(umuReleaseUrl), [this, installed](const QString &latest) {
            if (!latest.isEmpty() && latest != installed)
                addUpdate({{QStringLiteral("kind"), QStringLiteral("umu")},
                           {QStringLiteral("key"), QStringLiteral("umu")},
                           {QStringLiteral("name"), QStringLiteral("umu-launcher")},
                           {QStringLiteral("installed"), installed.isEmpty() ? i18n("unknown version") : installed},
                           {QStringLiteral("latest"), latest},
                           {QStringLiteral("skipKey"), QString(QStringLiteral("umu:") + latest)}});
            if (--m_pendingChecks == 0 && !m_updates.isEmpty()) {
                Q_EMIT updatesChanged();
                Q_EMIT updatesAvailable();
            }
        });
    }

    // the default Proton build, when it lives in Vermouth's own Proton folder
    const QString defaultProton = m_settings->defaultProtonPath();
    if (m_settings->defaultRuntimeType() == QLatin1String("proton") && defaultProton.startsWith(localProton + QLatin1Char('/'))) {
        const QString folder = defaultProton.mid(localProton.size() + 1).section(QLatin1Char('/'), 0, 0);
        const ProtonBuildConfig *config = ProtonDownloader::configForFolder(folder);
        if (config) {
            const QString marker = ProtonDownloader::installedTag(localProton + QLatin1Char('/') + folder);
            const QString installed = marker.isEmpty() ? folder : marker;
            const QString key = QString::fromLatin1(config->key);
            ++m_pendingChecks;
            fetchTag(QString::fromLatin1(config->releaseUrl), [this, installed, key](const QString &latest) {
                if (!latest.isEmpty() && installed != latest && !installed.contains(latest, Qt::CaseInsensitive))
                    addUpdate({{QStringLiteral("kind"), QStringLiteral("proton")},
                               {QStringLiteral("key"), key},
                               {QStringLiteral("name"), displayName(key)},
                               {QStringLiteral("installed"), installed},
                               {QStringLiteral("latest"), latest},
                               {QStringLiteral("skipKey"), QString(QStringLiteral("proton:") + key + QLatin1Char(':') + latest)}});
                if (--m_pendingChecks == 0 && !m_updates.isEmpty()) {
                    Q_EMIT updatesChanged();
                    Q_EMIT updatesAvailable();
                }
            });
        }
    }
}

void UpdateChecker::skipUpdates()
{
    for (const QVariant &u : std::as_const(m_updates))
        m_settings->skipUpdate(u.toMap().value(QStringLiteral("skipKey")).toString());
    m_updates.clear();
    Q_EMIT updatesChanged();
}

void UpdateChecker::applyUpdates()
{
    if (m_applying || m_updates.isEmpty())
        return;
    m_queue = m_updates;
    m_applying = true;
    Q_EMIT applyingChanged();
    applyNext();
}

void UpdateChecker::applyNext()
{
    if (m_queue.isEmpty()) {
        m_applying = false;
        m_activeKind.clear();
        m_updates.clear();
        Q_EMIT applyingChanged();
        Q_EMIT updatesChanged();
        Q_EMIT applyFinished();
        return;
    }
    const QVariantMap next = m_queue.takeFirst().toMap();
    m_activeKind = next.value(QStringLiteral("kind")).toString();
    Q_EMIT progressChanged();
    if (m_activeKind == QLatin1String("umu"))
        m_umu->downloadLatest();
    else
        m_proton->downloadLatest(next.value(QStringLiteral("key")).toString());
}
