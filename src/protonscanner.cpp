#include "protonscanner.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>

ProtonScanner::ProtonScanner(QObject *parent)
    : QObject(parent)
{
    QDir().mkpath(localProtonPath());
}

QStringList ProtonScanner::steamPaths() const {
    QStringList paths;
    QString home = QDir::homePath();

    QStringList candidates = {
        home + QStringLiteral("/.steam/steam"),
        home + QStringLiteral("/.steam/root"),
        home + QStringLiteral("/.local/share/Steam"),
        home + QStringLiteral("/.var/app/com.valvesoftware.Steam/.steam/steam"),
        home + QStringLiteral("/.var/app/com.valvesoftware.Steam/.local/share/Steam"),
    };

    for (const auto &p : candidates) {
        QFileInfo fi(p);
        if (fi.exists()) {
            QString resolved = fi.canonicalFilePath();
            if (!resolved.isEmpty() && !paths.contains(resolved))
                paths << resolved;
        }
    }

    QStringList vdfCandidates = {
        home + QStringLiteral("/.local/share/Steam/config/libraryfolders.vdf"),
        home + QStringLiteral("/.steam/steam/config/libraryfolders.vdf"),
        home + QStringLiteral("/.var/app/com.valvesoftware.Steam/.local/share/Steam/config/libraryfolders.vdf"),
    };

    for (const auto &vdfPath : vdfCandidates) {
        QFile vdf(vdfPath);
        if (!vdf.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;

        QTextStream in(&vdf);
        QRegularExpression pathRx(QStringLiteral("\"path\"\\s+\"([^\"]+)\""));
        while (!in.atEnd()) {
            QString line = in.readLine();
            auto match = pathRx.match(line);
            if (match.hasMatch()) {
                QString libPath = match.captured(1);
                QFileInfo fi(libPath);
                if (fi.exists()) {
                    QString resolved = fi.canonicalFilePath();
                    if (!resolved.isEmpty() && !paths.contains(resolved))
                        paths << resolved;
                }
            }
        }
        break;
    }

    return paths;
}

QStringList ProtonScanner::findProtonVersions() const {
    QStringList result;

    for (const auto &steamRoot : steamPaths()) {
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

    result.removeDuplicates();
    result.sort();
    return result;
}

QString ProtonScanner::homePath() const {
    return QDir::homePath();
}

QString ProtonScanner::prefixBasePath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/prefixes");
}

QString ProtonScanner::localProtonPath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/protons");
}

QStringList ProtonScanner::findExistingPrefixes() const {
    QStringList result;

    for (const auto &steamRoot : steamPaths()) {
        QDir prefixDir(steamRoot + QStringLiteral("/steamapps/compatdata"));
        if (!prefixDir.exists()) continue;

        for (const auto &entry : prefixDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QString path = prefixDir.absoluteFilePath(entry);
            if (QFileInfo::exists(path + QStringLiteral("/pfx")))
                result << path;
        }
    }

    QString dataHome = ProtonScanner::prefixBasePath();
    QDir customDir(dataHome);
    if (customDir.exists()) {
        for (const auto &entry : customDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
            result << customDir.absoluteFilePath(entry);
    }

    result.removeDuplicates();
    result.sort();
    return result;
}
