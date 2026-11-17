#pragma once

#include <QObject>
#include <QStringList>

class ProtonScanner : public QObject
{
    Q_OBJECT

public:
    explicit ProtonScanner(QObject *parent = nullptr);

    Q_INVOKABLE QStringList findProtonVersions() const;
    Q_INVOKABLE bool isInstalled(const QString &path) const;
    Q_INVOKABLE QString prefixBasePath() const;
    Q_INVOKABLE QString defaultPrefixBasePath() const;
    Q_INVOKABLE QString winePrefixBasePath() const;
    Q_INVOKABLE QString localProtonPath() const;
    Q_INVOKABLE QString localAssetsPath() const;
    Q_INVOKABLE QString homePath() const;
    // Steam's compatibilitytools.d folders, for every detected Steam install.
    Q_INVOKABLE QStringList steamCompatToolsPaths() const;

    void setExtraProtonPaths(const QStringList &paths);
    void setCustomPrefixBasePath(const QString &path);
    void setCustomLocalProtonPath(const QString &path);
    void setCustomAssetsPath(const QString &path);
    Q_INVOKABLE QString defaultLocalProtonPath() const;
    Q_INVOKABLE QString defaultAssetsPath() const;

private:
    QStringList m_extraProtonPaths;
    QString m_customPrefixBasePath;
    QString m_customLocalProtonPath;
    QString m_customAssetsPath;
};
