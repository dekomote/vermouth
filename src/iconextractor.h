#pragma once

#include <QObject>
#include <QString>

class IconExtractor : public QObject
{
    Q_OBJECT

public:
    explicit IconExtractor(QObject *parent = nullptr);

    Q_INVOKABLE QString extractIcon(const QString &exePath);

private:
    QString cacheDir() const;
    QString tryWrestool(const QString &exePath, const QString &outPath);
};
