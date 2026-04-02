#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QTimer>

class ProtonDownloader : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)

public:
    explicit ProtonDownloader(QObject *parent = nullptr);

    void setLocalProtonPath(const QString &path);

    bool busy() const;
    QString statusText() const;
    double progress() const;

    Q_INVOKABLE void downloadLatest();

Q_SIGNALS:
    void busyChanged();
    void statusTextChanged();
    void progressChanged();
    void finished();
    void error(const QString &message);

private Q_SLOTS:
    void onReleaseFetched(QNetworkReply *reply);
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished(QNetworkReply *reply);

private:
    bool extractTarGz(const QString &archivePath, const QString &destDir);

    QNetworkAccessManager m_nam;
    QTimer m_statusClearTimer;
    QString m_localProtonPath;
    bool m_busy = false;
    QString m_statusText;
    double m_progress = 0.0;

    void setBusy(bool busy);
    void setStatusText(const QString &text);
    void setProgress(double progress);
};
