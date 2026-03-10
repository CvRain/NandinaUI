// NanButton.qml
// Theme-aware clickable button built on NanPressable.
//
// Preset semantics (mirrors NanCard preset philosophy):
//   "filled"   — solid shade-500 fill, white text, high emphasis   [default]
//   "tonal"    — lightly tinted bg (shade 100 / 200), variant-coloured text
//   "outlined" — transparent bg, coloured border, no fill
//   "ghost"    — no bg, no border, hover-only tint; minimal visual footprint
//   "link"     — text-only with underline, for inline / semantic anchors
//
// ── Minimal usage ──────────────────────────────────────────────────────────
//   NanButton { text: "Save" }
//
// ── Preset + variant ───────────────────────────────────────────────────────
//   NanButton { text: "Delete"; preset: "outlined"; colorVariant: "error" }
//   NanButton { text: "Subscribe"; preset: "tonal";    colorVariant: "success" }
//
// ── Sizes ──────────────────────────────────────────────────────────────────
//   NanButton { text: "Compact"; size: "sm" }
//   NanButton { text: "Submit";  size: "lg" }
//
// ── With left icon ─────────────────────────────────────────────────────────
//   NanButton {
//       text: "Upload"
//       colorVariant: "primary"
//       leftIcon: Component {
//           Text {
//               text: "↑"
//               color: _btn.resolvedTextColor   // bind from outside or use "white"
//               font.pixelSize: 14
//           }
//       }
//       id: _btn
//   }
//
// ── Disabled ───────────────────────────────────────────────────────────────
//   NanButton { text: "Locked"; enabled: false }

import QtQuick
import Nandina.Theme
import Nandina.Controls

Item {
    id: root

    // ── Geometry ───────────────────────────────────────────────────
    implicitWidth: Math.max(_minWidth, _contentRow.implicitWidth + _hPadding * 2)
    implicitHeight: _height

    // ── Color ──────────────────────────────────────────────────────
    /// Semantic colour family.
    /// "surface" | "primary" | "secondary" | "tertiary" |
    /// "success" | "warning" | "error"
    property string colorVariant: "primary"

    // ── Preset ─────────────────────────────────────────────────────
    /// Visual fill style.
    /// "filled" | "tonal" | "outlined" | "ghost" | "link"
    property string preset: "filled"

    // ── Label ──────────────────────────────────────────────────────
    property string text: ""

    // ── Icons ──────────────────────────────────────────────────────
    /// Optional icon Component placed left of the label.
    /// The loaded item can reference resolvedTextColor for colour binding.
    property Component leftIcon: null

    /// Optional icon Component placed right of the label.
    property Component rightIcon: null

    // ── Size ───────────────────────────────────────────────────────
    /// "sm" | "md" | "lg"
    property string size: "md"

    // ── State ──────────────────────────────────────────────────────
    property bool enabled: true

    // ── Interaction (read-only) ────────────────────────────────────
    readonly property bool hovered: _pressable.hovered
    readonly property bool pressed: _pressable.pressed || _keyPressed

    // ── Resolved colour (read-only — bind from icon Components) ────
    readonly property color resolvedTextColor: _textColor

    // ── Signals ────────────────────────────────────────────────────
    signal clicked
    signal released
    signal pressStarted
    signal canceled

    // ── Accessibility ─────────────────────────────────────────────
    activeFocusOnTab: root.enabled
    Accessible.role: Accessible.Button
    Accessible.name: root.text
    Accessible.focusable: root.enabled

    // ── Transform ─────────────────────────────────────────────────
    transformOrigin: Item.Center
    scale: _currentScale
    opacity: root.enabled ? 1.0 : 0.45

    // ── Private: size tokens ───────────────────────────────────────
    readonly property bool _isDark: ThemeManager.darkMode
    readonly property real _sp: ThemeManager.primitives.spacing

    readonly property int _height: size === "sm" ? 30 : (size === "lg" ? 42 : 36)
    readonly property int _minWidth: size === "sm" ? 64 : (size === "lg" ? 96 : 80)
    readonly property real _hPadding: size === "sm" ? _sp * 3 : (size === "lg" ? _sp * 5 : _sp * 4)
    readonly property real _iconSpacing: _sp * 2
    readonly property real _fontSize: size === "sm" ? Math.round(12 * ThemeManager.primitives.textScaling) : (size === "lg" ? Math.round(15 * ThemeManager.primitives.textScaling) : Math.round(13 * ThemeManager.primitives.textScaling))

    // ── Private: palette ──────────────────────────────────────────
    readonly property var _palette: {
        switch (colorVariant) {
        case "primary":
            return ThemeManager.colors.primary;
        case "secondary":
            return ThemeManager.colors.secondary;
        case "tertiary":
            return ThemeManager.colors.tertiary;
        case "success":
            return ThemeManager.colors.success;
        case "warning":
            return ThemeManager.colors.warning;
        case "error":
            return ThemeManager.colors.error;
        default:
            return ThemeManager.colors.surface;
        }
    }

    // ── Private: shade helpers ─────────────────────────────────────
    function _shade(s) {
        if (!_palette)
            return "transparent";
        switch (s) {
        case 50:
            return _palette.shade50;
        case 100:
            return _palette.shade100;
        case 200:
            return _palette.shade200;
        case 300:
            return _palette.shade300;
        case 400:
            return _palette.shade400;
        case 500:
            return _palette.shade500;
        case 600:
            return _palette.shade600;
        case 700:
            return _palette.shade700;
        case 800:
            return _palette.shade800;
        case 900:
            return _palette.shade900;
        case 950:
            return _palette.shade950;
        default:
            return _palette.shade500;
        }
    }

    function _shadeA(s, alpha) {
        const c = _shade(s);
        return Qt.rgba(c.r, c.g, c.b, alpha);
    }

    // ── Private: background ────────────────────────────────────────
    // Dark mode: palette is reversed — shade50=darkest, shade950=lightest.
    // shade500 stays neutral (middle index, same hue in both modes).
    //
    // For "filled" hover/press we invert direction by mode:
    //   light  — hover=shade400 (lighter)  / press=shade600 (darker)
    //   dark   — hover=shade600 (=orig400, lighter) / press=shade400 (=orig600, darker)
    readonly property color _bgColor: {
        const isPr = _pressable.pressed || _keyPressed;
        const isHv = _pressable.hovered && !isPr;
        switch (preset) {
        case "filled":
            if (isPr)
                return _shade(_isDark ? 400 : 600);
            if (isHv)
                return _shade(_isDark ? 600 : 400);
            return _shade(500);
        case "tonal":
            if (isPr)
                return _shade(_isDark ? 400 : 300);
            if (isHv)
                return _shade(_isDark ? 300 : 200);
            return _shade(_isDark ? 200 : 100);
        case "outlined":
        case "ghost":
        case "link":
            if (isPr)
                return _shadeA(500, 0.16);
            if (isHv)
                return _shadeA(500, 0.08);
            return "transparent";
        default:
            return "transparent";
        }
    }

    // ── Private: border ────────────────────────────────────────────
    readonly property bool _hasBorder: preset === "outlined"
    readonly property color _borderColor: {
        if (preset !== "outlined")
            return "transparent";
        if (_pressable.hovered || _pressable.pressed || _keyPressed)
            return _shade(_isDark ? 600 : 600);
        return _shade(500);
    }

    // ── Private: text colour ───────────────────────────────────────
    readonly property color _textColor: {
        switch (preset) {
        case "filled":
            return "#ffffff";
        case "tonal":
            // dark shade700 = original shade300 = bright; light shade700 = dark
            return _shade(700);
        case "outlined":
        case "ghost":
        case "link":
            return _shade(_isDark ? 700 : 600);
        default:
            return _shade(600);
        }
    }

    // ── Private: scale / bounce animation ─────────────────────────
    property real _currentScale: 1.0
    property bool _bouncing: false

    readonly property real _targetScale: {
        if (!root.enabled)
            return 1.0;
        if (_pressable.pressed || _keyPressed)
            return 0.96;
        if (_pressable.hovered)
            return 1.02;
        return 1.0;
    }

    // Drive _currentScale from _targetScale whenever we're not mid-bounce.
    // Binding instead of onXChanged avoids the private-prop signal-handler issue.
    Binding {
        target: root
        property: "_currentScale"
        value: root._targetScale
        when: !root._bouncing
    }

    SequentialAnimation {
        id: _bounceAnim
        running: false

        onStarted: root._bouncing = true
        onStopped: {
            root._bouncing = false;
            root._currentScale = root._targetScale;
        }

        NumberAnimation {
            target: root
            property: "_currentScale"
            to: 0.95
            duration: 80
            easing.type: Easing.InQuad
        }
        NumberAnimation {
            target: root
            property: "_currentScale"
            to: root._pressable.hovered ? 1.02 : 1.0
            duration: 180
            easing.type: Easing.OutBack
        }
    }

    Behavior on _currentScale {
        enabled: !root._bouncing
        NumberAnimation {
            duration: 100
            easing.type: Easing.OutCubic
        }
    }

    // ── Private: keyboard press state ─────────────────────────────
    property bool _keyPressed: false

    Keys.onPressed: function (event) {
        if (!root.enabled || event.isAutoRepeat)
            return;
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            root.pressStarted();
            _bounceAnim.restart();
            root.clicked();
            root.released();
            event.accepted = true;
        } else if (event.key === Qt.Key_Space) {
            root._keyPressed = true;
            root.pressStarted();
            event.accepted = true;
        }
    }

    Keys.onReleased: function (event) {
        if (!root.enabled || event.isAutoRepeat)
            return;
        if (event.key === Qt.Key_Space && root._keyPressed) {
            root._keyPressed = false;
            _bounceAnim.restart();
            root.clicked();
            root.released();
            event.accepted = true;
        }
    }

    onActiveFocusChanged: {
        if (!activeFocus && _keyPressed) {
            _keyPressed = false;
            root.canceled();
        }
    }

    onEnabledChanged: {
        if (!enabled && _keyPressed) {
            _keyPressed = false;
            root.canceled();
        }
    }

    // ── Background ─────────────────────────────────────────────────
    Rectangle {
        id: _bg
        anchors.fill: parent
        radius: ThemeManager.primitives.radiusBase
        color: root._bgColor
        border.color: root._hasBorder ? root._borderColor : "transparent"
        border.width: root._hasBorder ? ThemeManager.primitives.borderWidth : 0

        Behavior on color {
            ColorAnimation {
                duration: 80
            }
        }
    }

    // ── Focus ring ─────────────────────────────────────────────────
    Rectangle {
        anchors.fill: _bg
        radius: _bg.radius
        color: "transparent"
        border.width: 2
        border.color: root._shade(500)
        visible: root.activeFocus
        opacity: 0.65
    }

    // ── Content row ────────────────────────────────────────────────
    Row {
        id: _contentRow
        anchors.centerIn: parent
        spacing: root._iconSpacing

        Loader {
            anchors.verticalCenter: parent.verticalCenter
            active: root.leftIcon !== null
            visible: active
            sourceComponent: root.leftIcon
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.text !== ""
            text: root.text
            font.family: ThemeManager.primitives.baseFont.fontFamily
            font.weight: Font.Medium
            font.pixelSize: root._fontSize
            font.underline: root.preset === "link"
            color: root._textColor
        }

        Loader {
            anchors.verticalCenter: parent.verticalCenter
            active: root.rightIcon !== null
            visible: active
            sourceComponent: root.rightIcon
        }
    }

    // ── Interaction overlay ────────────────────────────────────────
    NanPressable {
        id: _pressable
        anchors.fill: parent
        enabled: root.enabled
        onClicked: {
            _bounceAnim.restart();
            root.clicked();
        }
        onReleased: root.released()
        onPressStarted: root.pressStarted()
        onCanceled: root.canceled()
    }
}
