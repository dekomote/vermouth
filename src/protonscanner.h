#pragma once

#include <QObject>
#include <QStringList>

class ProtonScanner : public QObject {
    Q_OBJECT

public:
    explicit ProtonScanner(QObject *parent = nullptr);

    Q_INVOKABLE QStringList findProtonVersions() const;
    Q_INVOKABLE QStringList findExistingPrefixes() const;
    Q_INVOKABLE QString prefixBasePath() const;
    Q_INVOKABLE QString localProtonPath() const;
    Q_INVOKABLE QString homePath() const;

    void setExtraProtonPaths(const QStringList &paths);
    void setCustomPrefixBasePath(const QString &path);

private:
    QStringList steamPaths() const;
    QStringList m_extraProtonPaths;
    QString m_customPrefixBasePath;
};
