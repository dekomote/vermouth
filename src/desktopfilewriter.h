#pragma once

#include <QObject>
#include <QVariantMap>

class DesktopFileWriter : public QObject
{
    Q_OBJECT

public:
    explicit DesktopFileWriter(QObject *parent = nullptr);

    Q_INVOKABLE bool createStartMenuEntry(const QVariantMap &app);
    Q_INVOKABLE bool createDesktopShortcut(const QVariantMap &app);
    Q_INVOKABLE void removeShortcuts(const QVariantMap &app);

private:
    QString safeName(const QString &name) const;
    bool writeDesktopFile(const QString &filePath, const QVariantMap &app);
};
