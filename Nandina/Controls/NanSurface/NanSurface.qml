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
import Nandina.Tokens

Rectangle {
    id: root

    // ── Color variant ──────────────────────────────────────────────
    /// Semantic color family.
    /// Use NanTokens.colorPrimary … NanTokens.colorSurface constants.
    property int colorVariant: NanTokens.colorSurface

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
    readonly property var _palette: ThemeManager.colors.palette(colorVariant)
    // Dark mode: palette is reversed (shade50=darkest, shade950=lightest).
    // Use low shade numbers for dark containers so they appear near-dark,
    // and moderate numbers for borders to remain visible but not dazzling.
    readonly property int _resolvedBackgroundShade: backgroundShade >= 0 ? backgroundShade : (ThemeManager.darkMode ? 100 : 50)
    readonly property int _resolvedBorderShade: borderShade >= 0 ? borderShade : (ThemeManager.darkMode ? 300 : 200)
    readonly property color resolvedBackgroundColor: {
        // Dummy-read a named shade property so QML tracks _palette.changed.
        // Q_INVOKABLE shade() calls are not auto-tracked by the binding engine.
        const _ = _palette ? _palette.shade500 : null;
        return _palette ? _palette.shade(NanTokens.shadeIndex(_resolvedBackgroundShade)) : "transparent";
    }
    readonly property color resolvedBorderColor: {
        const _ = _palette ? _palette.shade500 : null;
        return _palette ? _palette.shade(NanTokens.shadeIndex(_resolvedBorderShade)) : "transparent";
    }

    // ── Rectangle bindings ─────────────────────────────────────────
    color: resolvedBackgroundColor
    border.color: bordered ? resolvedBorderColor : "transparent"
    border.width: bordered ? ThemeManager.primitives.borderWidth : 0
    radius: cornerRadius >= 0 ? cornerRadius : ThemeManager.primitives.radiusBase

    Behavior on color {
        ColorAnimation {
            duration: 150
            easing.type: Easing.InOutQuad
        }
    }
}
