#include "winescanner.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

WineScanner::WineScanner(QObject *parent)
    : QObject(parent)
{
    QDir().mkpath(localWinePath());
}

QVariantList WineScanner::findWineVersions() const
{
    QVariantList result;

    QStringList pathDirs = QString::fromLocal8Bit(qgetenv("PATH")).split(QLatin1Char(':'), Qt::SkipEmptyParts);
    for (const auto &dir : pathDirs) {
        QFileInfo fi(dir + QStringLiteral("/wine"));
        if (fi.exists() && fi.isExecutable()) {
            result.append(QVariantMap{{QStringLiteral("label"), QStringLiteral("System Wine (%1)").arg(fi.absoluteFilePath())},
                                      {QStringLiteral("path"), fi.absoluteFilePath()}});
            break;
        }
    }

    QDir localDir(localWinePath());
    if (localDir.exists()) {
        QStringList entries = localDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const auto &entry : entries) {
            QString wineBin = localDir.absoluteFilePath(entry + QStringLiteral("/bin/wine"));
            if (QFileInfo::exists(wineBin) && QFileInfo(wineBin).isExecutable()) {
                result.append(QVariantMap{{QStringLiteral("label"), entry}, {QStringLiteral("path"), wineBin}});
            }
        }
    }

    return result;
}

bool WineScanner::isInstalled(const QString &path) const
{
    if (path.isEmpty())
        return false;
    QFileInfo fi(path);
    return fi.exists() && fi.isExecutable();
}

QString WineScanner::localWinePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/wines");
}
