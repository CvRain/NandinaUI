#pragma once
#include "CatppuccinTheme.hpp"

class CatppuccinMacchiato : public CatppuccinTheme {
    Q_OBJECT
    QML_ELEMENT

public:
    explicit CatppuccinMacchiato(QObject *parent = nullptr);

private:
    static CatppuccinPalette createPalette();
};
