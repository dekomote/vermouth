#pragma once

#include <QString>
#include <QStringList>

// Command line arguments that make DOSBox Staging start a game entry: a folder, a DOS program, a .conf file or a
// disc image, with any extra disc images mounted next to it.
QStringList dosboxArgs(const QString &exePath, const QStringList &images);
