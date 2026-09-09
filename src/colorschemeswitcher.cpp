#include "colorschemeswitcher.h"

#include <KColorSchemeManager>
#include <QModelIndex>

namespace
{
// Index 0 ("System") has no real scheme name - it maps to an empty string,
// which both indexForScheme() and the persisted settings value treat as
// "follow the system scheme".
constexpr const char *kSchemeNames[] = {
    "System",
    "Vermouth Dark",
    "Vermouth Deep",
    "Vermouth Tokyo Night",
    "Vermouth Cobalt",
    "Vermouth 2077",
    "Vermouth Rose",
    "Vermouth Magenta",
    "Vermouth Sage",
};
constexpr int kSchemeCount = sizeof(kSchemeNames) / sizeof(kSchemeNames[0]);
}

ColorSchemeSwitcher::ColorSchemeSwitcher(QObject *parent)
    : QObject(parent)
{
}

QStringList ColorSchemeSwitcher::schemeNames() const
{
    QStringList names;
    names.reserve(kSchemeCount);
    for (const char *name : kSchemeNames)
        names.append(QString::fromUtf8(name));
    return names;
}

QString ColorSchemeSwitcher::schemeIdAt(int index) const
{
    if (index <= 0 || index >= kSchemeCount)
        return QString();
    return QString::fromUtf8(kSchemeNames[index]);
}

int ColorSchemeSwitcher::indexForSchemeId(const QString &schemeId) const
{
    if (schemeId.isEmpty())
        return 0;
    for (int i = 1; i < kSchemeCount; ++i) {
        if (QString::fromUtf8(kSchemeNames[i]) == schemeId)
            return i;
    }
    return 0;
}

void ColorSchemeSwitcher::applySchemeId(const QString &schemeId)
{
    // indexForScheme("") / an invalid index both mean "follow the system
    // scheme" (since KF 5.67) - this is the portable pre-6.19 API, unlike
    // activateSchemeId(QString), which some distros (e.g. Ubuntu 25.10's
    // kf6-kcolorscheme 6.17.0) don't ship yet.
    KColorSchemeManager *manager = KColorSchemeManager::instance();
    manager->activateScheme(manager->indexForScheme(schemeId));
}
