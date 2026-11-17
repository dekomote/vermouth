#include "storagemigrator.h"
#include "appmodel.h"
#include "settingsmanager.h"
#include <KLocalizedString>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QProcess>
#include <QtConcurrent>

static bool moveEntry(const QString &from, const QString &to)
{
    QProcess mv;
    mv.start(QStringLiteral("mv"), {QStringLiteral("--"), from, to});
    return mv.waitForFinished(-1) && mv.exitStatus() == QProcess::NormalExit && mv.exitCode() == 0;
}

// Returns an error message, or an empty string on success.
static QString moveFolder(const QString &oldPath, const QString &newPath)
{
    if (oldPath == newPath || !QDir(oldPath).exists())
        return {};
    if (newPath.startsWith(oldPath + QLatin1Char('/')))
        return i18n("The new folder can't be inside the old one.");

    QDir().mkpath(QFileInfo(newPath).absolutePath());
    if (!QDir(newPath).exists()) {
        return moveEntry(oldPath, newPath) ? QString() : i18n("Could not move %1 to %2.", oldPath, newPath);
    }

    const QStringList children = QDir(oldPath).entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    for (const QString &child : children) {
        if (QFileInfo::exists(newPath + QLatin1Char('/') + child))
            return i18n("%1 already contains %2.", newPath, child);
    }
    for (const QString &child : children) {
        if (!moveEntry(oldPath + QLatin1Char('/') + child, newPath + QLatin1Char('/') + child))
            return i18n("Could not move %1.", child);
    }
    QDir().rmdir(oldPath);
    return {};
}

StorageMigrator::StorageMigrator(AppModel *appModel, SettingsManager *settings, QObject *parent)
    : QObject(parent)
    , m_appModel(appModel)
    , m_settings(settings)
{
}

void StorageMigrator::migrate(const QString &kind, const QString &oldPath, const QString &newPath, bool moveFiles)
{
    if (m_busy)
        return;
    m_busy = true;
    Q_EMIT busyChanged();

    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, kind, oldPath, newPath]() {
        const QString error = watcher->result();
        watcher->deleteLater();
        if (error.isEmpty()) {
            m_appModel->rewritePaths(oldPath, newPath);
            m_settings->rewritePaths(oldPath, newPath);
            if (kind == QLatin1String("assets"))
                m_settings->setAssetsDir(newPath);
            else if (kind == QLatin1String("protons"))
                m_settings->setLocalProtonDir(newPath);
            else if (kind == QLatin1String("wines"))
                m_settings->setLocalWineDir(newPath);
            else if (kind == QLatin1String("prefixes"))
                m_settings->setDefaultPrefixDir(newPath);
        }
        m_busy = false;
        Q_EMIT busyChanged();
        Q_EMIT finished(kind, error.isEmpty(), error);
    });
    watcher->setFuture(QtConcurrent::run([moveFiles, oldPath, newPath]() {
        return moveFiles ? moveFolder(oldPath, newPath) : QString();
    }));
}
