#include "gpumanager.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>

GpuManager::GpuManager(const QString &drmRoot, QObject *parent)
    : QObject(parent)
{
    detect(drmRoot);
}

QVariantList GpuManager::gpus() const
{
    QVariantList list;
    for (const Gpu &gpu : m_gpus) {
        QVariantMap entry;
        entry[QStringLiteral("id")] = gpu.id;
        entry[QStringLiteral("name")] = gpu.name;
        list << entry;
    }
    return list;
}

bool GpuManager::available() const
{
    return m_gpus.size() > 1;
}

static QMap<QString, QString> readUevent(const QString &path)
{
    QMap<QString, QString> values;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return values;
    // sysfs files report a bogus size, so atEnd() never turns true. Read it in one go instead.
    const QStringList lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        const int eq = line.indexOf(QLatin1Char('='));
        if (eq > 0)
            values.insert(line.left(eq), line.mid(eq + 1));
    }
    return values;
}

void GpuManager::detect(const QString &drmRoot)
{
    QList<Gpu> found;
    QList<QPair<QString, QString>> pciIds;
    const QDir drm(drmRoot);
    const QStringList cards = drm.entryList({QStringLiteral("card*")}, QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    static const QRegularExpression cardName(QStringLiteral("^card\\d+$"));

    for (const QString &card : cards) {
        if (!cardName.match(card).hasMatch())
            continue;
        const QMap<QString, QString> info = readUevent(drm.filePath(card + QStringLiteral("/device/uevent")));
        const QString slot = info.value(QStringLiteral("PCI_SLOT_NAME"));
        if (slot.isEmpty())
            continue;
        bool duplicate = false;
        for (const Gpu &existing : std::as_const(found))
            duplicate = duplicate || existing.id == slot;
        if (duplicate)
            continue;

        const QStringList pciId = info.value(QStringLiteral("PCI_ID")).toLower().split(QLatin1Char(':'));
        Gpu gpu;
        gpu.id = slot;
        gpu.name = slot;
        gpu.driver = info.value(QStringLiteral("DRIVER"));
        found << gpu;
        pciIds << qMakePair(pciId.value(0), pciId.value(1));
    }

    // Names are only needed to tell GPUs apart in the UI, so skip the lspci calls on single-GPU systems.
    if (found.size() > 1)
        for (int i = 0; i < found.size(); ++i)
            found[i].name = lookupName(found.at(i).id, pciIds.at(i).first, pciIds.at(i).second);

    // Identical cards would show up as identical entries, so tell them apart by slot.
    QHash<QString, int> nameCount;
    for (const Gpu &gpu : std::as_const(found))
        ++nameCount[gpu.name];
    for (Gpu &gpu : found)
        if (nameCount.value(gpu.name) > 1)
            gpu.name += QStringLiteral(" (%1)").arg(gpu.id);

    m_gpus = found;
}

QString GpuManager::lookupName(const QString &slot, const QString &vendorId, const QString &deviceId)
{
    QString vendor;
    if (vendorId == QStringLiteral("10de"))
        vendor = QStringLiteral("NVIDIA");
    else if (vendorId == QStringLiteral("1002"))
        vendor = QStringLiteral("AMD");
    else if (vendorId == QStringLiteral("8086"))
        vendor = QStringLiteral("Intel");

    QString device;
    const QString lspci = QStandardPaths::findExecutable(QStringLiteral("lspci"));
    if (!lspci.isEmpty()) {
        QProcess proc;
        proc.start(lspci, {QStringLiteral("-mm"), QStringLiteral("-s"), slot});
        if (proc.waitForFinished(2000) && proc.exitStatus() == QProcess::NormalExit) {
            // 01:00.0 "VGA compatible controller" "NVIDIA Corporation" "AD103 [GeForce RTX 4080 SUPER]" ...
            static const QRegularExpression quoted(QStringLiteral("\"([^\"]*)\""));
            QStringList fields;
            auto it = quoted.globalMatch(QString::fromUtf8(proc.readAllStandardOutput()));
            while (it.hasNext())
                fields << it.next().captured(1);
            if (fields.size() >= 3) {
                if (vendor.isEmpty())
                    vendor = fields.at(1);
                device = fields.at(2);
                // The marketing name is the last bracketed part, the rest is the chip codename.
                static const QRegularExpression bracket(QStringLiteral("\\[([^\\]]+)\\]\\s*$"));
                const QRegularExpressionMatch m = bracket.match(device);
                if (m.hasMatch())
                    device = m.captured(1);
            }
        }
    }

    if (device.isEmpty())
        device = QStringLiteral("GPU %1").arg(deviceId);
    return vendor.isEmpty() ? device : vendor + QLatin1Char(' ') + device;
}

QList<QPair<QString, QString>> GpuManager::environmentFor(const QString &id) const
{
    const Gpu *chosen = nullptr;
    bool hasNvidia = false;
    for (const Gpu &gpu : m_gpus) {
        hasNvidia = hasNvidia || gpu.driver == QStringLiteral("nvidia");
        if (gpu.id == id)
            chosen = &gpu;
    }
    if (!chosen)
        return {};

    // DRI_PRIME names the card by PCI slot (pci-0000_03_00_0). Mesa's Vulkan device selection honours it for
    // every Vulkan device, NVIDIA included, which is what DXVK and VKD3D use, and it tells identical cards apart.
    QString slot = chosen->id;
    slot.replace(QLatin1Char(':'), QLatin1Char('_')).replace(QLatin1Char('.'), QLatin1Char('_'));
    QList<QPair<QString, QString>> env{{QStringLiteral("DRI_PRIME"), QStringLiteral("pci-") + slot}};

    // OpenGL goes through GLVND, which DRI_PRIME doesn't steer once the NVIDIA driver is installed.
    if (chosen->driver == QStringLiteral("nvidia")) {
        env.append({QStringLiteral("__NV_PRIME_RENDER_OFFLOAD"), QStringLiteral("1")});
        env.append({QStringLiteral("__GLX_VENDOR_LIBRARY_NAME"), QStringLiteral("nvidia")});
    } else if (hasNvidia) {
        env.append({QStringLiteral("__GLX_VENDOR_LIBRARY_NAME"), QStringLiteral("mesa")});
    }
    return env;
}
