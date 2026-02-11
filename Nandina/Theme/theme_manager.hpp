//
// Created by cvrain on 2026/2/11.
//

#ifndef NANDINA_THEME_MANAGER_HPP
#define NANDINA_THEME_MANAGER_HPP

#include <QObject>
#include <QQmlEngine>

#include "color_atla.hpp"

namespace Nandina::NandinaTheme {
    Q_NAMESPACE
    QML_ELEMENT

    class ThemeManager : public QObject {
        Q_OBJECT
        QML_ELEMENT
    public:
        explicit ThemeManager(QObject *parent = nullptr);

    private:
        NandinaColor::ColorCollection &currentColorCollection;
        NandinaColor::PaletteType &currentPaletteCollection;
        NandinaColor::PaletteType currentPaletteType;
        NandinaColor::NanColorAtla colorAtla{};
    };

} // namespace Nandina::NandinaTheme

#endif // NANDINA_THEME_MANAGER_HPP
