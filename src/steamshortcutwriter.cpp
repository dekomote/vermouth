#include "steamshortcutwriter.h"

#include "flatpakutils.h"
#include "steamlibrary.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>

namespace
{

// Binary VDF tags: 0x00 nested node, 0x01 string, 0x02 int32 LE, 0x08 node end.
constexpr quint8 kVdfString = 0x01;
constexpr quint8 kVdfInt = 0x02;
constexpr quint8 kVdfNodeEnd = 0x08;

void writeStringProp(QByteArray &out, const char *key, const QString &value)
{
    out.append(char(kVdfString));
    out.append(key, int(qstrlen(key)));
    out.append('\0');
    out.append(value.toUtf8());
    out.append('\0');
}

void writeIntProp(QByteArray &out, const char *key, quint32 value)
{
    out.append(char(kVdfInt));
    out.append(key, int(qstrlen(key)));
    out.append('\0');
    out.append(char(value & 0xFF));
    out.append(char((value >> 8) & 0xFF));
    out.append(char((value >> 16) & 0xFF));
    out.append(char((value >> 24) & 0xFF));
}

} // namespace

SteamShortcutWriter::SteamShortcutWriter(QObject *parent)
    : QObject(parent)
{
}

QString SteamShortcutWriter::steamRoot() const
{
    const QStringList roots = SteamLibrary::steamRootPaths();
    return roots.isEmpty() ? QString() : roots.first();
}

QString SteamShortcutWriter::userDataDir() const
{
    const QString root = steamRoot();
    if (root.isEmpty())
        return QString();

    QString chosen;
    qint64 newestMtime = -1;
    QDir userdata(root + QStringLiteral("/userdata"));

    QFile loginUsers(root + QStringLiteral("/config/loginusers.vdf"));
    if (loginUsers.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&loginUsers);
        QRegularExpression userRx(QStringLiteral("\"(\\d+)\"\\s*\\{"));
        QRegularExpression recentRx(QStringLiteral("\"mostrecent\"\\s+\"(\\d+)\""));
        QString currentUser;
        while (!in.atEnd()) {
            QString line = in.readLine();
            auto um = userRx.match(line);
            if (um.hasMatch()) {
                currentUser = um.captured(1);
                continue;
            }
            auto rm = recentRx.match(line);
            if (rm.hasMatch() && rm.captured(1) == QLatin1String("1") && !currentUser.isEmpty()) {
                if (QDir(userdata.filePath(currentUser)).exists())
                    return userdata.filePath(currentUser);
            }
        }
    }

    // Modern Steam omits "mostrecent" - fall back to the newest userdata folder.
    const auto entries = userdata.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const auto &name : entries) {
        if (name == QLatin1String("config"))
            continue;
        QFileInfo fi(userdata.filePath(name));
        if (fi.lastModified().toMSecsSinceEpoch() > newestMtime) {
            newestMtime = fi.lastModified().toMSecsSinceEpoch();
            chosen = userdata.filePath(name);
        }
    }
    return chosen;
}

QString SteamShortcutWriter::shortcutsPath() const
{
    const QString user = userDataDir();
    return user.isEmpty() ? QString() : user + QStringLiteral("/config/shortcuts.vdf");
}

QString SteamShortcutWriter::shortcutScriptDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/steam-shortcuts");
}

QString SteamShortcutWriter::shortcutScriptFor(const QString &id) const
{
    QString safe = id;
    safe.replace(QRegularExpression(QStringLiteral("[^a-zA-Z0-9_-]")), QStringLiteral("_"));
    return shortcutScriptDir() + QLatin1Char('/') + safe + QStringLiteral(".sh");
}

bool SteamShortcutWriter::writeShortcutScript(const QString &id) const
{
    const QString path = shortcutScriptFor(id);
    QDir().mkpath(QFileInfo(path).absolutePath());

    // Absolute path - "vermouth" on PATH may be unrelated or absent.
    const QString launch = isInsideFlatpak() ? QStringLiteral("flatpak run com.dekomote.vermouth --launch-id %1").arg(id)
                                             : QStringLiteral("%1 --launch-id %2").arg(QCoreApplication::applicationFilePath(), id);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;
    QTextStream out(&file);
    out << QStringLiteral("#!/bin/sh\n# Vermouth Steam shortcut for %1\nexec %2\n").arg(id, launch);
    file.close();
    return file.setPermissions(file.permissions() | QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther);
}

quint32 SteamShortcutWriter::shortcutAppId(const QString &quotedExe, const QString &appName) const
{
    const QByteArray data = (quotedExe + appName).toUtf8();
    quint32 crc = 0xFFFFFFFFu;
    for (const char byte : data) {
        crc ^= static_cast<quint8>(byte);
        for (int i = 0; i < 8; ++i)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    crc ^= 0xFFFFFFFFu;
    return crc | 0x80000000u;
}

QByteArray SteamShortcutWriter::encodeEntry(int index, const QVariantMap &app) const
{
    const QString id = app.value(QStringLiteral("id")).toString();
    const QString name = app.value(QStringLiteral("name")).toString();
    const QString iconPath = app.value(QStringLiteral("iconPath")).toString();

    // Per-game wrapper script as the exe; its unique path is also the appid seed.
    const QString script = shortcutScriptFor(id);
    const QString quotedScript = QStringLiteral("\"%1\"").arg(script);
    const quint32 appid = shortcutAppId(quotedScript, name);

    QByteArray out;
    out.append('\0');
    out.append(QByteArray::number(index));
    out.append('\0');

    writeIntProp(out, "AllowDesktopConfig", 0);
    writeIntProp(out, "AllowOverlay", 0);
    writeIntProp(out, "appid", appid);
    writeStringProp(out, "AppName", name);
    writeIntProp(out, "DevKit", 0);
    writeStringProp(out, "DevkitGameID", QString());
    writeIntProp(out, "DevkitOverrideAppID", 0);
    writeStringProp(out, "Exe", quotedScript);
    writeStringProp(out, "FlatpakAppID", QString());
    writeIntProp(out, "IsHidden", 0);
    writeIntProp(out, "LastPlayTime", 0);
    writeStringProp(out, "LaunchOptions", QString());
    writeIntProp(out, "OpenVR", 0);
    writeStringProp(out, "ShortcutPath", QString());
    writeStringProp(out, "StartDir", shortcutScriptDir());
    if (iconPath.startsWith(QStringLiteral("/")))
        writeStringProp(out, "icon", iconPath);

    out.append('\0');
    out.append("tags");
    out.append('\0');
    out.append(char(kVdfNodeEnd));
    out.append(char(kVdfNodeEnd));
    return out;
}

QList<SteamShortcutWriter::ParsedEntry> SteamShortcutWriter::parseEntries(const QByteArray &data) const
{
    QList<ParsedEntry> out;
    if (data.size() < 11 || data.at(0) != '\0' || data.mid(1, 9) != "shortcuts" || data.at(10) != '\0')
        return out;

    qsizetype pos = 11;
    auto readCstr = [&data](qsizetype &p) {
        qsizetype end = data.indexOf('\0', p);
        if (end < 0)
            end = data.size();
        QByteArray s = data.mid(p, end - p);
        p = end + 1; // skip the trailing NUL
        return s;
    };

    while (pos < data.size()) {
        while (pos < data.size() && data.at(pos) == kVdfNodeEnd)
            pos++;
        if (pos >= data.size() || data.at(pos) != '\0')
            break;

        ParsedEntry e;
        pos++; // head '\x00'
        readCstr(pos); // index
        if (pos >= data.size())
            break;
        const qsizetype contentStart = pos; // first property type byte

        while (pos < data.size()) {
            const char t = data.at(pos);
            if (t == kVdfNodeEnd)
                break;
            pos++;
            QByteArray key = readCstr(pos);
            if (t == kVdfString) {
                QByteArray value = readCstr(pos);
                if (key.compare("AppName", Qt::CaseInsensitive) == 0)
                    e.name = QString::fromUtf8(value);
                else if (key.compare("Exe", Qt::CaseInsensitive) == 0) {
                    e.exe = QString::fromUtf8(value);
                    if (e.exe.size() >= 2 && e.exe.startsWith(QLatin1Char('"')) && e.exe.endsWith(QLatin1Char('"')))
                        e.exe = e.exe.mid(1, e.exe.size() - 2);
                }
            } else if (t == kVdfInt) {
                pos += 4;
            } else if (t == '\0') {
                // Nested tags node: \x01<idx>\x00<tag>\x00 items, then one closer.
                while (pos + 1 < data.size() && data.at(pos) == kVdfString) {
                    pos++;
                    readCstr(pos);
                    readCstr(pos);
                }
                if (pos < data.size() && data.at(pos) == kVdfNodeEnd)
                    pos++;
            } else {
                pos += 4;
            }
        }

        // Terminator is the 0x08 run after the last property/tags node.
        while (pos < data.size() && data.at(pos) == kVdfNodeEnd)
            pos++;
        e.content = data.mid(contentStart, pos - contentStart);
        out << e;
    }
    return out;
}

bool SteamShortcutWriter::createShortcut(const QVariantMap &app)
{
    const QString name = app.value(QStringLiteral("name")).toString();
    const QString gameExe = app.value(QStringLiteral("exePath")).toString();
    const QString id = app.value(QStringLiteral("id")).toString();
    if (name.isEmpty() || id.isEmpty())
        return false;

    const QString path = shortcutsPath();
    if (path.isEmpty())
        return false;

    QDir().mkpath(QFileInfo(path).absolutePath());

    QByteArray data;
    QFile file(path);
    const bool existed = file.exists();
    if (existed && file.open(QIODevice::ReadOnly))
        data = file.readAll();
    file.close();

    QList<ParsedEntry> entries = parseEntries(data);

    // Upsert: replace this game's existing entry (by name, our script, or a legacy raw exe).
    const QString scriptPath = shortcutScriptFor(id);
    entries.erase(std::remove_if(entries.begin(),
                                 entries.end(),
                                 [&](const ParsedEntry &e) {
                                     return e.name == name || (!e.exe.isEmpty() && (e.exe == scriptPath || e.exe == gameExe));
                                 }),
                  entries.end());

    if (!writeShortcutScript(id))
        return false;

    if (existed)
        QFile::copy(path, path + QStringLiteral(".bak"));

    // Reindex keys 0..N-1 - Steam stops importing at the first index gap.
    QByteArray payload;
    payload.append('\0');
    payload.append("shortcuts");
    payload.append('\0');
    int nextIndex = 0;
    for (const auto &e : entries) {
        payload.append('\0');
        payload.append(QByteArray::number(nextIndex));
        payload.append('\0');
        QByteArray block = e.content;
        if (block.startsWith(QByteArray("\x02index\x00")))
            block = block.mid(11); // drop the obsolete index property
        // End each entry with exactly two 0x08 bytes; Steam balks at stray ones.
        while (block.size() && block.at(block.size() - 1) == kVdfNodeEnd)
            block.chop(1);
        block.append(char(kVdfNodeEnd));
        block.append(char(kVdfNodeEnd));
        payload.append(block);
        ++nextIndex;
    }
    payload.append(encodeEntry(nextIndex, app));
    payload.append(char(kVdfNodeEnd));
    payload.append(char(kVdfNodeEnd));

    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly))
        return false;
    out.write(payload);
    if (!out.commit())
        return false;

    QFile::setPermissions(path, QFile::permissions(path)); // bump mtime for Steam

    // Read back to confirm the write persisted.
    QFile check(path);
    if (check.open(QIODevice::ReadOnly))
        qWarning() << "[steam-shortcut] entries on disk:" << check.readAll().count("appid");

    const QString quotedScript = QStringLiteral("\"%1\"").arg(scriptPath);
    installArtwork(shortcutAppId(quotedScript, name), app);
    return true;
}

void SteamShortcutWriter::installArtwork(quint32 appid, const QVariantMap &app)
{
    const QString user = userDataDir();
    if (user.isEmpty())
        return;
    const QString gridDir = user + QStringLiteral("/config/grid");
    QDir().mkpath(gridDir);

    // Cover both the legacy signed and current unsigned shortcut id bases.
    const QStringList idBases = {QString::number(appid), QString::number(static_cast<qint32>(appid))};

    auto copyTo = [&](const QString &src, const QStringList &names) {
        if (src.isEmpty() || !QFileInfo::exists(src))
            return;
        for (const QString &stem : names)
            for (const QString &id : idBases) {
                QFile::remove(gridDir + QLatin1Char('/') + id + stem);
                QFile::copy(src, gridDir + QLatin1Char('/') + id + stem);
            }
    };

    const QString grid = app.value(QStringLiteral("gridPath")).toString();
    const QString hero = app.value(QStringLiteral("heroPath")).toString();
    const QString logo = app.value(QStringLiteral("logoPath")).toString();
    const QString icon = app.value(QStringLiteral("iconPath")).toString();
    const QString anyLandscape = hero.isEmpty() ? grid : hero;

    copyTo(grid, {QStringLiteral("p.png"), QStringLiteral("p.jpg"), QStringLiteral("_library_600x900.png"), QStringLiteral("_library_600x900.jpg")});
    copyTo(anyLandscape,
           {QStringLiteral(".jpg"),
            QStringLiteral("_hero.jpg"),
            QStringLiteral("_hero.png"),
            QStringLiteral("_library_hero.jpg"),
            QStringLiteral("_library_hero.png")});
    copyTo(logo, {QStringLiteral("l.png"), QStringLiteral("_logo.jpg"), QStringLiteral("_library_logo.png"), QStringLiteral("_library_logo.jpg")});
    copyTo(icon, {QStringLiteral("_icon.png")});
}

bool SteamShortcutWriter::isAvailable() const
{
    return !shortcutsPath().isEmpty();
}