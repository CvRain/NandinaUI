//
// Created by cvrain on 2026/3/2.
//

#include "primitive_factory.hpp"

#include <QString>
#include <string_view>
#include "color_utils.hpp"
#include "font_manager.hpp"

namespace Nandina::Core::Primitives {

    // ═══════════════════════════════════════════════════════════════
    //  Internal helper: per-theme primitive data struct
    //  Values are pre-converted from CSS rem/px to device-independent px.
    //  Conversion: 1rem = 16px (standard browser default).
    // ═══════════════════════════════════════════════════════════════

    // Font family is resolved at runtime from FontManager (not stored in
    // per-theme data) because family names depend on what QFontDatabase
    // actually registered — not on a hardcoded string.
    struct TypographyData {
        int      fontWeight; // QFont::Weight compatible
        bool     italic;
        qreal    letterSpacing;
        uint32_t fontColorRgba;     // 0xRRGGBBFF
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
    // ── Aurora ────────────────────────────────────────────
    // base    → surface-900 (0x221D42FF) / surface-100 (0xEDEAFFFF)
    // heading → primary-600  (0x7C3AEDFF) / primary-300  (0xC084FCFF)
    // anchor  → secondary-600 (0xDB2777FF) / secondary-300 (0xF9A8D4FF)
    static const ThemePrimitiveData s_aurora = {
            4.0,    // spacing
            1.067,  // text-scaling
            8.0,    // radius-base: rounder corners = more playful
            16.0,   // radius-container: pill-ish
            1.0,
            1.0,
            2.0,    // ring-width: thicker focus ring for visual pop
            0xF8F7FFFF, // body-background: surface-50 (ice lavender)
            0x0E0A1FFF, // body-background-dark: surface-950 (deep space purple)
            {400, false, 0.0,  0x221D42FF, 0xEDEAFFFF},
            {700, false, 0.3,  0x7C3AEDFF, 0xC084FCFF},
            {400, false, 0.0,  0xDB2777FF, 0xF9A8D4FF},
    };
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
            {400,
             false,
             0.0,
             0x606275FF,
             0xDDE0E7FF},
            {800,
             false,
             0.0, // bolder ≈ 800
             0x0F9299FF,
             0xF3A3DDFF},
            {400,
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
            {400, false, 0.0, 0x121212FF, 0xFCFCFCFF},
            {700,
             false,
             0.0, // bold = 700
             0x121212FF,
             0xFCFCFCFF},
            {400, false, 0.0, 0x0770EFFF, 0x57A1F9FF},
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
            {400, false, 0.0, 0x1E1E23FF, 0xF5F5F5FF},
            {700,
             false,
             0.4, // letter-spacing: 0.025em ≈ 0.4px at 16px
             0x1E1E23FF,
             0xF5F5F5FF},
            {400, false, 0.0, 0x3F93DFFF, 0x44A3F5FF},
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
            {400,
             false,
             0.0,
             0x0C0E17FF,
             0xE0E0E0FF},
            {400,
             false,
             0.0,
             0x0C0E17FF,
             0xE0E0E0FF},
            {400,
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
            {400,
             false,
             0.0,
             0x000000FF,
             0xFFFFFFFF},
            {400,
             false,
             0.0,
             0x000000FF,
             0xFEF2DDFF},
            {400,
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
            {400,
             false,
             0.0,
             0x1F2741FF,
             0xE4E5ECFF},
            {700,
             false,
             0.0,
             0x1F2741FF,
             0xE4E5ECFF},
            {400,
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
            case Types::ThemeVariant::ThemeTypes::Aurora:
                return s_aurora;
            case Types::ThemeVariant::ThemeTypes::Catppuccin:
                return s_catppuccin;
            case Types::ThemeVariant::ThemeTypes::Cerberus:
                return s_cerberus;
            case Types::ThemeVariant::ThemeTypes::Concord:
                return s_concord;
            case Types::ThemeVariant::ThemeTypes::Crimson:
                return s_crimson;
            case Types::ThemeVariant::ThemeTypes::Fennec:
                return s_fennec;
            case Types::ThemeVariant::ThemeTypes::Legacy:
                return s_legacy;
        }
        return s_aurora;
    }

    static void applyTypography(const TypographyData &data, TypographySchema *schema,
                                 const QString &fontFamily) {
        schema->setFontFamily(fontFamily);
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

        // Typography — font family resolved from FontManager at runtime
        const QString defaultFamily = Fonts::FontManager::resolvedDefaultFamily();
        applyTypography(d.baseFont,    primitives->baseFont(),    defaultFamily);
        applyTypography(d.headingFont, primitives->headingFont(), defaultFamily);
        applyTypography(d.anchorFont,  primitives->anchorFont(),  defaultFamily);
    }

} // namespace Nandina::Core::Primitives
