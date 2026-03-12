// NanSurface.qml
// Theme-aware visual container. Maps design tokens from ThemeManager to
// Rectangle properties. Compose with NanPressable to add interactivity.
//
// Shade numbers follow Tailwind/Skeleton convention:
//   50 (lightest) → 100 → 200 → … → 900 → 950 (darkest)
//
// Usage:
//   NanSurface { width: 200; height: 100 }
//   NanSurface { backgroundShade: 100; borderShade: 300 }

import QtQuick
import Nandina.Theme
import Nandina.Types

Rectangle {
    id: root

    readonly property var _colorVariantTypes: ThemeVariant.ColorVariantTypes || ({})
    readonly property int _colorSurface: _colorVariantTypes.Surface ?? 6

    // ── Color variant ──────────────────────────────────────────────
    /// Semantic color family.
    /// Use the shared ColorVariantTypes enum exposed by Nandina.Types.
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
    readonly property var _shadeIdx: ({
            50: 0,
            100: 1,
            200: 2,
            300: 3,
            400: 4,
            500: 5,
            600: 6,
            700: 7,
            800: 8,
            900: 9,
            950: 10
        })
    readonly property var _paletteShades: _palette ? [_palette.shade50, _palette.shade100, _palette.shade200, _palette.shade300, _palette.shade400, _palette.shade500, _palette.shade600, _palette.shade700, _palette.shade800, _palette.shade900, _palette.shade950] : []
    readonly property color resolvedBackgroundColor: _paletteShades[_shadeIdx[_resolvedBackgroundShade] ?? 5] ?? "transparent"
    readonly property color resolvedBorderColor: _paletteShades[_shadeIdx[_resolvedBorderShade] ?? 5] ?? "transparent"

    // ── Rectangle bindings ─────────────────────────────────────────
    color: resolvedBackgroundColor
    border.color: bordered ? resolvedBorderColor : "transparent"
    border.width: bordered ? ThemeManager.primitives.borderWidth : 0
    radius: cornerRadius >= 0 ? cornerRadius : ThemeManager.primitives.radiusBase

    // ── Private helpers ────────────────────────────────────────────
    function _resolvePalette(index) {
        return [ThemeManager.colors.primary, ThemeManager.colors.secondary, ThemeManager.colors.tertiary, ThemeManager.colors.success, ThemeManager.colors.warning, ThemeManager.colors.error, ThemeManager.colors.surface][index] ?? ThemeManager.colors.surface;
    }
}
