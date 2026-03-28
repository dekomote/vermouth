#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

class AppEntry {
    Q_GADGET
public:
    enum RuntimeType { Proton, Wine };

    QString name;
    QString exePath;
    RuntimeType runtimeType = Proton;

    QString protonPath;
    QString protonPrefix;

    QString wineBinary;
    QString winePrefix;

    QString iconPath;
    QString launchOptions;
    bool enableLogging = false;

    QJsonObject toJson() const;
    static AppEntry fromJson(const QJsonObject &obj);
};
