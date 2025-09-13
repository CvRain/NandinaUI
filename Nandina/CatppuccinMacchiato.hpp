#pragma once
#include <QtQml/qqml.h>
#include "Theme.hpp"

class CatppuccinMacchiato : public Theme {
    Q_OBJECT
    QML_ELEMENT

public:
    explicit CatppuccinMacchiato(QObject *parent = nullptr);

private:
    static Palette createPalette();
};
