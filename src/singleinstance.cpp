#include "singleinstance.h"
#include <QDBusConnection>
#include <QDBusInterface>

#define VERMOUTH_SERVICE "com.dekomote.vermouth"
#define VERMOUTH_PATH "/com/dekomote/vermouth"
#define VERMOUTH_INTERFACE "com.dekomote.vermouth.App"

SingleInstance::SingleInstance(QObject *parent)
    : QObject(parent)
{
}

bool SingleInstance::tryRegister()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerService(QStringLiteral(VERMOUTH_SERVICE)))
        return false;
    bus.registerObject(QStringLiteral(VERMOUTH_PATH), this, QDBusConnection::ExportScriptableSlots);
    return true;
}

void SingleInstance::forwardToRunning(const QString &exePath)
{
    QDBusInterface iface(QStringLiteral(VERMOUTH_SERVICE), QStringLiteral(VERMOUTH_PATH), QStringLiteral(VERMOUTH_INTERFACE), QDBusConnection::sessionBus());
    if (!exePath.isEmpty())
        iface.call(QStringLiteral("openExe"), exePath);
    iface.call(QStringLiteral("raise"));
}

void SingleInstance::openExe(const QString &path)
{
    Q_EMIT openExeRequested(path);
}

void SingleInstance::raise()
{
    Q_EMIT raiseRequested();
}
