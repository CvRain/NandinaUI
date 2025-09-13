#pragma once
#include <QtQml/qqml.h>
#include "Theme.hpp"

class CatppuccinMocha : public Theme {
    Q_OBJECT
    QML_ELEMENT
public:
    explicit CatppuccinMocha(QObject *parent = nullptr);

private:
    static Palette createPalette();
};
