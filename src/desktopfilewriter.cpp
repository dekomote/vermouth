#include "desktopfilewriter.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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

    QString exec = QFileInfo::exists(QStringLiteral("/.flatpak-info"))
        ? QStringLiteral("flatpak run com.dekomote.vermouth --launch-id %1").arg(id)
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

    f.close();
    f.setPermissions(f.permissions() | QFile::ExeUser);
    return true;
}

bool DesktopFileWriter::createStartMenuEntry(const QVariantMap &app)
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
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

    const QString menuEntry = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + fileName;
    QFile::remove(menuEntry);

    QString desktopDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (desktopDir.isEmpty())
        desktopDir = QDir::homePath() + QStringLiteral("/Desktop");
    QFile::remove(desktopDir + fileName);
}
