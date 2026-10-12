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
        if (pciId.size() == 2)
            gpu.pciId = pciId.join(QLatin1Char(':'));
        gpu.driver = info.value(QStringLiteral("DRIVER"));
        found << gpu;
        pciIds << qMakePair(pciId.value(0), pciId.value(1));
    }

    // Names are only needed to tell GPUs apart in the UI, so skip the lspci calls on single-GPU systems.
    if (found.size() > 1)
        for (int i = 0; i < found.size(); ++i)
            found[i].name = lookupName(found.at(i).id, pciIds.at(i).first, pciIds.at(i).second);

    if (found.size() > 1) {
        const QHash<QString, QString> uuids = vulkanDeviceUuids();
        for (Gpu &gpu : found) {
            gpu.icdFiles = findIcdFiles(gpu.driver);
            gpu.deviceUuid = uuids.value(gpu.pciId);
        }
    }

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

// Vulkan driver manifests (ICD files) for a kernel driver, searched where the loader looks.
QString GpuManager::findIcdFiles(const QString &driver)
{
    static const QHash<QString, QString> loaders{
        {QStringLiteral("amdgpu"), QStringLiteral("radeon")},
        {QStringLiteral("vc4-drm"), QStringLiteral("broadcom")},
        {QStringLiteral("v3d"), QStringLiteral("broadcom")},
        {QStringLiteral("virtio-pci"), QStringLiteral("lvp")},
        {QStringLiteral("i915"), QStringLiteral("intel")},
        {QStringLiteral("xe"), QStringLiteral("intel")},
    };
    const QString loader = loaders.value(driver, driver);
    if (loader.isEmpty())
        return {};

    static const QStringList dataDirs{
        QStringLiteral("/usr/local/etc"),
        QStringLiteral("/usr/local/share"),
        QStringLiteral("/etc"),
        QStringLiteral("/usr/share"),
        QStringLiteral("/usr/lib/x86_64-linux-gnu/GL"),
        QStringLiteral("/usr/lib/i386-linux-gnu/GL"),
        QStringLiteral("/opt/amdgpu-pro/etc"),
    };
    QStringList files;
    for (const QString &dataDir : dataDirs) {
        const QDir icdDir(dataDir + QStringLiteral("/vulkan/icd.d"));
        const QStringList manifests = icdDir.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name);
        for (const QString &manifest : manifests) {
            if (manifest.contains(loader))
                files << icdDir.filePath(manifest);
        }
    }
    return files.join(QLatin1Char(':'));
}

// Maps "vendor:device" to the Vulkan device UUID. Empty where vulkaninfo isn't installed.
QHash<QString, QString> GpuManager::vulkanDeviceUuids()
{
    QHash<QString, QString> uuids;
    const QString vulkaninfo = QStandardPaths::findExecutable(QStringLiteral("vulkaninfo"));
    if (vulkaninfo.isEmpty())
        return uuids;

    // Overlay layers can crash vulkaninfo on exit, so they are off and the exit status is ignored.
    QProcess proc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("VK_LOADER_LAYERS_DISABLE"), QStringLiteral("~implicit~"));
    proc.setProcessEnvironment(env);
    proc.start(vulkaninfo, {QStringLiteral("--summary")});
    if (!proc.waitForFinished(5000)) {
        proc.kill();
        return uuids;
    }

    QString vendor;
    QString device;
    const QStringList lines = QString::fromUtf8(proc.readAllStandardOutput()).split(QLatin1Char('\n'));
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        const int eq = line.indexOf(QStringLiteral("= "));
        if (line.startsWith(QStringLiteral("GPU"))) {
            vendor.clear();
            device.clear();
        } else if (eq > 0) {
            const QString key = line.left(eq).trimmed();
            const QString value = line.mid(eq + 2).trimmed();
            if (key == QStringLiteral("vendorID"))
                vendor = value.mid(value.startsWith(QStringLiteral("0x")) ? 2 : 0).toLower();
            else if (key == QStringLiteral("deviceID"))
                device = value.mid(value.startsWith(QStringLiteral("0x")) ? 2 : 0).toLower();
            else if (key == QStringLiteral("deviceUUID") && !vendor.isEmpty() && !device.isEmpty()) {
                const QString id = vendor + QLatin1Char(':') + device;
                if (!uuids.contains(id))
                    uuids.insert(id, QString(value).remove(QLatin1Char('-')));
            }
        }
    }
    return uuids;
}

QList<QPair<QString, QString>> GpuManager::environmentFor(const QString &id) const
{
    const Gpu *chosen = nullptr;
    for (const Gpu &gpu : m_gpus) {
        if (gpu.id == id)
            chosen = &gpu;
    }
    if (!chosen || !available())
        return {};

    QList<QPair<QString, QString>> env;
    if (chosen->driver == QStringLiteral("nvidia")) {
        env.append({QStringLiteral("DRI_PRIME"), QStringLiteral("1")});
        env.append({QStringLiteral("__NV_PRIME_RENDER_OFFLOAD"), QStringLiteral("1")});
        env.append({QStringLiteral("__GLX_VENDOR_LIBRARY_NAME"), QStringLiteral("nvidia")});
        env.append({QStringLiteral("__VK_LAYER_NV_optimus"), QStringLiteral("NVIDIA_only")});
    } else if (!chosen->pciId.isEmpty()) {
        env.append({QStringLiteral("DRI_PRIME"), chosen->pciId});
    }
    // An empty value would hide every Vulkan driver.
    if (!chosen->icdFiles.isEmpty()) {
        env.append({QStringLiteral("VK_ICD_FILENAMES"), chosen->icdFiles});
        env.append({QStringLiteral("VK_DRIVER_FILES"), chosen->icdFiles});
    }
    if (!chosen->deviceUuid.isEmpty())
        env.append({QStringLiteral("DXVK_FILTER_DEVICE_UUID"), chosen->deviceUuid});
    return env;
}
