#pragma once
#include <QObject>

class SingleInstance : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.dekomote.vermouth.App")

public:
    explicit SingleInstance(QObject *parent = nullptr);

    // Returns true if this process is the primary instance.
    bool tryRegister();

    // Called by a secondary instance to hand off work to the primary.
    void forwardToRunning(const QString &exePath);

public Q_SLOTS:
    Q_SCRIPTABLE void openExe(const QString &path);
    Q_SCRIPTABLE void raise();

Q_SIGNALS:
    void openExeRequested(const QString &path);
    void raiseRequested();
};
