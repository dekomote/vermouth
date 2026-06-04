#include "goglibrary.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

static GogEntry tryParseWindows(const QDir &gameDir)
{
    for (const QString &infoFile : gameDir.entryList({QStringLiteral("goggame-*.info")}, QDir::Files)) {
        QFile f(gameDir.absoluteFilePath(infoFile));
        if (!f.open(QIODevice::ReadOnly))
            continue;

        QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
        QString gameId = obj[QStringLiteral("gameId")].toString();
        if (gameId != obj[QStringLiteral("rootGameId")].toString())
            continue;

        QString name = obj[QStringLiteral("name")].toString();
        if (name.isEmpty())
            continue;

        QString exeRelPath;
        bool foundPrimary = false;
        for (const QJsonValue &val : obj[QStringLiteral("playTasks")].toArray()) {
            QJsonObject t = val.toObject();
            if (t[QStringLiteral("type")].toString() != QStringLiteral("FileTask"))
                continue;
            if (t[QStringLiteral("category")].toString() != QStringLiteral("game"))
                continue;
            QString path = t[QStringLiteral("path")].toString().replace(QLatin1Char('\\'), QLatin1Char('/'));
            if (path.isEmpty())
                continue;
            bool primary = t[QStringLiteral("isPrimary")].toBool();
            if (exeRelPath.isEmpty() || (!foundPrimary && primary)) {
                exeRelPath = path;
                foundPrimary = primary;
            }
        }

        if (exeRelPath.isEmpty())
            continue;

        QString exePath = gameDir.absoluteFilePath(exeRelPath);
        if (!QFileInfo::exists(exePath))
            continue;

        QString iconPath;
        QStringList icoFiles = gameDir.entryList({QStringLiteral("goggame-*.ico")}, QDir::Files);
        if (!icoFiles.isEmpty())
            iconPath = gameDir.absoluteFilePath(icoFiles.constFirst());

        GogEntry entry;
        entry.gameId = gameId;
        entry.name = name;
        entry.exePath = exePath;
        entry.iconPath = iconPath;
        entry.isWindows = true;
        return entry;
    }
    return {};
}

static GogEntry tryParseLinux(const QDir &gameDir)
{
    QString startSh = gameDir.absoluteFilePath(QStringLiteral("start.sh"));
    if (!QFileInfo::exists(startSh))
        return {};

    QFile gameInfo(gameDir.absoluteFilePath(QStringLiteral("gameinfo")));
    if (!gameInfo.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QString name = QTextStream(&gameInfo).readLine().trimmed();
    if (name.isEmpty())
        return {};

    QString iconPath = gameDir.absoluteFilePath(QStringLiteral("support/icon.png"));
    if (!QFileInfo::exists(iconPath))
        iconPath.clear();

    GogEntry entry;
    entry.name = name;
    entry.exePath = startSh;
    entry.iconPath = iconPath;
    entry.isWindows = false;
    return entry;
}

QVector<GogEntry> GogLibrary::scan(const QString &rootPath)
{
    QVector<GogEntry> entries;
    QDir root(rootPath);
    if (!root.exists())
        return entries;

    // Check if root itself is a game
    GogEntry rootEntry = tryParseWindows(root);
    if (rootEntry.name.isEmpty())
        rootEntry = tryParseLinux(root);
    if (!rootEntry.name.isEmpty())
        entries.append(rootEntry);

    for (const QString &dirName : root.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QDir gameDir(root.absoluteFilePath(dirName));

        GogEntry entry = tryParseWindows(gameDir);
        if (entry.name.isEmpty())
            entry = tryParseLinux(gameDir);
        if (!entry.name.isEmpty())
            entries.append(entry);
    }

    return entries;
}
