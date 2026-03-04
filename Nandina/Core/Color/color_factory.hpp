//
// Created by cvrain on 2026/3/2.
//

#ifndef NANDINA_COLOR_FACTORY_HPP
#define NANDINA_COLOR_FACTORY_HPP

#include <array>

#include "color_schema.hpp"
#include "color_utils.hpp"
#include "theme_type.hpp"

namespace Nandina::Core::Color {

    // 11 accent hex values per variant (RGBA format: 0xRRGGBBFF)
    using AccentHexArray = std::array<uint32_t, AccentCount>;

    // Complete color data for one theme (7 variants × 11 shades)
    struct ThemeColorData {
        AccentHexArray primary;
        AccentHexArray secondary;
        AccentHexArray tertiary;
        AccentHexArray success;
        AccentHexArray warning;
        AccentHexArray error;
        AccentHexArray surface;

        // Array-style access by variant index
        [[nodiscard]] const AccentHexArray &operator[](int variantIndex) const noexcept {
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
        static void applyVariant(const AccentHexArray &hexColors, bool isDark, ColorPalette *palette);
    };

} // namespace Nandina::Core::Color


#endif // NANDINA_COLOR_FACTORY_HPP
