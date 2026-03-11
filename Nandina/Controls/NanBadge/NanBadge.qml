pragma ComponentBehavior: Bound
// NanBadge.qml
// Compact inline status/label tag — non-interactive by default.
//
// Mirrors the shadcn Badge concept: a small pill used to classify, tag, or
// display a short status string.  Unlike NanButton there is no press state or
// click signal; it is purely a display element.
//
// ── ColorVariant  (ThemeVariant.ColorVariantTypes — import Nandina.Types) ──
//   ThemeVariant.ColorVariantTypes.Primary | Secondary | Tertiary | Success | Warning | Error | Surface
//
// ── Preset  (ThemeVariant.PresetTypes — import Nandina.Types) ─────────────
//   ThemeVariant.PresetTypes.Filled   — solid shade-500 background, white/dark text  [default]
//   ThemeVariant.PresetTypes.Tonal    — shade-100/200 tinted background, shade-700 text
//   ThemeVariant.PresetTypes.Outlined — transparent bg, coloured border, shade-600 text
//   ThemeVariant.PresetTypes.Ghost    — very pale shade-50 bg, shade-500 text; minimal footprint
//
// ── Size  (ThemeVariant.SizeTypes — import Nandina.Types) ─────────────────
//   ThemeVariant.SizeTypes.Sm | Md | Lg
//
// ── Minimal usage ──────────────────────────────────────────────────────────
//   NanBadge { text: "New" }
//
// ── Variant + preset ───────────────────────────────────────────────────────
//   NanBadge { text: "Beta";   colorVariant: ThemeVariant.ColorVariantTypes.Secondary; preset: ThemeVariant.PresetTypes.Tonal }
//   NanBadge { text: "Error";  colorVariant: ThemeVariant.ColorVariantTypes.Error;     preset: ThemeVariant.PresetTypes.Outlined }
//   NanBadge { text: "Active"; colorVariant: ThemeVariant.ColorVariantTypes.Success }
//
// ── Dot indicator (no label) ───────────────────────────────────────────────
//   NanBadge { dot: true; colorVariant: ThemeVariant.ColorVariantTypes.Success }
//
// ── With icon ──────────────────────────────────────────────────────────────
//   NanBadge {
//       id: _badge
//       text: "Admin"
//       colorVariant: ThemeVariant.ColorVariantTypes.Primary
//       leftIcon: Component { Text { text: "★"; color: _badge.resolvedTextColor } }
//   }

import QtQuick
import Nandina.Theme
import Nandina.Types

Item {
    id: root

    readonly property var _colorVariantTypes: ThemeVariant.ColorVariantTypes || ({})
    readonly property var _presetTypes: ThemeVariant.PresetTypes || ({})
    readonly property var _sizeTypes: ThemeVariant.SizeTypes || ({})

    readonly property int _colorPrimary: _colorVariantTypes.Primary ?? 0
    readonly property int _colorSurface: _colorVariantTypes.Surface ?? 6
    readonly property int _presetFilled: _presetTypes.Filled ?? 0
    readonly property int _presetOutlined: _presetTypes.Outlined ?? 2
    readonly property int _sizeMd: _sizeTypes.Md ?? 1

    // ── API ────────────────────────────────────────────────────────

    /// Semantic colour family.
    /// Use ThemeVariant.ColorVariantTypes.Primary … ThemeVariant.ColorVariantTypes.Surface  (Nandina.Types).
    property int colorVariant: root._colorPrimary

    /// Visual fill style.
    /// Use ThemeVariant.PresetTypes.Filled … ThemeVariant.PresetTypes.Ghost  (Nandina.Types).
    property int preset: root._presetFilled

    /// Label text. Leave empty when using dot mode.
    property string text: ""

    /// When true, renders a small round indicator dot instead of a label.
    property bool dot: false

    /// Optional icon Component shown left of the label.
    /// Bind color to `resolvedTextColor` inside the component.
    property Component leftIcon: null

    /// Optional icon Component shown right of the label.
    property Component rightIcon: null

    /// Badge size.  Use ThemeVariant.SizeTypes.Sm / Md / Lg  (Nandina.Types).
    property int size: root._sizeMd

    // ── Read-only helpers (bind from icon components) ──────────────
    readonly property color resolvedTextColor: _textColor

    // ── Geometry ───────────────────────────────────────────────────
    implicitWidth: dot ? _dotSize : Math.max(_minWidth, _contentRow.implicitWidth + _hPad * 2)
    implicitHeight: dot ? _dotSize : _height

    // ── Accessibility ─────────────────────────────────────────────
    Accessible.role: dot ? Accessible.Graphic : Accessible.StaticText
    Accessible.name: dot ? (colorVariant + " indicator") : root.text

    // ── Private: reactive flags ───────────────────────────────────
    readonly property bool _isDark: ThemeManager.darkMode
    readonly property real _sp: ThemeManager.primitives.spacing

    // ── Private: size tokens (indexed by SizeTypes: sm=0, md=1, lg=2) ────────
    readonly property int _height: [20, 22, 28][size]
    readonly property int _dotSize: [8, 10, 14][size]
    readonly property int _minWidth: [32, 40, 48][size]
    readonly property real _hPad: [_sp * 2, _sp * 2.5, _sp * 3.5][size]
    readonly property real _iconGap: _sp * 1.5
    readonly property real _fontSize: [10, 11, 13][size]

    // ── Private: palette (indexed by ColorVariantTypes: primary=0 … surface=6) ─
    readonly property var _palette: [ThemeManager.colors.primary    // Primary   = 0
        , ThemeManager.colors.secondary  // Secondary = 1
        , ThemeManager.colors.tertiary   // Tertiary  = 2
        , ThemeManager.colors.success    // Success   = 3
        , ThemeManager.colors.warning    // Warning   = 4
        , ThemeManager.colors.error      // Error     = 5
        , ThemeManager.colors.surface     // Surface   = 6
    ][colorVariant] ?? ThemeManager.colors.surface

    // ── Private: shade helper — maps shade number → ColorPalette.shade(index)
    // Uses a static lookup object instead of a 12-case switch.
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

    function _shade(s) {
        if (!_palette)
            return "transparent";
        return _palette.shade(_shadeIdx[s] ?? 5);
    }

    // ── Private: colours (indexed by PresetTypes: filled=0, tonal=1, outlined=2, ghost=3) ─
    readonly property color _bgColor: [_shade(500), _isDark ? _shade(800) : _shade(100), "transparent", _isDark ? Qt.rgba(_shade(500).r, _shade(500).g, _shade(500).b, 0.12) : _shade(50)][preset] ?? _shade(500)

    readonly property color _borderColor: preset === root._presetOutlined ? _shade(_isDark ? 400 : 500) : "transparent"

    readonly property color _textColor: [colorVariant === root._colorSurface ? _shade(950) : "#ffffff" // filled  — surface gets dark text for contrast
        , _shade(_isDark ? 300 : 700)                                  // Tonal
        , _shade(_isDark ? 400 : 600)                                  // Outlined
        , _shade(_isDark ? 400 : 600)                                   // Ghost
    ][preset] ?? "#ffffff"

    // ── Background pill ───────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        radius: root.dot ? width / 2 : height / 2
        color: root._bgColor
        border.color: root._borderColor
        border.width: root.preset === root._presetOutlined ? ThemeManager.primitives.borderWidth : 0

        Behavior on color {
            ColorAnimation {
                duration: 80
            }
        }
        Behavior on border.color {
            ColorAnimation {
                duration: 80
            }
        }
    }

    // ── Dot mode ─────────────────────────────────────────────────
    // When dot: true we just show the coloured circle (background above fills it).
    // Both the Row and extra elements are hidden to keep implicitWidth/Height clean.

    // ── Content row (text + icons) ────────────────────────────────
    Row {
        id: _contentRow
        visible: !root.dot
        anchors.centerIn: parent
        spacing: root._iconGap

        Loader {
            anchors.verticalCenter: parent.verticalCenter
            active: root.leftIcon !== null && !root.dot
            visible: active
            sourceComponent: root.leftIcon
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.text !== "" && !root.dot
            text: root.text
            font.family: ThemeManager.primitives.baseFont.fontFamily
            font.weight: Font.Medium
            font.pixelSize: root._fontSize
            // Tighten letter-spacing for compact badge text
            font.letterSpacing: 0.3
            color: root._textColor
        }

        Loader {
            anchors.verticalCenter: parent.verticalCenter
            active: root.rightIcon !== null && !root.dot
            visible: active
            sourceComponent: root.rightIcon
        }
    }
}
