#include "steamlibrary.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>

QStringList SteamLibrary::steamRootPaths()
{
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

static QStringList artworkCacheDirs(const QStringList &roots)
{
    QStringList dirs;
    QString home = QDir::homePath();
    dirs << home + QStringLiteral("/.local/share/Steam/appcache/librarycache");
    dirs << home + QStringLiteral("/.var/app/com.valvesoftware.Steam/.local/share/Steam/appcache/librarycache");
    for (const auto &root : roots) {
        QString cache = root + QStringLiteral("/appcache/librarycache");
        if (!dirs.contains(cache))
            dirs << cache;
    }
    return dirs;
}

static void resolveArtwork(SteamEntry &entry, const QStringList &cacheDirs)
{
    auto check = [](const QString &path) -> QString {
        return QFileInfo::exists(path) ? path : QString();
    };

    QString id = QString::number(entry.appId);
    for (const auto &dir : cacheDirs) {
        QString sub = dir + QLatin1Char('/') + id + QLatin1Char('/');

        if (entry.gridPath.isEmpty()) {
            entry.gridPath = check(sub + QStringLiteral("library_600x900.jpg"));
            if (entry.gridPath.isEmpty())
                entry.gridPath = check(dir + QLatin1Char('/') + id + QStringLiteral("_library_600x900.jpg"));
        }
        if (entry.heroPath.isEmpty()) {
            entry.heroPath = check(sub + QStringLiteral("library_hero.jpg"));
            if (entry.heroPath.isEmpty())
                entry.heroPath = check(dir + QLatin1Char('/') + id + QStringLiteral("_library_hero.jpg"));
            if (entry.heroPath.isEmpty())
                entry.heroPath = check(dir + QLatin1Char('/') + id + QStringLiteral("_hero.jpg"));
        }
        if (entry.logoPath.isEmpty()) {
            entry.logoPath = check(sub + QStringLiteral("logo.png"));
            if (entry.logoPath.isEmpty())
                entry.logoPath = check(dir + QLatin1Char('/') + id + QStringLiteral("_logo.png"));
        }

        if (!entry.gridPath.isEmpty() && !entry.heroPath.isEmpty() && !entry.logoPath.isEmpty())
            break;
    }

    if (entry.iconPath.isEmpty())
        entry.iconPath = entry.gridPath;
}

static bool isSteamTool(const QString &name, int appId)
{
    static const QSet<int> toolIds = {
        7,
        8,
        228980,
        1070560,
        1391110,
        1628350,
        1826330,
        1493710,
        218010,
        740,
        205790,
        222840,
        232250,
    };

    if (toolIds.contains(appId))
        return true;

    if (name.startsWith(QStringLiteral("Proton ")) || name.startsWith(QStringLiteral("Steamworks ")) || name.startsWith(QStringLiteral("Steam Linux Runtime"))
        || name.endsWith(QStringLiteral(" Dedicated Server")))
        return true;

    return false;
}

void SteamLibrary::parseManifest(const QString &path, QVector<SteamEntry> &out)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    SteamEntry entry;
    QTextStream in(&f);
    QRegularExpression rx(QStringLiteral("\"([^\"]+)\"\\s+\"([^\"]*)\""));
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        auto match = rx.match(line);
        if (!match.hasMatch())
            continue;
        QString key = match.captured(1);
        QString val = match.captured(2);
        if (key == QStringLiteral("appid"))
            entry.appId = val.toInt();
        else if (key == QStringLiteral("name"))
            entry.name = val;
        else if (key == QStringLiteral("installdir"))
            entry.installDir = val;
    }

    if (entry.appId <= 0 || entry.name.isEmpty())
        return;

    if (isSteamTool(entry.name, entry.appId))
        return;

    out.append(entry);
}

QVector<SteamEntry> SteamLibrary::scan()
{
    QVector<SteamEntry> entries;
    QStringList roots = steamRootPaths();
    QStringList cacheDirs = artworkCacheDirs(roots);

    for (const auto &root : roots) {
        QDir steamapps(root + QStringLiteral("/steamapps"));
        if (!steamapps.exists())
            continue;

        auto manifests = steamapps.entryList({QStringLiteral("appmanifest_*.acf")}, QDir::Files);
        for (const auto &mf : manifests) {
            QString path = steamapps.absoluteFilePath(mf);
            int before = entries.size();
            parseManifest(path, entries);
            if (entries.size() > before) {
                auto &entry = entries.last();
                entry.installDir = steamapps.absolutePath() + QStringLiteral("/common/") + entry.installDir;
                resolveArtwork(entry, cacheDirs);
            }
        }
    }

    return entries;
}
