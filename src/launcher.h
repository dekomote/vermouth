#pragma once

#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>

class Launcher : public QObject {
    Q_OBJECT

public:
    explicit Launcher(QObject *parent = nullptr);

    Q_INVOKABLE void launchEntry(const QVariantMap &app);
    Q_INVOKABLE void runInPrefix(const QVariantMap &app, const QString &exePath);
    Q_INVOKABLE QString logDir() const;

Q_SIGNALS:
    void launched(const QString &name);
    void launchError(const QString &name, const QString &error);
    void processFinished(int exitCode);

private:
    void launch(const QString &binary, const QStringList &baseArgs,
                const QString &exePath, const QProcessEnvironment &env,
                const QString &launchOptions, bool enableLogging,
                const QString &logName);
    void setupLogging(QProcess *proc, const QString &name);
    QString m_logDir;
};
