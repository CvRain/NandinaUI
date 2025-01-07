#pragma once

#include "CatppuccinTheme.hpp"

#include <QQmlEngine>
#include <QString>

class CatppuccinFrappe : public CatppuccinTheme {
    Q_OBJECT
    QML_ELEMENT
public:
    explicit CatppuccinFrappe(QObject *parent = nullptr);

private:
    static CatppuccinPalette createPalette();
};
