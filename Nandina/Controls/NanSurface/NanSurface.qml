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
    readonly property color resolvedBackgroundColor: ThemeManager.colors.color(_variantIdx, _resolveShadeIndex(backgroundShade >= 0 ? backgroundShade : (ThemeManager.darkMode ? 800 : 50)))
    readonly property color resolvedBorderColor: ThemeManager.colors.color(_variantIdx, _resolveShadeIndex(borderShade >= 0 ? borderShade : (ThemeManager.darkMode ? 700 : 200)))

    // ── Rectangle bindings ─────────────────────────────────────────
    color: resolvedBackgroundColor
    border.color: bordered ? resolvedBorderColor : "transparent"
    border.width: bordered ? ThemeManager.primitives.borderWidth : 0
    radius: cornerRadius >= 0 ? cornerRadius : ThemeManager.primitives.radiusBase

    // ── Private helpers ────────────────────────────────────────────
    readonly property int _variantIdx: _resolveVariant(colorVariant)

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

    function _resolveShadeIndex(shade) {
        switch (shade) {
        case 50:
            return 0;
        case 100:
            return 1;
        case 200:
            return 2;
        case 300:
            return 3;
        case 400:
            return 4;
        case 500:
            return 5;
        case 600:
            return 6;
        case 700:
            return 7;
        case 800:
            return 8;
        case 900:
            return 9;
        case 950:
            return 10;
        default:
            return 5;   // fallback → shade500
        }
    }
}
