#pragma once

#include "CatppuccinTheme.hpp"

class CatppuccinLatte : public CatppuccinTheme {
    Q_OBJECT
    QML_ELEMENT

public:
    explicit CatppuccinLatte(QObject *parent = nullptr);

private:
    static CatppuccinPalette createPalette();
};
