//
// Created by cvrain on 2026/3/2.
//

#include "primitive_factory.hpp"

#include <QColor>
#include <QString>

namespace Nandina::Core::Primitives {

    // ═══════════════════════════════════════════════════════════════
    //  Internal helper: per-theme primitive data struct
    //  Values are pre-converted from CSS rem/px to device-independent px.
    //  Conversion: 1rem = 16px (standard browser default).
    // ═══════════════════════════════════════════════════════════════

    struct TypographyData {
        const char *fontFamily;
        int         fontWeight;     // QFont::Weight compatible
        bool        italic;
        qreal       letterSpacing;
        uint32_t    fontColorRgba;      // 0xRRGGBBFF
        uint32_t    fontColorDarkRgba;  // 0xRRGGBBFF
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

    // ── Helper: RGBA → QColor ──────────────────────────────────────
    static inline QColor rgbaToQColor(uint32_t rgba) {
        return QColor::fromRgba(
            ((rgba >> 8) & 0x00FFFFFFu) | ((rgba & 0xFFu) << 24)
        );
    }

    // ═══════════════════════════════════════════════════════════════
    //  Static theme primitive data (derived from CSS source files)
    // ═══════════════════════════════════════════════════════════════

    // ── Catppuccin ─────────────────────────────────────────────────
    // Font colors: base → surface-700 (0x606275FF) / surface-50 (0xDDE0E7FF)
    //              heading → tertiary-500 (0x0F9299FF) / secondary-200 (0xF3A3DDFF)
    //              anchor → secondary-600 (0xD067B3FF) / tertiary-400 (0x2CA2A5FF)
    static const ThemePrimitiveData s_catppuccin = {
        4.0,        // spacing: 0.25rem
        1.067,      // text-scaling
        6.0,        // radius-base: 0.375rem
        12.0,       // radius-container: 0.75rem
        1.0, 1.0, 1.0,
        0xDDE0E7FF, // body-background: surface-50
        0x1E1E2EFF, // body-background-dark: surface-950
        { "ui-rounded, Hiragino Maru Gothic ProN, Quicksand, Comfortaa, sans-serif",
          400, false, 0.0,
          0x606275FF, 0xDDE0E7FF },
        { "Seravek, Gill Sans Nova, Ubuntu, Calibri, DejaVu Sans, sans-serif",
          800, false, 0.0,    // bolder ≈ 800
          0x0F9299FF, 0xF3A3DDFF },
        { "ui-rounded, Hiragino Maru Gothic ProN, Quicksand, Comfortaa, sans-serif",
          400, false, 0.0,
          0xD067B3FF, 0x2CA2A5FF },
    };

    // ── Cerberus ───────────────────────────────────────────────────
    // Font colors: base → surface-950 (0x121212FF) / surface-50 (0xFCFCFCFF)
    //              heading → inherit (same as base)
    //              anchor → primary-500 (0x0770EFFF) / primary-400 (0x57A1F9FF)
    static const ThemePrimitiveData s_cerberus = {
        4.0, 1.067,
        4.0,        // radius-base: 0.25rem
        4.0,        // radius-container: 0.25rem
        1.0, 1.0, 1.0,
        0xFCFCFCFF, 