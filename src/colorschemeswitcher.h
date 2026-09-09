#pragma once

#include <QObject>
#include <QString>

class ColorSchemeSwitcher : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentSchemeName READ currentSchemeName NOTIFY schemeChanged)

public:
    explicit ColorSchemeSwitcher(QObject *parent = nullptr);

    QString currentSchemeName() const;

    // POC only: cycles Default -> Breeze Light -> Breeze Dark -> Default ...
    Q_INVOKABLE void cycleScheme();

Q_SIGNALS:
    void schemeChanged();

private:
    void applySchemeAt(int index);

    int m_index = 0;
};
