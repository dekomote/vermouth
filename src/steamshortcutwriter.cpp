#include "steamshortcutwriter.h"

#include "flatpakutils.h"
#include "steamlibrary.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>

namespace
{

// Binary VDF tags: 0x00 = nested node, 0x01 = string, 0x02 = int32 LE (bools), 0x08 = node end.
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

}

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

    // The user flagged "mostrecent" in loginusers.vdf; fall back to newest mtime.
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

// Shortcut launcher: point exe at Vermouth (like the desktop shortcuts) so the
// game goes through its prefix/runtime settings. App name + quoted exe are the
// seed of the shortcut app id, so both are derived here in one place.
QString SteamShortcutWriter::shortcutExe() const
{
    QString binary = isInsideFlatpak() ? QStandardPaths::findExecutable(QStringLiteral("flatpak")) : QStandardPaths::findExecutable(QStringLiteral("vermouth"));
    if (binary.isEmpty())
        binary = isInsideFlatpak() ? QStringLiteral("flatpak") : QStringLiteral("vermouth");
    return binary;
}

QString SteamShortcutWriter::shortcutLaunchOptions(const QString &id) const
{
    return isInsideFlatpak() ? QStringLiteral("run com.dekomote.vermouth --launch-id %1").arg(id) : QStringLiteral("--launch-id %1").arg(id);
}

// Shortcut app id, matching the scheme Steam currently uses (from the
// SteamGridDB steam-rom-manager reference everyone follows): standard CRC-32
// of the quoted exe + app name, with bit 31 set.
quint32 SteamShortcutWriter::shortcutAppId(const QString &quotedExe, const QString &appName) const
{
    const QString seed = quotedExe + appName;
    const QByteArray data = seed.toUtf8();
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
    const QString exe = shortcutExe();
    const QString name = app.value(QStringLiteral("name")).toString();
    const QString iconPath = app.value(QStringLiteral("iconPath")).toString();

    // Exe is double-quoted, like Steam and other launchers write it; the appid
    // seed uses that exact value.
    const QString quotedExe = QStringLiteral("\"%1\"").arg(exe);
    const quint32 appid = shortcutAppId(quotedExe, name);

    QByteArray out;
    out.append('\0');
    out.append(QByteArray::number(index));
    out.append('\0');

    writeIntProp(out, "appid", appid);
    writeStringProp(out, "appname", name);
    writeStringProp(out, "exe", quotedExe);
    writeStringProp(out, "StartDir", QStringLiteral("\"%1\"").arg(QFileInfo(exe).absolutePath()));
    writeStringProp(out, "icon", iconPath.startsWith(QStringLiteral("/")) ? iconPath : QString());
    writeStringProp(out, "ShortcutPath", QString());
    writeStringProp(out, "LaunchOptions", shortcutLaunchOptions(id));
    writeIntProp(out, "IsHidden", 0);
    writeIntProp(out, "AllowDesktopConfig", 1);
    writeIntProp(out, "AllowOverlay", 1);
    writeIntProp(out, "OpenVR", 0);
    writeIntProp(out, "LastPlayTime", 0);

    out.append('\0');
    out.append("tags");
    out.append('\0');
    out.append(char(kVdfNodeEnd));
    out.append(char(kVdfNodeEnd));
    return out;
}

bool SteamShortcutWriter::createShortcut(const QVariantMap &app)
{
    const QString name = app.value(QStringLiteral("name")).toString();
    const QString gameExe = app.value(QStringLiteral("exePath")).toString();
    if (name.isEmpty() || gameExe.isEmpty())
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

    // Markers are built byte-explicit - C hex-escapes like "\x01appname"
    // would eat the 'a' as a hex digit (\x1a).
    const QByteArray nameUtf8 = name.toUtf8();
    const QByteArray exeUtf8 = gameExe.toUtf8();
    auto marker = [](const char *key) {
        QByteArray m;
        m.append(char(kVdfString));
        m.append(key);
        m.append('\0');
        return m;
    };
    const QByteArray appnamePat = marker("appname") + nameUtf8 + '\0';
    const QByteArray exePat = marker("exe") + exeUtf8 + '\0';
    if (data.contains(appnamePat) || data.contains(exePat))
        return true;

    if (existed)
        QFile::copy(path, path + QStringLiteral(".bak"));

    // Append the new entry before the trailing node-end bytes.
    QByteArray appidMarker;
    appidMarker.append(char(kVdfInt));
    appidMarker.append("appid");
    appidMarker.append('\0');
    const int nextIndex = data.count(appidMarker);

    const QByteArray head = data.isEmpty() ? QByteArray("\x00shortcuts\x00") : QByteArray();
    qsizetype trim = data.size();
    while (trim > 0 && data.at(trim - 1) == kVdfNodeEnd)
        trim--;

    const QByteArray payload = head + data.left(trim) + encodeEntry(nextIndex, app) + QByteArray(2, char(kVdfNodeEnd));

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    file.write(payload);
    file.close();

    // Touch mtime so a running Steam picks up the change.
    QFile::setPermissions(path, QFile::permissions(path));

    const QString quotedExe = QStringLiteral("\"%1\"").arg(shortcutExe());
    installArtwork(shortcutAppId(quotedExe, name), app);
    return true;
}

void SteamShortcutWriter::installArtwork(quint32 appid, const QVariantMap &app)
{
    const QString user = userDataDir();
    if (user.isEmpty())
        return;
    const QString gridDir = user + QStringLiteral("/config/grid");
    QDir().mkpath(gridDir);

    // Steam looks grid art up by the shortcut id, but older generations used
    // different id bases and suffixes, so both are written. Content is copied
    // as-is; Steam reads the actual bytes regardless of the filename extension.
    const QStringList idBases = {QString::number(appid), QString::number(static_cast<qint32>(appid))};

    auto copyTo = [&](const QString &src, const QStringList &names) {
        if (src.isEmpty() || !QFileInfo::exists(src))
            return;
        for (const QString &stem : names) {
            for (const QString &id : idBases) {
                QFile::remove(gridDir + QLatin1Char('/') + id + stem);
                QFile::copy(src, gridDir + QLatin1Char('/') + id + stem);
            }
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
