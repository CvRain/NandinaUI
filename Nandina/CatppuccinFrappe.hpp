#pragma once
#include <QtQml/qqml.h>
#include "Theme.hpp"

#include <QQmlEngine>
#include <QString>

class CatppuccinFrappe : public Theme {
    Q_OBJECT
    QML_ELEMENT
public:
    explicit CatppuccinFrappe(QObject *parent = nullptr);

private:
    static Palette createPalette();
};
