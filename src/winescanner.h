#pragma once

#include <QObject>
#include <QVariantList>

class WineScanner : public QObject
{
    Q_OBJECT

public:
    explicit WineScanner(QObject *parent = nullptr);

    Q_INVOKABLE QVariantList findWineVersions() const;
    Q_INVOKABLE bool isInstalled(const QString &path) const;
    Q_INVOKABLE QString localWinePath() const;
    Q_INVOKABLE QString defaultLocalWinePath() const;
    void setCustomLocalWinePath(const QString &path);

private:
    QString m_customLocalWinePath;
};
