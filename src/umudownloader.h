#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QTimer>

class UmuDownloader : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)

public:
    explicit UmuDownloader(QObject *parent = nullptr);

    void setInstallPath(const QString &path);

    bool busy() const;
    QString statusText() const;
    double progress() const;

    Q_INVOKABLE void downloadLatest();

Q_SIGNALS:
    void busyChanged();
    void statusTextChanged();
    void progressChanged();
    void finished(const QString &umuBinPath);
    void error(const QString &message);

private:
    void onReleaseFetched(QNetworkReply *reply);
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished(QNetworkReply *reply, const QString &assetName);

    void setBusy(bool busy);
    void setStatusText(const QString &text);
    void setProgress(double progress);

    QNetworkAccessManager m_nam;
    QTimer m_statusClearTimer;
    QString m_installPath;
    bool m_busy = false;
    QString m_statusText;
    double m_progress = 0.0;
};
