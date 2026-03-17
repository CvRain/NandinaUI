// NanButton.qml
// Theme-aware clickable button built on NanPressable.
//
// Preset semantics (mirrors NanCard preset philosophy):
//   Filled   — solid shade-500 fill, white text, high emphasis   [default]
//   Tonal    — lightly tinted bg (shade 100 / 200), variant-coloured text
//   Outlined — transparent bg, coloured border, no fill
//   Ghost    — no bg, no border, hover-only tint; minimal visual footprint
//   Link     — text-only with underline, for inline / semantic anchors
//
// ── Minimal usage ──────────────────────────────────────────────────────────
//   NanButton { text: "Save" }
//
// ── Preset + variant ───────────────────────────────────────────────────────
//   NanButton { text: "Delete" }
//   NanButton { text: "Subscribe" }
//
// ── Sizes ──────────────────────────────────────────────────────────────────
//   NanButton { text: "Compact" }
//   NanButton { text: "Submit" }
//
// ── With left icon ─────────────────────────────────────────────────────────
//   NanButton {
//       text: "Upload"
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
import Nandina.Tokens
import Nandina.Controls

Item {
    id: root

    // ── Token aliases (from NanTokens singleton) ──────────────────
    readonly property int _colorPrimary: NanTokens.colorPrimary
    readonly property int _colorSecondary: NanTokens.colorSecondary
    readonly property int _colorTertiary: NanTokens.colorTertiary
    readonly property int _colorSuccess: NanTokens.colorSuccess
    readonly property int _colorWarning: NanTokens.colorWarning
    readonly property int _colorError: NanTokens.colorError
    readonly property int _colorSurface: NanTokens.colorSurface

    readonly property int _presetFilled: NanTokens.presetFilled
    readonly property int _presetTonal: NanTokens.presetTonal
    readonly property int _presetOutlined: NanTokens.presetOutlined
    readonly property int _presetGhost: NanTokens.presetGhost
    readonly property int _presetLink: NanTokens.presetLink

    readonly property int _sizeSm: NanTokens.sizeSm
    readonly property int _sizeMd: NanTokens.sizeMd
    readonly property int _sizeLg: NanTokens.sizeLg

    // ── Geometry ───────────────────────────────────────────────────
    implicitWidth: Math.max(_minWidth, _contentRow.implicitWidth + _hPadding * 2)
    implicitHeight: _height

    // ── Color ──────────────────────────────────────────────────────
    /// Semantic colour family.
    /// Use the shared ColorVariantTypes enum exposed by Nandina.Types.
    property int colorVariant: root._colorPrimary

    // ── Preset ─────────────────────────────────────────────────────
    /// Visual fill style.
    /// Use the shared PresetTypes enum exposed by Nandina.Types.
    property int preset: root._presetFilled

    // ── Label ──────────────────────────────────────────────────────
    property string text: ""

    // ── Icons ──────────────────────────────────────────────────────
    /// Optional icon Component placed left of the label.
    /// The loaded item can reference resolvedTextColor for colour binding.
    property Component leftIcon: null

    /// Optional icon Component placed right of the label.
    property Component rightIcon: null

    // ── Size ───────────────────────────────────────────────────────
    /// Use the shared SizeTypes enum exposed by Nandina.Types.
    property int size: root._sizeMd

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

    readonly property int _height: [30, 36, 42][size] ?? 36
    readonly property int _minWidth: [64, 80, 96][size] ?? 80
    readonly property real _hPadding: [_sp * 3, _sp * 4, _sp * 5][size] ?? (_sp * 4)
    readonly property real _iconSpacing: _sp * 2
    readonly property real _fontSize: [Math.round(12 * ThemeManager.primitives.textScaling), Math.round(13 * ThemeManager.primitives.textScaling), Math.round(15 * ThemeManager.primitives.textScaling)][size] ?? Math.round(13 * ThemeManager.primitives.textScaling)
    // ── Private: palette (via ColorSchema.palette) ────────────────
    readonly property var _palette: ThemeManager.colors.palette(colorVariant)
    readonly property string _interactionState: pressed ? "press" : (hovered ? "hover" : "idle")
    readonly property var _bgConfigs: [
        {
            idle: 500,
            hover: _isDark ? 600 : 400,
            press: _isDark ? 400 : 600,
            alpha: false
        },
        {
            idle: _isDark ? 200 : 100,
            hover: _isDark ? 300 : 200,
            press: _isDark ? 400 : 300,
            alpha: false
        },
        {
            idle: 0,
            hover: 0.08,
            press: 0.16,
            alpha: true
        },
        {
            idle: 0,
            hover: 0.08,
            press: 0.16,
            alpha: true
        },
        {
            idle: 0,
            hover: 0.08,
            press: 0.16,
            alpha: true
        }
    ]

    // ── Private: shade helpers ─────────────────────────────────────────
    function _shade(s) {
        if (!_palette)
            return "transparent";
        // Dummy-read a named shade property so QML tracks _palette.changed.
        // Q_INVOKABLE shade() calls are not auto-tracked by the binding engine.
        const _ = _palette.shade500;
        return _palette.shade(NanTokens.shadeIndex(s));
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
        const config = _bgConfigs[preset];
        if (!config)
            return "transparent";
        const value = config[_interactionState] ?? config.idle;
        return config.alpha ? _shadeA(500, value) : _shade(value);
    }

    // ── Private: border ────────────────────────────────────────────
    readonly property bool _hasBorder: preset === root._presetOutlined
    readonly property color _borderColor: {
        if (preset !== root._presetOutlined)
            return "transparent";
        if (_pressable.hovered || _pressable.pressed || _keyPressed)
            return _shade(_isDark ? 400 : 600);   // FIX: was (_isDark ? 600 : 600)
        return _shade(500);
    }

    // ── Private: text colour ───────────────────────────────────────
    // FIX: Filled + Surface variant → dark text for contrast (was always "#ffffff")
    readonly property color _textColor: {
        if (preset === root._presetFilled)
            return colorVariant === root._colorSurface ? _shade(950) : "#ffffff";
        return ["#ffffff", _shade(700), _shade(_isDark ? 300 : 600), _shade(_isDark ? 300 : 600), _shade(_isDark ? 300 : 600)][preset] ?? _shade(600);
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
            to: root.hovered ? 1.02 : 1.0
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
        border.width: ThemeManager.primitives.ringWidth
        border.color: ThemeManager.primitives.resolveFocusRingColor(ThemeManager.darkMode)
        visible: root.activeFocus
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
            font.underline: root.preset === root._presetLink
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
