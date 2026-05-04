#include "appentry.h"

const char *AppEntry::runtimeTypeString(RuntimeType rt)
{
    switch (rt) {
    case Proton:
        return "proton";
    case Wine:
        return "wine";
    case Native:
        return "native";
    case Steam:
        return "steam";
    }
    return "native";
}

AppEntry::RuntimeType AppEntry::runtimeTypeFromString(const QString &s)
{
    if (s == QStringLiteral("proton"))
        return Proton;
    if (s == QStringLiteral("wine"))
        return Wine;
    if (s == QStringLiteral("steam"))
        return Steam;
    return Native;
}

QJsonObject AppEntry::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("id")] = id;
    obj[QStringLiteral("name")] = name;
    obj[QStringLiteral("exePath")] = exePath;
    obj[QStringLiteral("runtimeType")] = QString::fromLatin1(runtimeTypeString(runtimeType));
    obj[QStringLiteral("protonPath")] = protonPath;
    obj[QStringLiteral("protonPrefix")] = protonPrefix;
    obj[QStringLiteral("wineBinary")] = wineBinary;
    obj[QStringLiteral("winePrefix")] = winePrefix;
    obj[QStringLiteral("iconPath")] = iconPath;
    obj[QStringLiteral("gridPath")] = gridPath;
    obj[QStringLiteral("heroPath")] = heroPath;
    obj[QStringLiteral("logoPath")] = logoPath;
    obj[QStringLiteral("steamGridDbId")] = steamGridDbId;
    obj[QStringLiteral("steamAppId")] = steamAppId;
    obj[QStringLiteral("launchOptions")] = launchOptions;
    obj[QStringLiteral("enableLogging")] = enableLogging;
    return obj;
}

QVariantMap AppEntry::toVariantMap() const
{
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("name"), name},
        {QStringLiteral("exePath"), exePath},
        {QStringLiteral("runtimeType"), QString::fromLatin1(runtimeTypeString(runtimeType))},
        {QStringLiteral("protonPath"), protonPath},
        {QStringLiteral("protonPrefix"), protonPrefix},
        {QStringLiteral("wineBinary"), wineBinary},
        {QStringLiteral("winePrefix"), winePrefix},
        {QStringLiteral("iconPath"), iconPath},
        {QStringLiteral("gridPath"), gridPath},
        {QStringLiteral("heroPath"), heroPath},
        {QStringLiteral("logoPath"), logoPath},
        {QStringLiteral("steamGridDbId"), steamGridDbId},
        {QStringLiteral("steamAppId"), steamAppId},
        {QStringLiteral("launchOptions"), launchOptions},
        {QStringLiteral("enableLogging"), enableLogging},
    };
}

AppEntry AppEntry::fromJson(const QJsonObject &obj)
{
    AppEntry e;
    e.id = obj[QStringLiteral("id")].toString();
    if (e.id.isEmpty())
        e.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    e.name = obj[QStringLiteral("name")].toString();
    e.exePath = obj[QStringLiteral("exePath")].toString();
    QString rt = obj[QStringLiteral("runtimeType")].toString();
    e.runtimeType = runtimeTypeFromString(rt);
    e.protonPath = obj[QStringLiteral("protonPath")].toString();
    e.protonPrefix = obj[QStringLiteral("protonPrefix")].toString();
    e.wineBinary = obj[QStringLiteral("wineBinary")].toString();
    e.winePrefix = obj[QStringLiteral("winePrefix")].toString();
    e.iconPath = obj[QStringLiteral("iconPath")].toString();
    e.gridPath = obj[QStringLiteral("gridPath")].toString();
    e.heroPath = obj[QStringLiteral("heroPath")].toString();
    e.logoPath = obj[QStringLiteral("logoPath")].toString();
    e.steamGridDbId = obj[QStringLiteral("steamGridDbId")].toInt(0);
    e.steamAppId = obj[QStringLiteral("steamAppId")].toInt(0);
    e.launchOptions = obj[QStringLiteral("launchOptions")].toString();
    e.enableLogging = obj[QStringLiteral("enableLogging")].toBool(false);
    return e;
}

void AppEntry::updateFromVariantMap(const QVariantMap &app)
{
    if (app.contains(QStringLiteral("appId")))
        id = app[QStringLiteral("appId")].toString();
    name = app[QStringLiteral("name")].toString();
    exePath = app[QStringLiteral("exePath")].toString();
    QString rt = app[QStringLiteral("runtimeType")].toString();
    runtimeType = runtimeTypeFromString(rt);
    protonPath = app[QStringLiteral("protonPath")].toString();
    protonPrefix = app[QStringLiteral("protonPrefix")].toString();
    wineBinary = app[QStringLiteral("wineBinary")].toString();
    winePrefix = app[QStringLiteral("winePrefix")].toString();
    iconPath = app[QStringLiteral("iconPath")].toString();
    gridPath = app[QStringLiteral("gridPath")].toString();
    heroPath = app[QStringLiteral("heroPath")].toString();
    logoPath = app[QStringLiteral("logoPath")].toString();
    steamGridDbId = app.value(QStringLiteral("steamGridDbId"), 0).toInt();
    steamAppId = app.value(QStringLiteral("steamAppId"), 0).toInt();
    launchOptions = app[QStringLiteral("launchOptions")].toString();
    enableLogging = app.value(QStringLiteral("enableLogging"), false).toBool();
}
