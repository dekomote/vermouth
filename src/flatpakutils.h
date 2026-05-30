#pragma once

#include <QFileInfo>

inline bool isInsideFlatpak()
{
    return QFileInfo::exists(QStringLiteral("/.flatpak-info"));
}
