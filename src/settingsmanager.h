#pragma once

#include <QObject>
#include <QSettings>
#include <QStringList>

class SettingsManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString defaultPrefixDir READ defaultPrefixDir WRITE setDefaultPrefixDir NOTIFY defaultPrefixDirChanged)
    Q_PROPERTY(QStringList extraProtonPaths READ extraProtonPaths WRITE setExtraProtonPaths NOTIFY extraProtonPathsChanged)

public:
    explicit SettingsManager(QObject *parent = nullptr);

    QString defaultPrefixDir() const;
    Q_INVOKABLE void setDefaultPrefixDir(const QString &path);

    QStringList extraProtonPaths() const;
    void setExtraProtonPaths(const QStringList &paths);

    Q_INVOKABLE void addExtraProtonPath(const QString &path);
    Q_INVOKABLE void removeExtraProtonPath(int index);

Q_SIGNALS:
    void defaultPrefixDirChanged();
    void extraProtonPathsChanged();

private:
    QSettings m_settings;
};
