#include "appentry.h"

const RuntimeTypeEntry AppEntry::runtimeTypeTable[] = {
    {"proton", "Proton"},
    {"wine", "Wine"},
    {"native", "Native"},
    {"steam", "Steam"},
    {"retroarch", "Retroarch"},
    {"uzdoom", "UZDOOM (beta)"},
    {"default", "Default"},
};
const int AppEntry::runtimeTypeCount = sizeof(runtimeTypeTable) / sizeof(runtimeTypeTable[0]);
static_assert(AppEntry::runtimeTypeCount == AppEntry::Count, "runtimeTypeTable and RuntimeType enum are out of sync");

const char *AppEntry::runtimeTypeString(RuntimeType rt)
{
    if (rt >= 0 && rt < Count)
        return runtimeTypeTable[rt].key;
    return "native";
}

AppEntry::RuntimeType AppEntry::runtimeTypeFromString(const QString &s)
{
    for (int i = 0; i < Count; ++i) {
        if (s == QLatin1String(runtimeTypeTable[i].key))
            return static_cast<RuntimeType>(i);
    }
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
    obj[QStringLiteral("platformSlug")] = platformSlug;
    obj[QStringLiteral("customCorePath")] = customCorePath;
    obj[QStringLiteral("uzdoomPath")] = uzdoomPath;
    obj[QStringLiteral("uzdoomMods")] = QJsonArray::fromStringList(uzdoomMods);
    obj[QStringLiteral("launchOptions")] = launchOptions;
    obj[QStringLiteral("enableLogging")] = enableLogging;
    obj[QStringLiteral("hidden")] = hidden;
    obj[QStringLiteral("playTime")] = static_cast<double>(playTime);
    obj[QStringLiteral("dateAdded")] = dateAdded.toString(Qt::ISODate);
    obj[QStringLiteral("protonGameId")] = protonGameId;
    obj[QStringLiteral("enableMangohud")] = enableMangohud;
    obj[QStringLiteral("enableGamemode")] = enableGamemode;
    obj[QStringLiteral("enablePreferSdl")] = enablePreferSdl;
    obj[QStringLiteral("enableLsfg")] = enableLsfg;
    obj[QStringLiteral("lsfgMultiplier")] = lsfgMultiplier;
    obj[QStringLiteral("lsfgFlowScale")] = lsfgFlowScale;
    obj[QStringLiteral("lsfgPerformanceMode")] = lsfgPerformanceMode;
    obj[QStringLiteral("lsfgPresentMode")] = lsfgPresentMode;
    obj[QStringLiteral("envVars")] = QJsonArray::fromStringList(envVars);
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
        {QStringLiteral("platformSlug"), platformSlug},
        {QStringLiteral("customCorePath"), customCorePath},
        {QStringLiteral("uzdoomPath"), uzdoomPath},
        {QStringLiteral("uzdoomMods"), uzdoomMods},
        {QStringLiteral("launchOptions"), launchOptions},
        {QStringLiteral("enableLogging"), enableLogging},
        {QStringLiteral("hidden"), hidden},
        {QStringLiteral("playTime"), playTime},
        {QStringLiteral("dateAdded"), dateAdded},
        {QStringLiteral("protonGameId"), protonGameId},
        {QStringLiteral("enableMangohud"), enableMangohud},
        {QStringLiteral("enableGamemode"), enableGamemode},
        {QStringLiteral("enablePreferSdl"), enablePreferSdl},
        {QStringLiteral("enableLsfg"), enableLsfg},
        {QStringLiteral("lsfgMultiplier"), lsfgMultiplier},
        {QStringLiteral("lsfgFlowScale"), lsfgFlowScale},
        {QStringLiteral("lsfgPerformanceMode"), lsfgPerformanceMode},
        {QStringLiteral("lsfgPresentMode"), lsfgPresentMode},
        {QStringLiteral("envVars"), envVars},
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
    e.platformSlug = obj[QStringLiteral("platformSlug")].toString();
    e.customCorePath = obj[QStringLiteral("customCorePath")].toString();
    e.uzdoomPath = obj[QStringLiteral("uzdoomPath")].toString();
    QJsonArray modsArr = obj[QStringLiteral("uzdoomMods")].toArray();
    for (const auto &v : modsArr)
        e.uzdoomMods.append(v.toString());
    e.launchOptions = obj[QStringLiteral("launchOptions")].toString();
    e.enableLogging = obj[QStringLiteral("enableLogging")].toBool(false);
    e.hidden = obj[QStringLiteral("hidden")].toBool(false);
    e.playTime = obj.value(QStringLiteral("playTime")).toInteger(0);
    e.dateAdded = QDateTime::fromString(obj[QStringLiteral("dateAdded")].toString(), Qt::ISODate);
    e.protonGameId = obj[QStringLiteral("protonGameId")].toString();
    e.enableMangohud = obj[QStringLiteral("enableMangohud")].toBool(false);
    e.enableGamemode = obj[QStringLiteral("enableGamemode")].toBool(false);
    e.enablePreferSdl = obj[QStringLiteral("enablePreferSdl")].toBool(false);
    e.enableLsfg = obj[QStringLiteral("enableLsfg")].toBool(false);
    e.lsfgMultiplier = obj[QStringLiteral("lsfgMultiplier")].toInt(2);
    e.lsfgFlowScale = obj[QStringLiteral("lsfgFlowScale")].toInt(50);
    e.lsfgPerformanceMode = obj[QStringLiteral("lsfgPerformanceMode")].toBool(false);
    e.lsfgPresentMode = obj[QStringLiteral("lsfgPresentMode")].toString();
    const QJsonArray envArr = obj[QStringLiteral("envVars")].toArray();
    for (const QJsonValue &v : envArr)
        e.envVars << v.toString();
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
    platformSlug = app[QStringLiteral("platformSlug")].toString();
    customCorePath = app[QStringLiteral("customCorePath")].toString();
    uzdoomPath = app[QStringLiteral("uzdoomPath")].toString();
    uzdoomMods = app.value(QStringLiteral("uzdoomMods")).toStringList();
    launchOptions = app[QStringLiteral("launchOptions")].toString();
    enableLogging = app.value(QStringLiteral("enableLogging"), false).toBool();
    hidden = app.value(QStringLiteral("hidden"), false).toBool();
    playTime = app.value(QStringLiteral("playTime"), 0).toLongLong();
    protonGameId = app.value(QStringLiteral("protonGameId"), QString()).toString();
    enableMangohud = app.value(QStringLiteral("enableMangohud"), false).toBool();
    enableGamemode = app.value(QStringLiteral("enableGamemode"), false).toBool();
    enablePreferSdl = app.value(QStringLiteral("enablePreferSdl"), false).toBool();
    enableLsfg = app.value(QStringLiteral("enableLsfg"), false).toBool();
    lsfgMultiplier = app.value(QStringLiteral("lsfgMultiplier"), 2).toInt();
    lsfgFlowScale = app.value(QStringLiteral("lsfgFlowScale"), 50).toInt();
    lsfgPerformanceMode = app.value(QStringLiteral("lsfgPerformanceMode"), false).toBool();
    lsfgPresentMode = app.value(QStringLiteral("lsfgPresentMode"), QString()).toString();
    envVars = app.value(QStringLiteral("envVars"), QStringList()).toStringList();
}
