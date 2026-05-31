#pragma once

#include "gogentry.h"
#include <QVector>

class GogLibrary
{
public:
    static QVector<GogEntry> scan(const QString &rootPath);
};
