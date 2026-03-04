//
// Created by cvrain on 2026/3/2.
//

#include "primitive_factory.hpp"

#include <QString>
#include <string_view>
#include "color_utils.hpp"

namespace Nandina::Core::Primitives {

    // ═══════════════════════════════════════════════════════════════
    //  Internal helper: per-theme primitive data struct
    //  Values are pre-converted from CSS rem/px to device-independent px.
    //  Conversion: 1rem = 16px (standard browser default).
    // ═══════════════════════════════════════════════════════════════

    struct TypographyData {
        std::string_view fontFamily;
        int fontWeight; // QFont::Weight compatible
        bool italic;
        qreal letterSpacing;
        uint32_t fontColorRgba; // 0xRRGGBBFF
        uint32_t fontColorDarkRgba; // 0xRRGGBBFF
    };

    struct ThemePrimitiveData {
        qreal spacing;
        qreal textScaling;
        qreal radiusBase;
        qreal radiusContainer;
        qreal borderWidth;
        qreal divideWidth;
        qreal ringWidth;
        uint32_t bodyBackgroundColorRgba;
        uint32_t bodyBackgroundColorDarkRgba;
        TypographyData baseFont;
        TypographyData headingFont;
        TypographyData anchorFont;
    };

    // ── Helper: RGBA → QColor (delegates to Core::rgbaToQColor) ───
    using Core::rgbaToQColor;

    // ═══════════════════════════════════════════════════════════════
    //  Static theme primitive data (derived from CSS source files)
    // ═══════════════════════════════════════════════════════════════

    // ── Catppuccin ─────────────────────────────────────────────────
    // Font colors: base → surface-700 (0x606275FF) / surface-50 (0xDDE0E7FF)
    //              heading → tertiary-500 (0x0F9299FF) / secondary-200 (0xF3A3DDFF)
    //              anchor → secondary-600 (0xD067B3FF) / tertiary-400 (0x2CA2A5FF)
    static const ThemePrimitiveData s_catppuccin = {
            4.0, // spacing: 0.25rem
            1.067, // text-scaling
            6.0, // radius-base: 0.375rem
            12.0, // radius-container: 0.75rem
            1.0,
            1.0,
            1.0,
            0xDDE0E7FF, // body-background: surface-50
            0x1E1E2EFF, // body-background-dark: surface-950
            {"ui-rounded, Hiragino Maru Gothic ProN, Quicksand, Comfortaa, sans-serif",
             400,
             false,
             0.0,
             0x606275FF,
             0xDDE0E7FF},
            {"Seravek, Gill Sans Nova, Ubuntu, Calibri, DejaVu Sans, sans-serif",
             800,
             false,
             0.0, // bolder ≈ 800
             0x0F9299FF,
             0xF3A3DDFF},
            {"ui-rounded, Hiragino Maru Gothic ProN, Quicksand, Comfortaa, sans-serif",
             400,
             false,
             0.0,
             0xD067B3FF,
             0x2CA2A5FF},
    };

    // ── Cerberus ───────────────────────────────────────────────────
    // Font colors: base → surface-950 (0x121212FF) / surface-50 (0xFCFCFCFF)
    //              heading → inherit (same as base)
    //              anchor → primary-500 (0x0770EFFF) / primary-400 (0x57A1F9FF)
    static const ThemePrimitiveData s_cerberus = {
            4.0,
            1.067,
            4.0, // radius-base: 0.25rem
            4.0, // radius-container: 0.25rem
            1.0,
            1.0,
            1.0,
            0xFCFCFCFF, // surface-50
            0x121212FF, // surface-950
            {"system-ui", 400, false, 0.0, 0x121212FF, 0xFCFCFCFF},
            {"system-ui",
             700,
             false,
             0.0, // bold = 700
             0x121212FF,
             0xFCFCFCFF},
            {"system-ui", 400, false, 0.0, 0x0770EFFF, 0x57A1F9FF},
    };

    // ── Concord ────────────────────────────────────────────────────
    // Font colors: base → surface-950 (0x1E1E23FF) / surface-50 (0xF5F5F5FF)
    //              heading → inherit
    //              anchor → tertiary-600 (0x3F93DFFF) / tertiary-500 (0x44A3F5FF)
    static const ThemePrimitiveData s_concord = {
            4.0,
            1.067,
            6.0, // 0.375rem
            12.0, // 0.75rem
            1.0,
            1.0,
            1.0,
            0xFFFFFFFF, // oklch(1 0 0) = white
            0x2B2B30FF, // surface-900
            {"system-ui, sans-serif", 400, false, 0.0, 0x1E1E23FF, 0xF5F5F5FF},
            {"Seravek, Gill Sans Nova, Ubuntu, Calibri, DejaVu Sans, sans-serif",
             700,
             false,
             0.4, // letter-spacing: 0.025em ≈ 0.4px at 16px
             0x1E1E23FF,
             0xF5F5F5FF},
            {"system-ui, sans-serif", 400, false, 0.0, 0x3F93DFFF, 0x44A3F5FF},
    };

    // ── Crimson ────────────────────────────────────────────────────
    // Font colors: base → surface-950 (0x0C0E17FF) / surface-50 (0xE0E0E0FF)
    //              heading → inherit
    //              anchor → primary-500 (0xD21D3DFF) / primary-500 (same)
    static const ThemePrimitiveData s_crimson = {
            4.0,
            1.067,
            6.0,
            12.0,
            1.0,
            1.0,
            1.0,
            0xFFFFFFFF, // oklch(1 0 0) = white
            0x0C0E17FF, // surface-950
            {"Avenir, Montserrat, Corbel, URW Gothic, source-sans-pro, sans-serif",
             400,
             false,
             0.0,
             0x0C0E17FF,
             0xE0E0E0FF},
            {"Avenir, Montserrat, Corbel, URW Gothic, source-sans-pro, sans-serif",
             400,
             false,
             0.0,
             0x0C0E17FF,
             0xE0E0E0FF},
            {"Avenir, Montserrat, Corbel, URW Gothic, source-sans-pro, sans-serif",
             400,
             false,
             0.0,
             0xD21D3DFF,
             0xD21D3DFF},
    };

    // ── Fennec ─────────────────────────────────────────────────────
    // Font colors: base → oklch(0 0 0) = black / oklch(1 0 0) = white
    //              heading → black / secondary-50 (0xFEF2DDFF)
    //              anchor → primary-600 (0xDE4403FF) / primary-500 (0xF6530DFF)
    static const ThemePrimitiveData s_fennec = {
            4.0,
            1.067,
            6.0,
            12.0,
            1.0,
            1.0,
            1.0,
            0xB9C0BCFF, // surface-50
            0x212227FF, // surface-950
            {"Bahnschrift, DIN Alternate, Franklin Gothic Medium, sans-serif-condensed, sans-serif",
             400,
             false,
             0.0,
             0x000000FF,
             0xFFFFFFFF},
            {"Bahnschrift, DIN Alternate, Franklin Gothic Medium, sans-serif-condensed, sans-serif",
             400,
             false,
             0.0,
             0x000000FF,
             0xFEF2DDFF},
            {"Bahnschrift, DIN Alternate, Franklin Gothic Medium, sans-serif-condensed, sans-serif",
             400,
             false,
             0.0,
             0xDE4403FF,
             0xF6530DFF},
    };

    // ── Legacy ─────────────────────────────────────────────────────
    // Font colors: base → surface-950 (0x1F2741FF) / surface-50 (0xE4E5ECFF)
    //              heading → inherit
    //              anchor → primary-500 (0x11BA81FF) / primary-500 (same)
    static const ThemePrimitiveData s_legacy = {
            4.0,
            1.067,
            6.0,
            12.0,
            1.0,
            1.0,
            1.0,
            0xFFFFFFFF, // oklch(1 0 0) = white
            0x1F2741FF, // surface-950
            {"Inter, Roboto, Helvetica Neue, Arial Nova, Nimbus Sans, Arial, sans-serif",
             400,
             false,
             0.0,
             0x1F2741FF,
             0xE4E5ECFF},
            {"Inter, Roboto, Helvetica Neue, Arial Nova, Nimbus Sans, Arial, sans-serif",
             700,
             false,
             0.0,
             0x1F2741FF,
             0xE4E5ECFF},
            {"Inter, Roboto, Helvetica Neue, Arial Nova, Nimbus Sans, Arial, sans-serif",
             400,
             false,
             0.0,
             0x11BA81FF,
             0x11BA81FF},
    };

    // ═══════════════════════════════════════════════════════════════
    //  Internal: data selector
    // ═══════════════════════════════════════════════════════════════

    static const ThemePrimitiveData &getThemePrimitiveData(Types::ThemeVariant::ThemeTypes theme) {
        switch (theme) {
            case Types::ThemeVariant::ThemeTypes::catppuccin:
                return s_catppuccin;
            case Types::ThemeVariant::ThemeTypes::cerberus:
                return s_cerberus;
            case Types::ThemeVariant::ThemeTypes::concord:
                return s_concord;
            case Types::ThemeVariant::ThemeTypes::crimson:
                return s_crimson;
            case Types::ThemeVariant::ThemeTypes::fennec:
                return s_fennec;
            case Types::ThemeVariant::ThemeTypes::legacy:
                return s_legacy;
        }
        return s_cerberus;
    }

    static void applyTypography(const TypographyData &data, TypographySchema *schema) {
        schema->setFontFamily(QString::fromUtf8(data.fontFamily));
        schema->setFontWeight(data.fontWeight);
        schema->setItalic(data.italic);
        schema->setLetterSpacing(data.letterSpacing);
        schema->setFontColor(rgbaToQColor(data.fontColorRgba));
        schema->setFontColorDark(rgbaToQColor(data.fontColorDarkRgba));
    }

    // ═══════════════════════════════════════════════════════════════
    //  Public API
    // ═══════════════════════════════════════════════════════════════

    void PrimitiveFactory::applyTheme(const Types::ThemeVariant::ThemeTypes theme, PrimitiveSchema *primitives) {
        const auto &d = getThemePrimitiveData(theme);

        // Layout tokens
        primitives->setSpacing(d.spacing);
        primitives->setTextScaling(d.textScaling);

        // Radius tokens
        primitives->setRadiusBase(d.radiusBase);
        primitives->setRadiusContainer(d.radiusContainer);

        // Border tokens
        primitives->setBorderWidth(d.borderWidth);
        primitives->setDivideWidth(d.divideWidth);
        primitives->setRingWidth(d.ringWidth);

        // Background colors
        primitives->setBodyBackgroundColor(rgbaToQColor(d.bodyBackgroundColorRgba));
        primitives->setBodyBackgroundColorDark(rgbaToQColor(d.bodyBackgroundColorDarkRgba));

        // Typography
        applyTypography(d.baseFont, primitives->baseFont());
        applyTypography(d.headingFont, primitives->headingFont());
        applyTypography(d.anchorFont, primitives->anchorFont());
    }

} // namespace Nandina::Core::Primitives
