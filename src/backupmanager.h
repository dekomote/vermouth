#pragma once

#include <QObject>
#include <QString>

class AppModel;
class SettingsManager;

// Saves and restores the settings file and the game list. Every method returns an error message, or an empty string on success.
class BackupManager : public QObject
{
    Q_OBJECT

public:
    BackupManager(AppModel *appModel, SettingsManager *settings, QObject *parent = nullptr);

    Q_INVOKABLE QString backupSettings(const QString &destination) const;
    Q_INVOKABLE QString restoreSettings(const QString &source);
    Q_INVOKABLE QString backupGames(const QString &destination) const;
    Q_INVOKABLE QString restoreGames(const QString &source);

private:
    AppModel *m_appModel;
    SettingsManager *m_settings;
};
