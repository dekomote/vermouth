#include "colorschemeswitcher.h"

#include <KColorScheme>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QApplication>

namespace
{
struct SchemeEntry {
    const char *name;
    const char *resourcePath; // qrc source; empty = follow the system scheme
};

// POC list: swap/extend freely while testing.
constexpr SchemeEntry kSchemes[] = {
    {"Default", ""},
    {"Vermouth Light", ":/colors/VermouthLight.colors"},
    {"Vermouth Dark", ":/colors/VermouthDark.colors"},
};
constexpr int kSchemeCount = sizeof(kSchemes) / sizeof(kSchemes[0]);

// Groups a KDE .colors scheme defines. Kirigami's own org.kde.desktop theme
// backend (plasmadesktoptheme.cpp) builds its colors via
// KColorScheme(group, set) with no config argument, i.e. from
// KSharedConfig::openConfig() - the app's own config, cascaded with
// kdeglobals. So instead of installing a named scheme file anywhere
// discoverable, we write these groups straight into that same config: both
// KColorScheme::createApplicationPalette() and Kirigami's theme backend end
// up reading the exact same values, with nothing visible outside the app.
constexpr const char *kManagedGroups[] = {
    "Colors:View",
    "Colors:Window",
    "Colors:Button",
    "Colors:Selection",
    "Colors:Tooltip",
    "Colors:Complementary",
    "Colors:Header",
    "ColorEffects:Disabled",
    "ColorEffects:Inactive",
    "WM",
};
}

ColorSchemeSwitcher::ColorSchemeSwitcher(QObject *parent)
    : QObject(parent)
{
}

QString ColorSchemeSwitcher::currentSchemeName() const
{
    return QString::fromUtf8(kSchemes[m_index].name);
}

void ColorSchemeSwitcher::cycleScheme()
{
    m_index = (m_index + 1) % kSchemeCount;
    applySchemeAt(m_index);
}

void ColorSchemeSwitcher::applySchemeAt(int index)
{
    const SchemeEntry &entry = kSchemes[index];
    const QString resourcePath = QString::fromUtf8(entry.resourcePath);

    KSharedConfigPtr appConfig = KSharedConfig::openConfig();

    if (resourcePath.isEmpty()) {
        for (const char *group : kManagedGroups)
            appConfig->deleteGroup(QString::fromUtf8(group));
    } else {
        KSharedConfigPtr scheme = KSharedConfig::openConfig(resourcePath, KConfig::SimpleConfig);
        for (const char *group : kManagedGroups) {
            const QString groupName = QString::fromUtf8(group);
            KConfigGroup source(scheme, groupName);
            if (!source.exists())
                continue;
            KConfigGroup dest(appConfig, groupName);
            dest.deleteGroup();
            const auto entries = source.entryMap();
            for (auto it = entries.constBegin(); it != entries.constEnd(); ++it)
                dest.writeEntry(it.key(), it.value());
        }
    }

    appConfig->sync();

    // Also triggers QEvent::ApplicationPaletteChange, which is what makes
    // Kirigami's theme backend re-read (the now-updated) appConfig.
    qApp->setPalette(KColorScheme::createApplicationPalette(appConfig));

    Q_EMIT schemeChanged();
}
