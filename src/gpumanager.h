#pragma once

#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QVariantList>

// Detects the GPUs in the system and knows which environment variables make a game render on a given one.
// GPUs are identified by their PCI slot (e.g. "0000:01:00.0"), which stays put when card numbers change.
// Detection runs once, when the object is created. Hot-plugged GPUs are picked up on the next start.
class GpuManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList gpus READ gpus CONSTANT)
    Q_PROPERTY(bool available READ available CONSTANT)

public:
    explicit GpuManager(const QString &drmRoot = QStringLiteral("/sys/class/drm"), QObject *parent = nullptr);

    // List of {id, name} maps, in the order the kernel reports the cards.
    QVariantList gpus() const;

    // Only worth showing a selector when there is something to choose from.
    bool available() const;

    // Environment variables that make a process use the GPU with this id. Empty if the id is unknown.
    QList<QPair<QString, QString>> environmentFor(const QString &id) const;

private:
    void detect(const QString &drmRoot);

    struct Gpu {
        QString id;
        QString name;
        QString driver;
    };

    static QString lookupName(const QString &slot, const QString &vendorId, const QString &deviceId);

    QList<Gpu> m_gpus;
};
