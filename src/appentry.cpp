#include "appentry.h"

QJsonObject AppEntry::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("id")] = id;
    obj[QStringLiteral("name")] = name;
    obj[QStringLiteral("exePath")] = exePath;
    obj[QStringLiteral("runtimeType")] = (runtimeType == Proton) ? QStringLiteral("proton") : QStringLiteral("wine");
    obj[QStringLiteral("protonPath")] = protonPath;
    obj[QStringLiteral("protonPrefix")] = protonPrefix;
    obj[QStringLiteral("wineBinary")] = wineBinary;
    obj[QStringLiteral("winePrefix")] = winePrefix;
    obj[QStringLiteral("iconPath")] = iconPath;
    obj[QStringLiteral("launchOptions")] = launchOptions;
    obj[QStringLiteral("enableLogging")] = enableLogging;
    return obj;
}

AppEntry AppEntry::fromJson(const QJsonObject &obj)
{
    AppEntry e;
    e.id = obj[QStringLiteral("id")].toString();
    if (e.id.isEmpty())
        e.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    e.name = obj[QStringLiteral("name")].toString();
    e.exePath = obj[QStringLiteral("exePath")].toString();
    e.runtimeType = (obj[QStringLiteral("runtimeType")].toString() == QStringLiteral("proton")) ? Proton : Wine;
    e.protonPath = obj[QStringLiteral("protonPath")].toString();
    e.protonPrefix = obj[QStringLiteral("protonPrefix")].toString();
    e.wineBinary = obj[QStringLiteral("wineBinary")].toString();
    e.winePrefix = obj[QStringLiteral("winePrefix")].toString();
    e.iconPath = obj[QStringLiteral("iconPath")].toString();
    e.launchOptions = obj[QStringLiteral("launchOptions")].toString();
    e.enableLogging = obj[QStringLiteral("enableLogging")].toBool(false);
    return e;
}
