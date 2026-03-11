// NanSurface.qml
// Theme-aware visual container. Maps design tokens from ThemeManager to
// Rectangle properties. Compose with NanPressable to add interactivity.
//
// Shade numbers follow Tailwind/Skeleton convention:
//   50 (lightest) → 100 → 200 → … → 900 → 950 (darkest)
//
// Usage:
//   NanSurface {
//       width: 200; height: 100
//       colorVariant: ThemeVariant.ColorVariantTypes.Primary
//       // Defaults auto-adapt to light/dark mode
//   }
//
//   NanSurface {
//       colorVariant: ThemeVariant.ColorVariantTypes.Error
//       backgroundShade: 100
//       borderShade: 300
//   }

import QtQuick
import Nandina.Theme
import Nandina.Types

Rectangle {
    id: root

    readonly property var _colorVariantTypes: ThemeVariant.ColorVariantTypes || ({})
    readonly property int _colorSurface: _colorVariantTypes.Surface ?? 6

    // ── Color variant ──────────────────────────────────────────────
    /// Semantic color family.
    /// Use ThemeVariant.ColorVariantTypes.Surface … ThemeVariant.ColorVariantTypes.Error.
    property int colorVariant: root._colorSurface

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
    readonly property var _palette: _resolvePalette(colorVariant)
    // Dark mode: palette is reversed (shade50=darkest, shade950=lightest).
    // Use low shade numbers for dark containers so they appear near-dark,
    // and moderate numbers for borders to remain visible but not dazzling.
    readonly property int _resolvedBackgroundShade: backgroundShade >= 0 ? backgroundShade : (ThemeManager.darkMode ? 100 : 50)
    readonly property int _resolvedBorderShade: borderShade >= 0 ? borderShade : (ThemeManager.darkMode ? 300 : 200)
    readonly property color resolvedBackgroundColor: _shadeColor(_palette, _resolvedBackgroundShade)
    readonly property color resolvedBorderColor: _shadeColor(_palette, _resolvedBorderShade)

    // ── Rectangle bindings ─────────────────────────────────────────
    color: resolvedBackgroundColor
    border.color: bordered ? resolvedBorderColor : "transparent"
    border.width: bordered ? ThemeManager.primitives.borderWidth : 0
    radius: cornerRadius >= 0 ? cornerRadius : ThemeManager.primitives.radiusBase

    // ── Private helpers ────────────────────────────────────────────
    function _resolvePalette(index) {
        return [ThemeManager.colors.primary, ThemeManager.colors.secondary, ThemeManager.colors.tertiary, ThemeManager.colors.success, ThemeManager.colors.warning, ThemeManager.colors.error, ThemeManager.colors.surface][index] ?? ThemeManager.colors.surface;
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
}
