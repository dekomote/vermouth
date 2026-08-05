#pragma once

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUuid>
#include <QVariantMap>

struct RuntimeTypeEntry {
    const char *key;
    const char *label;
};

class AppEntry
{
    Q_GADGET
public:
    enum RuntimeType {
        Proton,
        Wine,
        Native,
        Steam,
        Retroarch,
        Uzdoom,
        Count
    };
    Q_ENUM(RuntimeType)

    static const RuntimeTypeEntry runtimeTypeTable[];
    static const int runtimeTypeCount;

    static const char *runtimeTypeString(RuntimeType rt);
    static RuntimeType runtimeTypeFromString(const QString &s);

    QString id;
    QString name;
    QString exePath;
    RuntimeType runtimeType = Proton;

    QString protonPath;
    QString protonPrefix;

    QString wineBinary;
    QString winePrefix;

    QString iconPath;
    QString gridPath;
    QString heroPath;
    QString logoPath;
    int steamGridDbId = 0;
    int steamAppId = 0;
    QString platformSlug;
    QString customCorePath;
    QString uzdoomPath;
    QStringList uzdoomMods;
    QString launchOptions;
    bool enableLogging = false;
    bool hidden = false;
    QDateTime dateAdded;

    QJsonObject toJson() const;
    QVariantMap toVariantMap() const;
    static AppEntry fromJson(const QJsonObject &obj);
    void updateFromVariantMap(const QVariantMap &map);
};
