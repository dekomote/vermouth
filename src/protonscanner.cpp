#include "protonscanner.h"
#include "steamlibrary.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

ProtonScanner::ProtonScanner(QObject *parent)
    : QObject(parent)
{
    QDir().mkpath(localProtonPath());
}

QStringList ProtonScanner::findProtonVersions() const
{
    QStringList result;

    for (const auto &steamRoot : SteamLibrary::steamRootPaths()) {
        QDir commonDir(steamRoot + QStringLiteral("/steamapps/common"));
        if (commonDir.exists()) {
            for (const auto &entry : commonDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                if (entry.startsWith(QStringLiteral("Proton"), Qt::CaseInsensitive)) {
                    QString path = commonDir.absoluteFilePath(entry);
                    if (QFileInfo::exists(path + QStringLiteral("/proton")))
                        result << path;
                }
            }
        }

        QDir compatDir(steamRoot + QStringLiteral("/compatibilitytools.d"));
        if (compatDir.exists()) {
            for (const auto &entry : compatDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                QString path = compatDir.absoluteFilePath(entry);
                if (QFileInfo::exists(path + QStringLiteral("/proton")))
                    result << path;
            }
        }
    }

    QDir localDir(localProtonPath());
    if (localDir.exists()) {
        for (const auto &entry : localDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QString path = localDir.absoluteFilePath(entry);
            if (QFileInfo::exists(path + QStringLiteral("/proton")))
                result << path;
        }
    }

    for (const auto &extraPath : m_extraProtonPaths) {
        QDir extraDir(extraPath);
        if (!extraDir.exists())
            continue;
        for (const auto &entry : extraDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QString path = extraDir.absoluteFilePath(entry);
            if (QFileInfo::exists(path + QStringLiteral("/proton")))
                result << path;
        }
    }

    result.removeDuplicates();
    result.sort();
    return result;
}

bool ProtonScanner::isInstalled(const QString &path) const
{
    return !path.isEmpty() && QFileInfo::exists(path + QStringLiteral("/proton"));
}

QString ProtonScanner::homePath() const
{
    return QDir::homePath();
}

void ProtonScanner::setCustomPrefixBasePath(const QString &path)
{
    m_customPrefixBasePath = path;
}

QString ProtonScanner::prefixBasePath() const
{
    if (!m_customPrefixBasePath.isEmpty())
        return m_customPrefixBasePath;
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/prefixes");
}

QString ProtonScanner::winePrefixBasePath() const
{
    return prefixBasePath() + QStringLiteral("/wines");
}

QString ProtonScanner::localProtonPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/protons");
}

QString ProtonScanner::localAssetsPath() const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/assets");
    QDir().mkpath(path);
    return path;
}

void ProtonScanner::setExtraProtonPaths(const QStringList &paths)
{
    m_extraProtonPaths = paths;
}
