#include "backupmanager.h"
#include "appmodel.h"
#include "settingsmanager.h"
#include <KLocalizedString>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSettings>
#include <QUrl>

static QString localPath(const QString &pathOrUrl)
{
    const QUrl url(pathOrUrl);
    return url.isLocalFile() ? url.toLocalFile() : pathOrUrl;
}

static bool copyOver(const QString &from, const QString &to)
{
    QFile::remove(to);
    return QFile::copy(from, to);
}

BackupManager::BackupManager(AppModel *appModel, SettingsManager *settings, QObject *parent)
    : QObject(parent)
    , m_appModel(appModel)
    , m_settings(settings)
{
}

QString BackupManager::backupSettings(const QString &destination) const
{
    m_settings->sync();
    const QString source = m_settings->filePath();
    if (!QFile::exists(source))
        return i18n("There are no saved settings to back up yet.");
    return copyOver(source, localPath(destination)) ? QString() : i18n("Could not write %1.", localPath(destination));
}

QString BackupManager::restoreSettings(const QString &sourceUrl)
{
    const QString source = localPath(sourceUrl);
    QSettings candidate(source, QSettings::IniFormat);
    if (candidate.status() != QSettings::NoError || candidate.allKeys().isEmpty())
        return i18n("%1 is not a Vermouth settings backup.", source);

    m_settings->sync();
    const QString target = m_settings->filePath();
    if (QFile::exists(target))
        copyOver(target, target + QStringLiteral(".bak"));
    if (!copyOver(source, target))
        return i18n("Could not write %1.", target);
    m_settings->sync();
    return {};
}

QString BackupManager::backupGames(const QString &destination) const
{
    m_appModel->save();
    const QString source = m_appModel->gamesFilePath();
    if (!QFile::exists(source))
        return i18n("There is no game list to back up yet.");
    return copyOver(source, localPath(destination)) ? QString() : i18n("Could not write %1.", localPath(destination));
}

QString BackupManager::restoreGames(const QString &sourceUrl)
{
    const QString source = localPath(sourceUrl);
    QFile in(source);
    if (!in.open(QIODevice::ReadOnly))
        return i18n("Could not read %1.", source);
    const QByteArray raw = in.readAll();
    in.close();

    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    if (!doc.isArray())
        return i18n("%1 is not a Vermouth game list backup.", source);
    for (const QJsonValue &v : doc.array()) {
        if (!v.isObject() || !v.toObject().contains(QStringLiteral("name")))
            return i18n("%1 is not a Vermouth game list backup.", source);
    }

    m_appModel->save();
    const QString target = m_appModel->gamesFilePath();
    if (QFile::exists(target))
        copyOver(target, target + QStringLiteral(".bak"));

    QSaveFile out(target);
    if (!out.open(QIODevice::WriteOnly) || out.write(raw) != raw.size() || !out.commit())
        return i18n("Could not write %1.", target);
    m_appModel->load();
    return {};
}
