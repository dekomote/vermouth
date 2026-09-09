#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class ColorSchemeSwitcher : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList schemeNames READ schemeNames CONSTANT)

public:
    explicit ColorSchemeSwitcher(QObject *parent = nullptr);

    QStringList schemeNames() const;

    Q_INVOKABLE QString schemeIdAt(int index) const;
    Q_INVOKABLE int indexForSchemeId(const QString &schemeId) const;
    Q_INVOKABLE void applySchemeId(const QString &schemeId);
};
