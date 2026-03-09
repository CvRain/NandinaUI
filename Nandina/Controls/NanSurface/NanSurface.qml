// NanSurface.qml
// Theme-aware visual container. Maps design tokens from ThemeManager to
// Rectangle properties.  Compose with NanPressable to add interactivity.
//
// Shade numbers follow Tailwind/Skeleton convention:
//   50 (lightest) → 100 → 200 → … → 900 → 950 (darkest)
//
// Usage:
//   NanSurface {
//       width: 200; height: 100
//       colorVariant: "primary"
//       // Defaults auto-adapt to light/dark mode
//   }
//
//   NanSurface {
//       colorVariant: "error"
//       backgroundShade: 100   // tinted badge background
//       borderShade:     300
//   }

import QtQuick
import Nandina.Theme

Rectangle {
    id: root

    // ── Color variant ──────────────────────────────────────────────
    /// Semantic color family.
    /// One of: "surface" | "primary" | "secondary" | "tertiary" |
    ///         "success" | "warning" | "error"
    property string colorVariant: "surface"

    // ── Shade hints ────────────────────────────────────────────────
    /// Background shade level. -1 = auto (theme-appropriate default).
    /// Valid values: 50, 100, 200, 300, 400, 500, 600, 700, 800, 900, 950
    property int backgroundShade: -1

    /// Border shade level. -1 = auto.
    property int borderShade: -1

    // ── Border ─────────────────────────────────────────────────────
    property bool bordered: true

    // ── Radius ─────────────────────────────────────────────────────
    /// -1 = use ThemeManager.primitives.radiusBase
    property real cornerRadius: -1

    // ── Resolved colours (readonly, useful for child items) ────────
    readonly property var _palette: _resolvePalette(_variantIdx)
    readonly property int _resolvedBackgroundShade: backgroundShade >= 0 ? backgroundShade : (ThemeManager.darkMode ? 800 : 50)
    readonly property int _resolvedBorderShade: borderShade >= 0 ? borderShade : (ThemeManager.darkMode ? 700 : 200)
    readonly property color resolvedBackgroundColor: _shadeColor(_palette, _resolvedBackgroundShade)
    readonly property color resolvedBorderColor: _shadeColor(_palette, _resolvedBorderShade)

    // ── Rectangle bindings ─────────────────────────────────────────
    color: resolvedBackgroundColor
    border.color: bordered ? resolvedBorderColor : "transparent"
    border.width: bordered ? ThemeManager.primitives.borderWidth : 0
    radius: cornerRadius >= 0 ? cornerRadius : ThemeManager.primitives.radiusBase

    // ── Private helpers ────────────────────────────────────────────
    readonly property int _variantIdx: _resolveVariant(colorVariant)

    function _resolvePalette(index) {
        switch (index) {
        case 0:
            return ThemeManager.colors.primary;
        case 1:
            return ThemeManager.colors.secondary;
        case 2:
            return ThemeManager.colors.tertiary;
        case 3:
            return ThemeManager.colors.success;
        case 4:
            return ThemeManager.colors.warning;
        case 5:
            return ThemeManager.colors.error;
        default:
            return ThemeManager.colors.surface;
        }
    }

    function _shadeColor(palette, shade) {
        if (!palette)
            return "transparent";

        switch (shade) {
        case 50:
            return palette.shade50;
        case 100:
            return palette.shade100;
        case 200:
            return palette.shade200;
        case 300:
            return palette.shade300;
        case 400:
            return palette.shade400;
        case 500:
            return palette.shade500;
        case 600:
            return palette.shade600;
        case 700:
            return palette.shade700;
        case 800:
            return palette.shade800;
        case 900:
            return palette.shade900;
        case 950:
            return palette.shade950;
        default:
            return palette.shade500;
        }
    }

    function _resolveVariant(name) {
        switch (name) {
        case "primary":
            return 0;
        case "secondary":
            return 1;
        case "tertiary":
            return 2;
        case "success":
            return 3;
        case "warning":
            return 4;
        case "error":
            return 5;
        default:
            return 6;  // "surface"
        }
    }
}
