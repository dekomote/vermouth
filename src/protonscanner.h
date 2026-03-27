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

private:
    QStringList steamPaths() const;
};
