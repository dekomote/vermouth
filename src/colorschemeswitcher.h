#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class ColorSchemeSwitcher : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList schemeNames READ schemeNames CONSTANT)
    Q_PROPERTY(bool needsShowWorkaround READ needsShowWorkaround CONSTANT)

public:
    explicit ColorSchemeSwitcher(QObject *parent = nullptr);

    QStringList schemeNames() const;

    // True when NOT running under the org.kde.desktop QQC2 style (e.g.
    // GNOME, AppImage without Plasma integration). On org.kde.desktop,
    // Popup/Menu/Drawer content already picks up the active scheme
    // correctly on its own, so QML should skip the reapplyCurrent()
    // workaround there.
    bool needsShowWorkaround() const;

    Q_INVOKABLE QString schemeIdAt(int index) const;
    Q_INVOKABLE int indexForSchemeId(const QString &schemeId) const;
    Q_INVOKABLE void applySchemeId(const QString &schemeId);

    // Re-applies whatever scheme is currently active. Some Qt Quick
    // Controls (e.g. Popup/Menu/Drawer content) are realized lazily on
    // first show and don't pick up a palette set before they existed -
    // call this from such a control's onAboutToShow the first time it
    // appears (guarded by needsShowWorkaround).
    Q_INVOKABLE void reapplyCurrent();

private:
    QString m_currentSchemeId;
};
