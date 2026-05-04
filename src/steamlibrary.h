#pragma once

#include <QString>
#include <QVector>

struct SteamEntry {
    int appId = 0;
    QString name;
    QString installDir;
    QString heroPath;
    QString gridPath;
    QString iconPath;
    QString logoPath;
};

class SteamLibrary
{
public:
    static QStringList steamRootPaths();
    static QVector<SteamEntry> scan();

private:
    static void parseManifest(const QString &path, QVector<SteamEntry> &out);
};
