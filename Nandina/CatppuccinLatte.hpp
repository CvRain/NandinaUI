#pragma once
#include <QtQml/qqml.h>
#include "Theme.hpp"

class CatppuccinLatte : public Theme {
    Q_OBJECT
    QML_ELEMENT

public:
    explicit CatppuccinLatte(QObject *parent = nullptr);

private:
    static Palette createPalette();
};
