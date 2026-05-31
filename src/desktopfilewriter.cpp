#include "desktopfilewriter.h"
#include "flatpakutils.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>

DesktopFileWriter::DesktopFileWriter(QObject *parent)
    : QObject(parent)
{
}

QString DesktopFileWriter::safeName(const QString &name) const
{
    QString safe = name;
    safe.replace(QRegularExpression(QStringLiteral("[^a-zA-Z0-9_-]")), QStringLiteral("_"));
    return safe;
}

bool DesktopFileWriter::writeDesktopFile(const QString &filePath, const QVariantMap &app)
{
    QString name = app[QStringLiteral("name")].toString();
    QString id = app[QStringLiteral("id")].toString();

    QString exec = isInsideFlatpak() ? QStringLiteral("flatpak run com.dekomote.vermouth --launch-id %1").arg(id)
                                     : QStringLiteral("'%1' --launch-id \"%2\"").arg(QCoreApplication::applicationFilePath(), id);

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&f);
    out << QStringLiteral("[Desktop Entry]\n");
    out << QStringLiteral("Type=Application\n");
    out << QStringLiteral("Name=") << name << QStringLiteral("\n");
    out << QStringLiteral("Exec=") << exec << QStringLiteral("\n");
    out << QStringLiteral("Terminal=false\n");
    out << QStringLiteral("Categories=Game;\n");
    QString icon = app[QStringLiteral("iconPath")].toString();
    if (icon.isEmpty())
        icon = QStringLiteral("com.dekomote.vermouth");
    out << QStringLiteral("Icon=") << icon << QStringLiteral("\n");
    out << QStringLiteral("Comment=Launched via Vermouth\n");
    out << QStringLiteral("X-Vermouth-AppId=") << id << QStringLiteral("\n");

    f.close();
    f.setPermissions(f.permissions() | QFile::ExeUser);
    return true;
}

bool DesktopFileWriter::createStartMenuEntry(const QVariantMap &app)
{
    QString dir = isInsideFlatpak() ? QDir::homePath() + QStringLiteral("/.local/share/applications")
                                    : QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir().mkpath(dir);
    QString filePath = dir + QStringLiteral("/vermouth-") + safeName(app[QStringLiteral("name")].toString()) + QStringLiteral(".desktop");
    return writeDesktopFile(filePath, app);
}

bool DesktopFileWriter::createDesktopShortcut(const QVariantMap &app)
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (dir.isEmpty())
        dir = QDir::homePath() + QStringLiteral("/Desktop");
    QDir().mkpath(dir);
    QString filePath = dir + QStringLiteral("/vermouth-") + safeName(app[QStringLiteral("name")].toString()) + QStringLiteral(".desktop");
    return writeDesktopFile(filePath, app);
}

void DesktopFileWriter::removeShortcuts(const QVariantMap &app)
{
    const QString fileName = QStringLiteral("/vermouth-") + safeName(app[QStringLiteral("name")].toString()) + QStringLiteral(".desktop");
    const QString id = app[QStringLiteral("id")].toString();
    const QString idMarker = QStringLiteral("X-Vermouth-AppId=") + id;

    const QString appsDir = isInsideFlatpak() ? QDir::homePath() + QStringLiteral("/.local/share/applications")
                                              : QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QString desktopDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (desktopDir.isEmpty())
        desktopDir = QDir::homePath() + QStringLiteral("/Desktop");

    const QStringList dirs = {appsDir, desktopDir};
    for (const QString &dir : dirs) {
        QFile::remove(dir + fileName);

        if (id.isEmpty())
            continue;
        QDir d(dir);
        if (!d.exists())
            continue;
        const QStringList entries = d.entryList({QStringLiteral("vermouth-*.desktop")}, QDir::Files);
        for (const QString &entry : entries) {
            const QString fullPath = d.absoluteFilePath(entry);
            QFile f(fullPath);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;
            bool match = false;
            QTextStream in(&f);
            while (!in.atEnd()) {
                if (in.readLine().trimmed() == idMarker) {
                    match = true;
                    break;
                }
            }
            f.close();
            if (match)
                QFile::remove(fullPath);
        }
    }
}
