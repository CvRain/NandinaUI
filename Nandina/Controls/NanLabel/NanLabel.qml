// NanLabel.qml
// Theme-aware form label with accessibility support.
// Built for semantic association with form controls via `forId` property.
//
// Features:
//   - Accessible labelling (click to focus associated control)
//   - Required indicator (*) with error-state coloring
//   - Error state support (red tint)
//   - Disabled state support
//   - Left/right icon support
//   - Theme-aware colors (light/dark mode)
//
// ── Minimal usage ──────────────────────────────────────────────────────────
//   NanLabel { text: "Email" }
//
// ── With form control association ─────────────────────────────────────────
//   Column {
//       NanLabel { text: "Email"; forId: "emailInput" }
//       NanInput { id: emailInput }
//   }
//
// ── Required field ────────────────────────────────────────────────────────
//   NanLabel { text: "Password"; required: true }
//
// ── Error state ───────────────────────────────────────────────────────────
//   NanLabel { text: "Username"; error: true }
//
// ── With icons ────────────────────────────────────────────────────────────
//   NanLabel {
//       text: "Website"
//       leftIcon: Component { Text { text: "🔗"; font.pixelSize: 14 } }
//   }
//
// ── Disabled ──────────────────────────────────────────────────────────────
//   NanLabel { text: "Read-only"; disabled: true }

import QtQuick
import Nandina.Theme
import Nandina.Tokens

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

    readonly property int _sizeSm: NanTokens.sizeSm
    readonly property int _sizeMd: NanTokens.sizeMd
    readonly property int _sizeLg: NanTokens.sizeLg

    // ── Geometry ───────────────────────────────────────────────────
    implicitWidth: _contentRow.implicitWidth + _hPadding * 2
    implicitHeight: _height

    // ── Content ────────────────────────────────────────────────────
    /// Label text content.
    property string text: ""

    // ── Accessibility ──────────────────────────────────────────────
    /// ID of the associated form control (similar to HTML `for` attribute).
    /// Clicking the label will focus this control.
    property string forId: ""

    // ── State ──────────────────────────────────────────────────────
    property bool disabled: false
    property bool error: false
    property bool required: false

    // ── Icons ──────────────────────────────────────────────────────
    /// Optional icon Component placed left of the text.
    property Component leftIcon: null

    /// Optional icon Component placed right of the text.
    property Component rightIcon: null

    // ── Typography ─────────────────────────────────────────────────
    /// Font size in pixels. -1 = auto (theme-based).
    property real fontSize: -1

    /// Font weight (Font.Medium, Font.Bold, etc.).
    property int fontWeight: Font.Medium

    // ── Resolved values (read-only) ────────────────────────────────
    readonly property color resolvedTextColor: _textColor
    readonly property real resolvedFontSize: fontSize >= 0 ? fontSize : _fontSize

    // ── Signals ────────────────────────────────────────────────────
    signal clicked
    signal pressStarted
    signal released

    // ── Accessibility ─────────────────────────────────────────────
    activeFocusOnTab: !root.disabled
    Accessible.role: Accessible.StaticText
    Accessible.name: root.text + (root.required ? " required" : "")
    Accessible.focusable: !root.disabled && root.forId !== ""

    // ── Private: size tokens ───────────────────────────────────────
    readonly property bool _isDark: ThemeManager.darkMode
    readonly property real _sp: ThemeManager.primitives.spacing
    readonly property real _fontSize: Math.round(13 * ThemeManager.primitives.textScaling)
    readonly property real _height: 36  // Match standard form row height
    readonly property real _hPadding: _sp * 1  // Minimal horizontal padding

    // ── Private: palette ───────────────────────────────────────────
    readonly property var _palette: ThemeManager.colors.palette(root._colorSurface)
    readonly property var _errorPalette: ThemeManager.colors.palette(root._colorError)

    // ── Shade helper ───────────────────────────────────────────────
    function _shade(s) {
        if (!_palette)
            return "transparent";
        const _ = _palette.shade500;
        return _palette.shade(NanTokens.shadeIndex(s));
    }

    function _errorShade(s) {
        if (!_errorPalette)
            return "transparent";
        const _ = _errorPalette.shade500;
        return _errorPalette.shade(NanTokens.shadeIndex(s));
    }

    // ── Private: text colour ───────────────────────────────────────
    // Priority: error > disabled > normal
    readonly property color _textColor: {
        if (root.disabled)
            return _shade(_isDark ? 500 : 400);  // Muted in disabled state
        if (root.error)
            return _errorShade(_isDark ? 400 : 600);  // Error color
        return _shade(_isDark ? 200 : 700);  // Normal text color
    }

    // ── Private: required indicator colour ─────────────────────────
    readonly property color _requiredColor: {
        if (root.error)
            return _errorShade(500);
        return _shade(_isDark ? 400 : 600);  // Match primary color
    }

    // ── Background (tap target) ────────────────────────────────────
    Rectangle {
        id: _tapTarget
        anchors.fill: parent
        radius: ThemeManager.primitives.radiusBase
        color: "transparent"

        // Visual feedback on hover/press
        readonly property color _hoverColor: _shadeA(500, 0.08)
        readonly property color _pressColor: _shadeA(500, 0.12)

        function _shadeA(s, alpha) {
            const c = _shade(s);
            return Qt.rgba(c.r, c.g, c.b, alpha);
        }

        states: [
            State {
                name: "hovered"
                when: _pressable.hovered && !root.disabled
                PropertyChanges { target: _tapTarget; color: _tapTarget._hoverColor }
            },
            State {
                name: "pressed"
                when: _pressable.pressed && !root.disabled
                PropertyChanges { target: _tapTarget; color: _tapTarget._pressColor }
            }
        ]

        transitions: [
            Transition {
                ColorAnimation {
                    duration: 80
                    easing.type: Easing.InOutQuad
                }
            }
        ]
    }

    // ── Focus ring (when label triggers focus on control) ──────────
    Rectangle {
        anchors.fill: _tapTarget
        radius: _tapTarget.radius
        color: "transparent"
        border.width: ThemeManager.primitives.ringWidth
        border.color: ThemeManager.primitives.resolveFocusRingColor(ThemeManager.darkMode)
        visible: _controlHasFocus
    }

    // ── Content row ────────────────────────────────────────────────
    Row {
        id: _contentRow
        anchors.centerIn: parent
        spacing: _iconSpacing
        height: parent.height

        readonly property real _iconSpacing: _sp * 1.5

        // Left icon
        Loader {
            id: _leftIconLoader
            anchors.verticalCenter: parent.verticalCenter
            active: root.leftIcon !== null
            visible: active
            sourceComponent: root.leftIcon

            onLoaded: {
                if (item) {
                    // Allow icon to access resolvedTextColor if needed
                    if (item.hasOwnProperty("color"))
                        item.color = root.resolvedTextColor;
                }
            }
        }

        // Text content
        Text {
            id: _labelText
            anchors.verticalCenter: parent.verticalCenter
            visible: root.text !== ""
            text: root.text
            font.family: ThemeManager.primitives.baseFont.fontFamily
            font.weight: root.fontWeight
            font.pixelSize: root.resolvedFontSize
            color: root.resolvedTextColor
            elide: Text.ElideRight
            renderType: Text.NativeRendering

            onTextChanged: updateText()

            function updateText() {
                text = root.text;
            }
        }

        // Required asterisk (separate item for color control)
        Text {
            id: _requiredAsterisk
            anchors.verticalCenter: parent.verticalCenter
            visible: root.required
            text: "*"
            font.family: ThemeManager.primitives.baseFont.fontFamily
            font.weight: Font.Bold
            font.pixelSize: root.resolvedFontSize
            color: _requiredAsterisk.requiredColor
        }

        // Right icon
        Loader {
            id: _rightIconLoader
            anchors.verticalCenter: parent.verticalCenter
            active: root.rightIcon !== null
            visible: active
            sourceComponent: root.rightIcon

            onLoaded: {
                if (item) {
                    if (item.hasOwnProperty("color"))
                        item.color = root.resolvedTextColor;
                }
            }
        }
    }

    // ── Interaction handler ────────────────────────────────────────
    NanPressable {
        id: _pressable
        anchors.fill: parent
        enabled: !root.disabled
        onClicked: {
            root.clicked();
            root._focusAssociatedControl();
        }
        onPressStarted: root.pressStarted()
        onReleased: root.released()
    }

    // ── Keyboard activation ────────────────────────────────────────
    Keys.onPressed: function (event) {
        if (root.disabled || event.isAutoRepeat)
            return;
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            root.pressStarted();
            root.clicked();
            root._focusAssociatedControl();
            root.released();
            event.accepted = true;
        } else if (event.key === Qt.Key_Space) {
            root.pressStarted();
            root._focusAssociatedControl();
            event.accepted = true;
        }
    }

    Keys.onReleased: function (event) {
        if (root.disabled || event.isAutoRepeat)
            return;
        if (event.key === Qt.Key_Space) {
            root.released();
            event.accepted = true;
        }
    }

    // ── Focus tracking ─────────────────────────────────────────────
    readonly property bool _controlHasFocus: {
        if (!root.forId)
            return false;
        // Find the associated control and check its focus
        const control = _findControlById(root.forId);
        return control ? control.activeFocus : false;
    }

    // ── Helper: find control by ID ─────────────────────────────────
    function _findControlById(id) {
        if (!id)
            return null;
        
        // Try to find in parent hierarchy
        let parentItem = root.parent;
        while (parentItem) {
            // Check if parent has the control as a direct property
            if (parentItem[id])
                return parentItem[id];
            
            // Try to find by objectName
            const children = parentItem.children;
            for (let i = 0; i < children.length; i++) {
                if (children[i].objectName === id)
                    return children[i];
            }
            
            parentItem = parentItem.parent;
        }
        
        return null;
    }

    // ── Helper: focus associated control ───────────────────────────
    function _focusAssociatedControl() {
        if (!root.forId || root.disabled)
            return;
        
        const control = _findControlById(root.forId);
        if (control) {
            if (control.forceActiveFocus)
                control.forceActiveFocus();
            else if (control.activeFocus = true)
                control.activeFocus = true;
        }
    }

    // ── State changes ──────────────────────────────────────────────
    onDisabledChanged: {
        if (disabled && _pressable.pressed)
            root.released();
    }
}
