#pragma once

#include <QObject>
#include <QVariantMap>

// Creates non-Steam shortcuts in Steam's shortcuts.vdf. Entries are re-indexed
// 0..N-1 on every write (Steam stops importing at the first index gap) and end
// with exactly two 0x08 bytes. Each shortcut points at a unique per-game
// wrapper script that execs Vermouth --launch-id.
class SteamShortcutWriter : public QObject
{
    Q_OBJECT

public:
    explicit SteamShortcutWriter(QObject *parent = nullptr);

    Q_INVOKABLE bool isAvailable() const;
    Q_INVOKABLE bool createShortcut(const QVariantMap &app);

private:
    struct ParsedEntry {
        QByteArray content; // props + terminator, head excluded (reindexed on write)
        QString name;
        QString exe;
    };

    QString steamRoot() const;
    QString userDataDir() const;
    QString shortcutsPath() const;
    QString shortcutScriptDir() const;
    QString shortcutScriptFor(const QString &id) const;
    bool writeShortcutScript(const QString &id) const;

    QByteArray encodeEntry(int index, const QVariantMap &app) const;
    quint32 shortcutAppId(const QString &quotedExe, const QString &appName) const;
    void installArtwork(quint32 appid, const QVariantMap &app);
    QList<ParsedEntry> parseEntries(const QByteArray &data) const;
};