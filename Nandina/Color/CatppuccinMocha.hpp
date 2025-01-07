#pragma once

#include "CatppuccinTheme.hpp"

class CatppuccinMocha : public CatppuccinTheme {
    Q_OBJECT
    QML_ELEMENT
public:
    explicit CatppuccinMocha(QObject *parent = nullptr);

private:
    static CatppuccinPalette createPalette();
};
