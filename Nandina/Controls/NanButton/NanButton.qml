// NanButton.qml
// Theme-aware clickable button built on NanPressable.
//
// Preset semantics (mirrors NanCard preset philosophy):
//   ThemeVariant.PresetTypes.Filled   — solid shade-500 fill, white text, high emphasis   [default]
//   ThemeVariant.PresetTypes.Tonal    — lightly tinted bg (shade 100 / 200), variant-coloured text
//   ThemeVariant.PresetTypes.Outlined — transparent bg, coloured border, no fill
//   ThemeVariant.PresetTypes.Ghost    — no bg, no border, hover-only tint; minimal visual footprint
//   ThemeVariant.PresetTypes.Link     — text-only with underline, for inline / semantic anchors
//
// ── Minimal usage ──────────────────────────────────────────────────────────
//   NanButton { text: "Save" }
//
// ── Preset + variant ───────────────────────────────────────────────────────
//   NanButton { text: "Delete"; preset: ThemeVariant.PresetTypes.Outlined; colorVariant: ThemeVariant.ColorVariantTypes.Error }
//   NanButton { text: "Subscribe"; preset: ThemeVariant.PresetTypes.Tonal;  colorVariant: ThemeVariant.ColorVariantTypes.Success }
//
// ── Sizes ──────────────────────────────────────────────────────────────────
//   NanButton { text: "Compact"; size: ThemeVariant.SizeTypes.Sm }
//   NanButton { text: "Submit";  size: ThemeVariant.SizeTypes.Lg }
//
// ── With left icon ─────────────────────────────────────────────────────────
//   NanButton {
//       text: "Upload"
//       colorVariant: ThemeVariant.ColorVariantTypes.Primary
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
import Nandina.Types
import Nandina.Controls

Item {
    id: root

    readonly property var _colorVariantTypes: ThemeVariant.ColorVariantTypes || ({})
    readonly property var _presetTypes: ThemeVariant.PresetTypes || ({})
    readonly property var _sizeTypes: ThemeVariant.SizeTypes || ({})

    readonly property int _colorPrimary: _colorVariantTypes.Primary ?? 0
    readonly property int _colorSecondary: _colorVariantTypes.Secondary ?? 1
    readonly property int _colorTertiary: _colorVariantTypes.Tertiary ?? 2
    readonly property int _colorSuccess: _colorVariantTypes.Success ?? 3
    readonly property int _colorWarning: _colorVariantTypes.Warning ?? 4
    readonly property int _colorError: _colorVariantTypes.Error ?? 5
    readonly property int _colorSurface: _colorVariantTypes.Surface ?? 6

    readonly property int _presetFilled: _presetTypes.Filled ?? 0
    readonly property int _presetTonal: _presetTypes.Tonal ?? 1
    readonly property int _presetOutlined: _presetTypes.Outlined ?? 2
    readonly property int _presetGhost: _presetTypes.Ghost ?? 3
    readonly property int _presetLink: _presetTypes.Link ?? 4

    readonly property int _sizeSm: _sizeTypes.Sm ?? 0
    readonly property int _sizeMd: _sizeTypes.Md ?? 1
    readonly property int _sizeLg: _sizeTypes.Lg ?? 2

    // ── Geometry ───────────────────────────────────────────────────
    implicitWidth: Math.max(_minWidth, _contentRow.implicitWidth + _hPadding * 2)
    implicitHeight: _height

    // ── Color ──────────────────────────────────────────────────────
    /// Semantic colour family.
    /// Use ThemeVariant.ColorVariantTypes.Surface … ThemeVariant.ColorVariantTypes.Error.
    property int colorVariant: root._colorPrimary

    // ── Preset ─────────────────────────────────────────────────────
    /// Visual fill style.
    /// Use ThemeVariant.PresetTypes.Filled … ThemeVariant.PresetTypes.Link.
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
    /// Use ThemeVariant.SizeTypes.Sm / Md / Lg.
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
    // ── Private: palette ──────────────────────────────────────────
    readonly property var _palette: [ThemeManager.colors.primary, ThemeManager.colors.secondary, ThemeManager.colors.tertiary, ThemeManager.colors.success, ThemeManager.colors.warning, ThemeManager.colors.error, ThemeManager.colors.surface][colorVariant] ?? ThemeManager.colors.surface

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
        case root._presetFilled:
            if (isPr)
                return _shade(_isDark ? 400 : 600);
            if (isHv)
                return _shade(_isDark ? 600 : 400);
            return _shade(500);
        case root._presetTonal:
            if (isPr)
                return _shade(_isDark ? 400 : 300);
            if (isHv)
                return _shade(_isDark ? 300 : 200);
            return _shade(_isDark ? 200 : 100);
        case root._presetOutlined:
        case root._presetGhost:
        case root._presetLink:
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
    readonly property bool _hasBorder: preset === root._presetOutlined
    readonly property color _borderColor: {
        if (preset !== root._presetOutlined)
            return "transparent";
        if (_pressable.hovered || _pressable.pressed || _keyPressed)
            return _shade(_isDark ? 600 : 600);
        return _shade(500);
    }

    // ── Private: text colour ───────────────────────────────────────
    readonly property color _textColor: {
        switch (preset) {
        case root._presetFilled:
            return "#ffffff";
        case root._presetTonal:
            // dark shade700 = original shade300 = bright; light shade700 = dark
            return _shade(700);
        case root._presetOutlined:
        case root._presetGhost:
        case root._presetLink:
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
