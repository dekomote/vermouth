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
        home + "/.steam/steam",
        home + "/.steam/root",
        home + "/.local/share/Steam",
        home + "/.var/app/com.valvesoftware.Steam/.steam/steam",
        home + "/.var/app/com.valvesoftware.Steam/.local/share/Steam",
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
        home + "/.local/share/Steam/config/libraryfolders.vdf",
        home + "/.steam/steam/config/libraryfolders.vdf",
        home + "/.var/app/com.valvesoftware.Steam/.local/share/Steam/config/libraryfolders.vdf",
    };

    for (const auto &vdfPath : vdfCandidates) {
        QFile vdf(vdfPath);
        if (!vdf.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;

        QTextStream in(&vdf);
        QRegularExpression pathRx("\"path\"\\s+\"([^\"]+)\"");
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
        break; // Only need to parse one vdf
    }

    return paths;
}

QStringList ProtonScanner::findProtonVersions() const {
    QStringList result;

    for (const auto &steamRoot : steamPaths()) {
        // Official Proton in steamapps/common
        QDir commonDir(steamRoot + "/steamapps/common");
        if (commonDir.exists()) {
            for (const auto &entry : commonDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                if (entry.startsWith("Proton", Qt::CaseInsensitive)) {
                    QString path = commonDir.absoluteFilePath(entry);
                    if (QFileInfo::exists(path + "/proton"))
                        result << path;
                }
            }
        }

        // Custom Proton (GE, TKG, etc.) in compatibilitytools.d
        QDir compatDir(steamRoot + "/compatibilitytools.d");
        if (compatDir.exists()) {
            for (const auto &entry : compatDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                QString path = compatDir.absoluteFilePath(entry);
                if (QFileInfo::exists(path + "/proton"))
                    result << path;
            }
        }
    }

    QDir localDir(localProtonPath());
    if (localDir.exists()) {
        for (const auto &entry : localDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QString path = localDir.absoluteFilePath(entry);
            if (QFileInfo::exists(path + "/proton"))
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
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/prefixes";
}

QString ProtonScanner::localProtonPath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/protons";
}

QStringList ProtonScanner::findExistingPrefixes() const {
    QStringList result;

    for (const auto &steamRoot : steamPaths()) {
        QDir prefixDir(steamRoot + "/steamapps/compatdata");
        if (!prefixDir.exists()) continue;

        for (const auto &entry : prefixDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QString path = prefixDir.absoluteFilePath(entry);
            if (QFileInfo::exists(path + "/pfx"))
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
