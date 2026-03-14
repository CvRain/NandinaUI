//
// Created by cvrain on 2026/3/2.
//

#ifndef NANDINA_COLOR_FACTORY_HPP
#define NANDINA_COLOR_FACTORY_HPP

#include <QColor>
#include <array>

#include "color_schema.hpp"
#include "theme_type.hpp"

namespace Nandina::Core::Color {

    using ThemeShadeArray = std::array<QColor, AccentCount>;

    // Complete color data for one theme (7 variants × 11 shades)
    struct ThemeColorData {
        ThemeShadeArray primary;
        ThemeShadeArray secondary;
        ThemeShadeArray tertiary;
        ThemeShadeArray success;
        ThemeShadeArray warning;
        ThemeShadeArray error;
        ThemeShadeArray surface;

        // Array-style access by variant index
        [[nodiscard]] const ThemeShadeArray &operator[](int variantIndex) const noexcept {
            switch (variantIndex) {
                case 0:
                    return primary;
                case 1:
                    return secondary;
                case 2:
                    return tertiary;
                case 3:
                    return success;
                case 4:
                    return warning;
                case 5:
                    return error;
                case 6:
                    return surface;
                default:
                    return primary;
            }
        }
    };

    class ColorFactory {
    public:
        /// Apply a theme's color data to an existing ColorSchema (in-place, no allocation).
        /// For dark mode the shade palette is reversed (50↔950, 100↔900, …).
        static void applyTheme(Types::ThemeVariant::ThemeTypes theme, bool isDark, ColorSchema *schema);

    private:
        static const ThemeColorData &getThemeData(Types::ThemeVariant::ThemeTypes theme);
        static void applyVariant(const ThemeShadeArray &colors, bool isDark, ColorPalette *palette);
    };

} // namespace Nandina::Core::Color


#endif // NANDINA_COLOR_FACTORY_HPP
