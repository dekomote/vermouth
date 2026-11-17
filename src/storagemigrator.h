#pragma once

#include <QObject>
#include <QString>

class AppModel;
class SettingsManager;

// Moves one of Vermouth's storage folders (assets, protons, wines) and points games and settings at the new location.
class StorageMigrator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    StorageMigrator(AppModel *appModel, SettingsManager *settings, QObject *parent = nullptr);

    bool busy() const
    {
        return m_busy;
    }

    // kind is "assets", "protons", "wines" or "prefixes".
    // With moveFiles false only the paths saved in games and settings are updated.
    Q_INVOKABLE void migrate(const QString &kind, const QString &oldPath, const QString &newPath, bool moveFiles = true);

Q_SIGNALS:
    void busyChanged();
    void finished(const QString &kind, bool ok, const QString &message);

private:
    AppModel *m_appModel;
    SettingsManager *m_settings;
    bool m_busy = false;
};
