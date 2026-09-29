#pragma once

#include <QObject>
#include <QVariantMap>

class QProcess;
class QProcessEnvironment;

// Unlike GOG, itch.io uploads are almost always portable archives (zip/tar)
// rather than InnoSetup-style installers, so this just extracts and scans for
// the game's executable. A bare .exe installer (no archive) is the rare case,
// handled via runWindowsInstaller the same way GOG's manual install works.
class ItchInstaller : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit ItchInstaller(QObject *parent = nullptr);

    void setUmuPath(const QString &path);

    bool busy() const;

    // Extracts (or, for a bare executable/AppImage, just copies) downloadedPath
    // into destDir. Emits installFinished/installError.
    Q_INVOKABLE void extractDownload(const QString &gameId, const QString &downloadedPath, const QString &destDir);

    Q_INVOKABLE void
    runWindowsInstaller(const QString &gameId, const QString &installerExe, const QString &runtimeType, const QString &runtimePath, const QString &prefix);

    // Scans destDir for the most likely game executable for the given platform.
    Q_INVOKABLE QVariantMap findGame(const QString &destDir, const QString &gameName, bool isWindows) const;

    // True if downloadedPath is a bare, directly-runnable file (not an archive) -
    // i.e. extractDownload will have just copied it rather than unpacked it.
    Q_INVOKABLE bool isBareExecutable(const QString &downloadedPath) const;

    // Copies an already-cached itch.io cover image into assetsPath as a game's
    // grid art, following the same "<safeName>_grid.<ext>" naming SteamGridDB
    // downloads use. Returns the final path, or empty on failure.
    Q_INVOKABLE QString copyGridCover(const QString &sourcePath, const QString &assetsPath, const QString &safeName) const;

Q_SIGNALS:
    void busyChanged();
    void installStarted(const QString &gameId);
    void installFinished(const QString &gameId, int exitCode);
    void installError(const QString &gameId, const QString &message);

private:
    void runProcess(const QString &gameId, const QString &program, const QStringList &args, const QProcessEnvironment &env, const QString &workingDir);

    QString m_umuPath;
    bool m_busy = false;
};
