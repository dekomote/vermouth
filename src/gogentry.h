#pragma once

#include <QString>

struct GogEntry {
    QString gameId;
    QString name;
    QString exePath;
    QString iconPath;
    bool isWindows = true;
};
