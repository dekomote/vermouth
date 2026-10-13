#include "dosbox.h"

#include <QDir>
#include <QFileInfo>

static bool isCdImage(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == QStringLiteral("iso") || suffix == QStringLiteral("cue");
}

// Anything up to a 2.88 MB floppy; bigger .img files are hard disk images.
static bool isFloppyImage(const QString &path)
{
    const QFileInfo info(path);
    const QString suffix = info.suffix().toLower();
    return (suffix == QStringLiteral("img") || suffix == QStringLiteral("ima")) && info.size() <= 2949120;
}

static QString imgmountCommand(const QString &drive, const QStringList &images, const QString &type)
{
    QString command = QStringLiteral("imgmount ") + drive;
    for (const QString &image : images)
        command += QStringLiteral(" \"") + image + QLatin1Char('"');
    return command + QStringLiteral(" -t ") + type;
}

// Arguments that make DOSBox Staging start what the entry points at. A config file is loaded as is and a DOS
// program gets its folder mounted as C: by DOSBox itself. A folder on its own would only give an empty DOS
// prompt, so look for something to start in it.
static QStringList dosboxTargetArgs(const QString &exePath)
{
    if (exePath.endsWith(QStringLiteral(".conf"), Qt::CaseInsensitive))
        return {QStringLiteral("--conf"), exePath};
    if (!QFileInfo(exePath).isDir())
        return {exePath};

    const QDir dir(exePath);
    const QStringList confs = dir.entryList({QStringLiteral("dosbox*.conf")}, QDir::Files, QDir::Name | QDir::IgnoreCase);
    if (!confs.isEmpty()) {
        // GOG ships dosbox_<game>.conf plus dosbox_<game>_single.conf, meant to be loaded together.
        // A plain dosbox.conf stands alone.
        QStringList toLoad = confs;
        for (const QString &conf : confs) {
            if (conf.compare(QStringLiteral("dosbox.conf"), Qt::CaseInsensitive) == 0)
                toLoad = {conf};
        }
        QStringList args;
        for (const QString &conf : std::as_const(toLoad))
            args << QStringLiteral("--conf") << dir.filePath(conf);
        // The configs mount paths relative to the game folder.
        args << QStringLiteral("--working-dir") << exePath;
        return args;
    }

    const QStringList programs = dir.entryList({QStringLiteral("*.bat"), QStringLiteral("*.exe"), QStringLiteral("*.com")}, QDir::Files);
    if (programs.size() == 1)
        return {dir.filePath(programs.first())};
    return {exePath};
}

// Disc images are mounted first (several images on one drive can be swapped while running), then the entry itself
// is started: an image as the entry means a CD to browse or a floppy to boot, anything else is handled by
// dosboxTargetArgs.
QStringList dosboxArgs(const QString &exePath, const QStringList &images)
{
    QStringList cds;
    QStringList floppies;
    for (const QString &image : images) {
        if (image == exePath)
            continue;
        if (isCdImage(image))
            cds << image;
        else if (isFloppyImage(image))
            floppies << image;
    }
    const bool entryIsCd = isCdImage(exePath);
    const bool entryIsFloppy = isFloppyImage(exePath);
    if (entryIsCd)
        cds.prepend(exePath);
    if (entryIsFloppy)
        floppies.prepend(exePath);

    QStringList args;
    if (!cds.isEmpty())
        args << QStringLiteral("-c") << imgmountCommand(QStringLiteral("d"), cds, QStringLiteral("iso"));
    if (!floppies.isEmpty())
        args << QStringLiteral("-c") << imgmountCommand(QStringLiteral("a"), floppies, QStringLiteral("floppy"));

    if (entryIsCd)
        args << QStringLiteral("-c") << QStringLiteral("d:");
    else if (entryIsFloppy)
        args << QStringLiteral("-c") << QStringLiteral("boot a:");
    else
        args << dosboxTargetArgs(exePath);
    return args;
}
