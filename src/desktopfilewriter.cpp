#include "desktopfilewriter.h"
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <QCoreApplication>
#include <QStandardPaths>

DesktopFileWriter::DesktopFileWriter(QObject *parent)
    : QObject(parent)
{}

QString DesktopFileWriter::safeName(const QString &name) const {
    QString safe = name;
    safe.replace(QRegularExpression(QStringLiteral("[^a-zA-Z0-9_-]")), QStringLiteral("_"));
    return safe;
}

bool DesktopFileWriter::writeDesktopFile(const QString &filePath, const QVariantMap &app) {
    QString name = app[QStringLiteral("name")].toString();
    QString vermouthBin = QCoreApplication::applicationFilePath();
    QString runtimeType = app[QStringLiteral("runtimeType")].toString();

    QString exec;
    if (runtimeType == QStringLiteral("proton")) {
        exec = QStringLiteral("%1 --launch-proton \"%2\" --proton \"%3\" --prefix \"%4\"")
            .arg(vermouthBin, app[QStringLiteral("exePath")].toString(),
                 app[QStringLiteral("protonPath")].toString(), app[QStringLiteral("protonPrefix")].toString());
    } else {
        exec = QStringLiteral("%1 --launch-wine \"%2\" --wine \"%3\" --prefix \"%4\"")
            .arg(vermouthBin, app[QStringLiteral("exePath")].toString(),
                 app[QStringLiteral("wineBinary")].toString(), app[QStringLiteral("winePrefix")].toString());
    }

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
    if (!app[QStringLiteral("iconPath")].toString().isEmpty())
        out << QStringLiteral("Icon=") << app[QStringLiteral("iconPath")].toString() << QStringLiteral("\n");
    out << QStringLiteral("Comment=Launched via Vermouth\n");

    f.close();
    f.setPermissions(f.permissions() | QFile::ExeUser);
    return true;
}

bool DesktopFileWriter::createStartMenuEntry(const QVariantMap &app) {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir().mkpath(dir);
    QString filePath = dir + QStringLiteral("/vermouth-") + safeName(app[QStringLiteral("name")].toString()) + QStringLiteral(".desktop");
    return writeDesktopFile(filePath, app);
}

bool DesktopFileWriter::createDesktopShortcut(const QVariantMap &app) {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (dir.isEmpty())
        dir = QDir::homePath() + QStringLiteral("/Desktop");
    QDir().mkpath(dir);
    QString filePath = dir + QStringLiteral("/vermouth-") + safeName(app[QStringLiteral("name")].toString()) + QStringLiteral(".desktop");
    return writeDesktopFile(filePath, app);
}

bool DesktopFileWriter::removeStartMenuEntry(const QString &name) {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    return QFile::remove(dir + QStringLiteral("/vermouth-") + safeName(name) + QStringLiteral(".desktop"));
}
