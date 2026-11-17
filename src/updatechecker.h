#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QVariantList>
#include <functional>

class SettingsManager;
class ProtonScanner;
class ProtonDownloader;
class UmuDownloader;

// Looks for newer releases of the umu launcher and the default Proton build, and installs them on request.
class UpdateChecker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList updates READ updates NOTIFY updatesChanged)
    Q_PROPERTY(bool applying READ applying NOTIFY applyingChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY progressChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)

public:
    UpdateChecker(SettingsManager *settings, ProtonScanner *scanner, ProtonDownloader *proton, UmuDownloader *umu, QObject *parent = nullptr);

    QVariantList updates() const
    {
        return m_updates;
    }
    bool applying() const
    {
        return m_applying;
    }
    QString statusText() const;
    double progress() const;

    Q_INVOKABLE void check();
    Q_INVOKABLE void applyUpdates();
    Q_INVOKABLE void skipUpdates();

Q_SIGNALS:
    void updatesChanged();
    void applyingChanged();
    void progressChanged();
    void updatesAvailable();
    void applyFinished();

private:
    void fetchTag(const QString &releaseUrl, std::function<void(const QString &)> done);
    void addUpdate(const QVariantMap &update);
    void applyNext();

    SettingsManager *m_settings;
    ProtonScanner *m_scanner;
    ProtonDownloader *m_proton;
    UmuDownloader *m_umu;
    QNetworkAccessManager m_nam;
    QVariantList m_updates;
    QVariantList m_queue;
    bool m_applying = false;
    int m_pendingChecks = 0;
    QString m_activeKind;
};
