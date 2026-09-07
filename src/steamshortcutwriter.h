#pragma once

#include <QObject>
#include <QVariantMap>

class SteamShortcutWriter : public QObject
{
    Q_OBJECT

public:
    explicit SteamShortcutWriter(QObject *parent = nullptr);

    Q_INVOKABLE bool isAvailable() const;
    Q_INVOKABLE bool createShortcut(const QVariantMap &app);

private:
    QString steamRoot() const;
    QString userDataDir() const;
    QString shortcutsPath() const;
    QString shortcutExe() const;
    QString shortcutLaunchOptions(const QString &id) const;

    QByteArray encodeEntry(int index, const QVariantMap &app) const;
    quint32 shortcutAppId(const QString &quotedExe, const QString &appName) const;
    void installArtwork(quint32 appid, const QVariantMap &app);
};
