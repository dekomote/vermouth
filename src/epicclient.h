#pragma once

#include "downloader.h"
#include <QProcess>
#include <QStringList>
#include <QVariantList>
#include <functional>

// Epic Games Store access through the `legendary` command line tool (the same backend Heroic uses).
class EpicClient : public Downloader
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(QString username READ username NOTIFY usernameChanged)
    Q_PROPERTY(QString installingApp READ installingApp NOTIFY installingAppChanged)

public:
    explicit EpicClient(QObject *parent = nullptr);

    bool available() const;
    bool authenticated() const;
    QString username() const;
    QString installingApp() const;

    // Path of the legendary binary, empty if it isn't installed.
    static QString findLegendary();

    Q_INVOKABLE void detect();
    Q_INVOKABLE void refreshStatus();
    Q_INVOKABLE QString loginUrl() const;
    Q_INVOKABLE QString extractCode(const QString &text) const;
    Q_INVOKABLE void authenticateWithCode(const QString &text);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void fetchLibrary();
    Q_INVOKABLE void install(const QString &appName, const QString &installDir);
    Q_INVOKABLE void update(const QString &appName);
    Q_INVOKABLE void checkUpdates();
    Q_INVOKABLE void cancelInstall();
    Q_INVOKABLE void uninstall(const QString &appName);

Q_SIGNALS:
    void availableChanged();
    void authenticatedChanged();
    void usernameChanged();
    void installingAppChanged();
    void loginSucceeded();
    void loginFailed(const QString &message);
    void libraryFetched(const QVariantList &games, const QVariantList &installed);
    void installFinished(const QString &appName, bool ok, const QString &message);
    void uninstalled(const QString &appName);
    void updatesChecked(const QStringList &appNames);
    void error(const QString &message);

private:
    QProcess *startLegendary(const QStringList &args, const QProcessEnvironment &env = QProcessEnvironment::systemEnvironment());
    void run(const QStringList &args, std::function<void(int, const QByteArray &)> done);
    void fetchInstalled(const QVariantList &games);
    void startInstallProcess(const QString &appName, const QStringList &args);
    void setAuthenticated(bool a);
    void setUsername(const QString &name);

    QString m_legendary;
    QString m_username;
    bool m_authenticated = false;
    QProcess *m_installProc = nullptr;
    QString m_installingApp;
    bool m_installCancelled = false;
};
