//
// Created by cvrain on 2026/2/10.
//

#ifndef NANDINA_COLOR_ATLA_HPP
#define NANDINA_COLOR_ATLA_HPP

#include <QObject>
#include <QQmlEngine>

#include "color_schema.hpp"

namespace Nandina::NandinaColor {

    class NANDINA_COLOR_EXPORT NanColorAtla : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

    public:
        explicit NanColorAtla(QObject *parent = nullptr);

        QHash<PaletteType, ColorCollection *> colorCollections{};
        QHash<PaletteType, PaletteCollection *> paletteCollections{};
    };

} // namespace Nandina::NandinaColor

#endif // NANDINA_COLOR_ATLA_HPP
