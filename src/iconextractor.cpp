#include "iconextractor.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QTemporaryFile>

IconExtractor::IconExtractor(QObject *parent)
    : QObject(parent)
{}

QString IconExtractor::cacheDir() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/icons");
    QDir().mkpath(dir);
    return dir;
}

QString IconExtractor::extractIcon(const QString &exePath) {
    if (!QFileInfo::exists(exePath))
        return {};

    QByteArray hash = QCryptographicHash::hash(exePath.toUtf8(), QCryptographicHash::Md5).toHex();
    QString outPath = cacheDir() + QStringLiteral("/") + QString::fromLatin1(hash) + QStringLiteral(".png");

    if (QFileInfo::exists(outPath))
        return outPath;

    QString result = tryWrestool(exePath, outPath);
    if (!result.isEmpty())
        return result;

    return {};
}

QString IconExtractor::tryWrestool(const QString &exePath, const QString &outPath) {
    QString wrestool = QStandardPaths::findExecutable(QStringLiteral("wrestool"));
    QString icotool = QStandardPaths::findExecutable(QStringLiteral("icotool"));
    if (wrestool.isEmpty() || icotool.isEmpty())
        return {};

    QTemporaryFile icoFile;
    icoFile.setFileTemplate(QDir::tempPath() + QStringLiteral("/vermouth_XXXXXX.ico"));
    if (!icoFile.open())
        return {};
    icoFile.close();

    QProcess wrestoolProc;
    wrestoolProc.start(wrestool, {QStringLiteral("--extract"), QStringLiteral("--type=14"), QStringLiteral("--output=") + icoFile.fileName(), exePath});
    if (!wrestoolProc.waitForFinished(10000) || wrestoolProc.exitCode() != 0)
        return {};

    if (QFileInfo(icoFile.fileName()).size() == 0)
        return {};

    QProcess icotoolProc;
    icotoolProc.start(icotool, {QStringLiteral("--list"), icoFile.fileName()});
    if (!icotoolProc.waitForFinished(5000))
        return {};

    QString listOutput = QString::fromUtf8(icotoolProc.readAllStandardOutput());
    int bestSize = 0;
    int bestIndex = 1;
    int currentIndex = 0;
    for (const auto &line : listOutput.split(QLatin1Char('\n'))) {
        currentIndex++;
        QRegularExpression rx(QStringLiteral("--width=(\\d+)"));
        auto match = rx.match(line);
        if (match.hasMatch()) {
            int w = match.captured(1).toInt();
            if (w > bestSize) {
                bestSize = w;
                QRegularExpression idxRx(QStringLiteral("--index=(\\d+)"));
                auto idxMatch = idxRx.match(line);
                if (idxMatch.hasMatch())
                    bestIndex = idxMatch.captured(1).toInt();
            }
        }
    }

    QProcess convertProc;
    convertProc.start(icotool, {QStringLiteral("--extract"), QStringLiteral("--index=") + QString::number(bestIndex),
                                QStringLiteral("--output=") + outPath, icoFile.fileName()});
    if (!convertProc.waitForFinished(5000) || convertProc.exitCode() != 0) {
        QProcess fallback;
        fallback.start(icotool, {QStringLiteral("--extract"), QStringLiteral("--output=") + outPath, icoFile.fileName()});
        fallback.waitForFinished(5000);
    }

    if (QFileInfo::exists(outPath) && QFileInfo(outPath).size() > 0)
        return outPath;
    return {};
}
