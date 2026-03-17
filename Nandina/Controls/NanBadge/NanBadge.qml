pragma ComponentBehavior: Bound
// NanBadge.qml
// Compact inline status/label tag — non-interactive by default.
//
// Mirrors the shadcn Badge concept: a small pill used to classify, tag, or
// display a short status string.  Unlike NanButton there is no press state or
// click signal; it is purely a display element.
//
// ── ColorVariant ─────────────────────────────────────────────────────────
//   Primary | Secondary | Tertiary | Success | Warning | Error | Surface
//
// ── Preset ───────────────────────────────────────────────────────────────
//   Filled   — solid shade-500 background, white/dark text  [default]
//   Tonal    — shade-100/200 tinted background, shade-700 text
//   Outlined — transparent bg, coloured border, shade-600 text
//   Ghost    — very pale shade-50 bg, shade-500 text; minimal footprint
//
// ── Size ─────────────────────────────────────────────────────────────────
//   Sm | Md | Lg
//
// ── Minimal usage ──────────────────────────────────────────────────────────
//   NanBadge { text: "New" }
//
// ── Variant + preset ───────────────────────────────────────────────────────
//   NanBadge { text: "Beta" }
//   NanBadge { text: "Error" }
//   NanBadge { text: "Active" }
//
// ── Dot indicator (no label) ───────────────────────────────────────────────
//   NanBadge { dot: true }
//
// ── With icon ──────────────────────────────────────────────────────────────
//   NanBadge {
//       id: _badge
//       text: "Admin"
//       leftIcon: Component { Text { text: "★"; color: _badge.resolvedTextColor } }
//   }

import QtQuick
import Nandina.Theme
import Nandina.Tokens

Item {
    id: root

    readonly property int _colorPrimary: NanTokens.colorPrimary
    readonly property int _colorSurface: NanTokens.colorSurface
    readonly property int _presetFilled: NanTokens.presetFilled
    readonly property int _presetOutlined: NanTokens.presetOutlined
    readonly property int _sizeMd: NanTokens.sizeMd

    // ── API ────────────────────────────────────────────────────────

    /// Semantic colour family.
    /// Use the shared ColorVariantTypes enum exposed by Nandina.Types.
    property int colorVariant: root._colorPrimary

    /// Visual fill style.
    /// Use the shared PresetTypes enum exposed by Nandina.Types.
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

    /// Badge size.  Use the shared SizeTypes enum exposed by Nandina.Types.
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

    // ── Private: palette (via ColorSchema.palette) ─────────────────
    readonly property var _palette: ThemeManager.colors.palette(colorVariant)

    function _shade(s) {
        if (!_palette)
            return "transparent";
        // Dummy-read a named shade property so QML tracks _palette.changed.
        const _ = _palette.shade500;
        return _palette.shade(NanTokens.shadeIndex(s));
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
