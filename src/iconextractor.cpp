#include "iconextractor.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTextStream>

static QString parseIconFromDesktop(const QString &path);
static QString parseExecFromDesktop(const QString &path);
static QString resolveIconName(const QString &iconName, const QString &searchBase);
static QString findAdjacentIcon(const QString &filePath);

IconExtractor::IconExtractor(QObject *parent)
    : QObject(parent)
{
}

QString IconExtractor::cacheDir() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/assets/icons");
    QDir().mkpath(dir);
    return dir;
}

QString IconExtractor::extractIcon(const QString &exePath)
{
    if (!QFileInfo::exists(exePath))
        return {};

    QByteArray hash = QCryptographicHash::hash(exePath.toUtf8(), QCryptographicHash::Md5).toHex();
    QString outBase = cacheDir() + QStringLiteral("/") + QString::fromLatin1(hash);

    for (const char *ext : {".png", ".svg"}) {
        if (QFileInfo::exists(outBase + QLatin1String(ext)))
            return outBase + QLatin1String(ext);
    }

    QString result = tryWrestool(exePath, outBase + QStringLiteral(".png"));
    if (!result.isEmpty())
        return result;

    if (exePath.endsWith(QStringLiteral(".AppImage"), Qt::CaseInsensitive) || exePath.endsWith(QStringLiteral(".appimage"), Qt::CaseInsensitive))
        return tryAppImage(exePath, outBase);

    if (exePath.endsWith(QStringLiteral(".desktop"), Qt::CaseInsensitive))
        return tryDesktop(exePath);

    // Scripts/binaries: look for an adjacent icon
    QString adjacent = findAdjacentIcon(exePath);
    if (!adjacent.isEmpty())
        return adjacent;

    // Search for a .desktop file in the same directory that references this binary
    QFileInfo fi(exePath);
    const auto desktops = QDir(fi.absolutePath()).entryInfoList(QStringList{QStringLiteral("*.desktop")}, QDir::Files);
    for (const QFileInfo &d : desktops) {
        QString exec = parseExecFromDesktop(d.absoluteFilePath());
        if (!exec.isEmpty()) {
            exec.replace(QRegularExpression(QStringLiteral("\\s*%[fFuUdDnNickvm]")), QString());
            if (exec.contains(fi.fileName()) || exec.contains(exePath)) {
                QString resolved = resolveIconName(parseIconFromDesktop(d.absoluteFilePath()), fi.absolutePath());
                if (!resolved.isEmpty())
                    return resolved;
                // Steal the .desktop's adjacent icon too
                QString adjFromDesktop = findAdjacentIcon(d.absoluteFilePath());
                if (!adjFromDesktop.isEmpty())
                    return adjFromDesktop;
            }
        }
    }

    return {};
}

QString IconExtractor::tryWrestool(const QString &exePath, const QString &outPath)
{
    QString wrestool = QStandardPaths::findExecutable(QStringLiteral("wrestool"));
    QString icotool = QStandardPaths::findExecutable(QStringLiteral("icotool"));
    if (wrestool.isEmpty() || icotool.isEmpty())
        return {};

    QTemporaryFile icoFile;
    icoFile.setFileTemplate(QDir::tempPath() + QStringLiteral("/vermouth_XXXXXX.ico"));
    if (!icoFile.open())
        return {};
    icoFile.close();

    QProcess wrestoolProc;
    wrestoolProc.start(wrestool, {QStringLiteral("--extract"), QStringLiteral("--type=14"), QStringLiteral("--output=") + icoFile.fileName(), exePath});
    if (!wrestoolProc.waitForFinished(10000) || wrestoolProc.exitCode() != 0)
        return {};

    if (QFileInfo(icoFile.fileName()).size() == 0)
        return {};

    QProcess icotoolProc;
    icotoolProc.start(icotool, {QStringLiteral("--list"), icoFile.fileName()});
    if (!icotoolProc.waitForFinished(5000))
        return {};

    QString listOutput = QString::fromUtf8(icotoolProc.readAllStandardOutput());
    int bestSize = 0;
    int bestIndex = 1;
    for (const auto &line : listOutput.split(QLatin1Char('\n'))) {
        QRegularExpression rx(QStringLiteral("--width=(\\d+)"));
        auto match = rx.match(line);
        if (match.hasMatch()) {
            int w = match.captured(1).toInt();
            if (w > bestSize) {
                bestSize = w;
                QRegularExpression idxRx(QStringLiteral("--index=(\\d+)"));
                auto idxMatch = idxRx.match(line);
                if (idxMatch.hasMatch())
                    bestIndex = idxMatch.captured(1).toInt();
            }
        }
    }

    QProcess convertProc;
    convertProc.start(
        icotool,
        {QStringLiteral("--extract"), QStringLiteral("--index=") + QString::number(bestIndex), QStringLiteral("--output=") + outPath, icoFile.fileName()});
    if (!convertProc.waitForFinished(5000) || convertProc.exitCode() != 0) {
        QProcess fallback;
        fallback.start(icotool, {QStringLiteral("--extract"), QStringLiteral("--output=") + outPath, icoFile.fileName()});
        fallback.waitForFinished(5000);
    }

    if (QFileInfo::exists(outPath) && QFileInfo(outPath).size() > 0)
        return outPath;
    return {};
}

static QString parseIconFromDesktop(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    bool inDesktopEntry = false;
    QTextStream in(&f);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line == QStringLiteral("[Desktop Entry]"))
            inDesktopEntry = true;
        else if (line.startsWith(QLatin1Char('[')) && inDesktopEntry)
            break;
        else if (inDesktopEntry && line.startsWith(QStringLiteral("Icon=")))
            return line.mid(5).trimmed();
    }
    return {};
}

static QString parseExecFromDesktop(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    bool inDesktopEntry = false;
    QTextStream in(&f);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line == QStringLiteral("[Desktop Entry]"))
            inDesktopEntry = true;
        else if (line.startsWith(QLatin1Char('[')) && inDesktopEntry)
            break;
        else if (inDesktopEntry && line.startsWith(QStringLiteral("Exec=")))
            return line.mid(5).trimmed();
    }
    return {};
}

static QString resolveIconName(const QString &iconName, const QString &searchBase)
{
    if (iconName.isEmpty())
        return {};

    // Absolute path to an existing file — use directly
    if (iconName.startsWith(QLatin1Char('/')) && QFileInfo::exists(iconName))
        return iconName;

    // Relative to searchBase
    if (!iconName.startsWith(QLatin1Char('/'))) {
        for (const char *ext : {".png", ".svg", ".xpm", ".ico", ""}) {
            QString candidate = searchBase + QLatin1Char('/') + iconName + QLatin1String(ext);
            if (QFileInfo::exists(candidate))
                return candidate;
        }
    }

    // Theme icon name — use directly if the theme has it
    if (QIcon::hasThemeIcon(iconName))
        return iconName;

    // Search common icon directories
    const QStringList searchDirs = {
        QDir::homePath() + QStringLiteral("/.local/share/icons/hicolor/256x256/apps"),
        QDir::homePath() + QStringLiteral("/.local/share/icons/hicolor/128x128/apps"),
        QDir::homePath() + QStringLiteral("/.local/share/icons/hicolor/64x64/apps"),
        QDir::homePath() + QStringLiteral("/.local/share/icons/hicolor/48x48/apps"),
        QStringLiteral("/usr/share/icons/hicolor/256x256/apps"),
        QStringLiteral("/usr/share/icons/hicolor/128x128/apps"),
        QStringLiteral("/usr/share/icons/hicolor/64x64/apps"),
        QStringLiteral("/usr/share/icons/hicolor/48x48/apps"),
        QStringLiteral("/usr/share/pixmaps"),
    };
    for (const QString &dir : searchDirs) {
        for (const char *ext : {".png", ".svg", ".xpm", ""}) {
            QString candidate = dir + QLatin1Char('/') + iconName + QLatin1String(ext);
            if (QFileInfo::exists(candidate))
                return candidate;
        }
    }

    return {};
}

static QString findAdjacentIcon(const QString &filePath)
{
    QFileInfo fi(filePath);
    QString dir = fi.absolutePath();
    QString base = dir + QLatin1Char('/') + fi.completeBaseName();

    // Same basename, common image extensions
    for (const char *ext : {".png", ".svg", ".ico", ".xpm"}) {
        if (QFileInfo::exists(base + QLatin1String(ext)))
            return base + QLatin1String(ext);
    }

    // Common generic icon names in the same directory
    for (const char *name : {"icon", "app", "logo", "application"}) {
        for (const char *ext : {".png", ".svg", ".ico"}) {
            QString candidate = dir + QLatin1Char('/') + QLatin1String(name) + QLatin1String(ext);
            if (QFileInfo::exists(candidate))
                return candidate;
        }
    }

    return {};
}

static QString detectExt(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return QStringLiteral(".png");
    QByteArray hdr = f.read(8);
    if (hdr.startsWith("<?xml") || hdr.startsWith("<svg") || hdr.startsWith("\xef\xbb\xbf"))
        return QStringLiteral(".svg");
    return QStringLiteral(".png");
}

QString IconExtractor::tryAppImage(const QString &exePath, const QString &outBase)
{
    QFileInfo fi(exePath);
    if (!fi.isExecutable())
        QFile::setPermissions(exePath, fi.permissions() | QFileDevice::ExeOwner);

    // Fast path: extract the root .desktop file and check if its icon is in the theme
    {
        QTemporaryDir quickTmp;
        if (quickTmp.isValid()) {
            QProcess proc;
            proc.setWorkingDirectory(quickTmp.path());
            proc.start(exePath, {QStringLiteral("--appimage-extract"), QStringLiteral("*.desktop")});
            proc.waitForFinished(10000);
            const auto entries = QDir(quickTmp.path() + QStringLiteral("/squashfs-root")).entryList(QStringList{QStringLiteral("*.desktop")}, QDir::Files);
            for (const QString &df : entries) {
                QString iconName = parseIconFromDesktop(quickTmp.path() + QStringLiteral("/squashfs-root/") + df);
                if (!iconName.isEmpty() && QIcon::hasThemeIcon(iconName))
                    return iconName;
            }
        }
    }

    QTemporaryDir tmpDir;
    if (!tmpDir.isValid())
        return {};

    auto copyToCache = [&](const QString &srcPath) -> QString {
        QString outPath = outBase + detectExt(srcPath);
        return QFile::copy(srcPath, outPath) ? outPath : QString();
    };

    // Extract .DirIcon from the AppImage (uses built-in squashfs, no FUSE needed)
    {
        QProcess proc;
        proc.setWorkingDirectory(tmpDir.path());
        proc.start(exePath, {QStringLiteral("--appimage-extract"), QStringLiteral(".DirIcon")});
        proc.waitForFinished(15000);

        QString dirIcon = tmpDir.path() + QStringLiteral("/squashfs-root/.DirIcon");
        QFileInfo difi(dirIcon);

        if (difi.exists() && !difi.isSymLink())
            return copyToCache(dirIcon);

        if (difi.isSymLink()) {
            QString absTarget = difi.symLinkTarget();
            QString squashRoot = tmpDir.path() + QStringLiteral("/squashfs-root/");
            if (absTarget.startsWith(squashRoot)) {
                QString relTarget = absTarget.mid(squashRoot.length());
                QProcess proc2;
                proc2.setWorkingDirectory(tmpDir.path());
                proc2.start(exePath, {QStringLiteral("--appimage-extract"), relTarget});
                proc2.waitForFinished(15000);
                if (QFileInfo::exists(absTarget))
                    return copyToCache(absTarget);
            }
        }
    }

    // Fallback: unsquashfs with symlink following
    QString unsquashfs = QStandardPaths::findExecutable(QStringLiteral("unsquashfs"));
    if (!unsquashfs.isEmpty()) {
        QTemporaryDir tmpDir2;
        if (!tmpDir2.isValid())
            return {};
        QString extractDir = tmpDir2.path() + QStringLiteral("/root");
        QProcess proc;
        proc.start(unsquashfs,
                   {QStringLiteral("-n"), QStringLiteral("-follow-symlinks"), QStringLiteral("-d"), extractDir, exePath, QStringLiteral(".DirIcon")});
        proc.waitForFinished(15000);
        QString iconPath = extractDir + QStringLiteral("/.DirIcon");
        if (QFileInfo::exists(iconPath) && !QFileInfo(iconPath).isSymLink())
            return copyToCache(iconPath);
    }

    return {};
}

QString IconExtractor::tryDesktop(const QString &exePath)
{
    QString iconName = parseIconFromDesktop(exePath);
    QString desktopDir = QFileInfo(exePath).absolutePath();

    // Resolve the Icon= key
    QString resolved = resolveIconName(iconName, desktopDir);
    if (!resolved.isEmpty())
        return resolved;

    // Fallback: try to extract icon from the Exec= line's binary
    QString exec = parseExecFromDesktop(exePath);
    if (!exec.isEmpty()) {
        exec.replace(QRegularExpression(QStringLiteral("\\s*%[fFuUdDnNickvm]")), QString());
        exec.replace(QStringLiteral("%%"), QStringLiteral("%"));
        // Try the first space-delimited token as a path
        QString binary = exec.section(QLatin1Char(' '), 0, 0);
        if (!binary.isEmpty() && !binary.startsWith(QLatin1Char('-'))) {
            // Check if the executable has an adjacent icon
            QString adjIcon = findAdjacentIcon(binary);
            if (!adjIcon.isEmpty())
                return adjIcon;
            // Try wrestool on the executable (for Windows .exe)
            QString outBase =
                cacheDir() + QStringLiteral("/") + QString::fromLatin1(QCryptographicHash::hash(binary.toUtf8(), QCryptographicHash::Md5).toHex());
            QString extracted = tryWrestool(binary, outBase + QStringLiteral(".png"));
            if (!extracted.isEmpty())
                return extracted;
            // If the executable is an AppImage, try to extract its icon
            if (binary.endsWith(QStringLiteral(".AppImage"), Qt::CaseInsensitive) || binary.endsWith(QStringLiteral(".appimage"), Qt::CaseInsensitive)) {
                QString appImageIcon = tryAppImage(binary, outBase);
                if (!appImageIcon.isEmpty())
                    return appImageIcon;
            }
        }
    }

    // Last resort: adjacent images matching the .desktop filename
    return findAdjacentIcon(exePath);
}
