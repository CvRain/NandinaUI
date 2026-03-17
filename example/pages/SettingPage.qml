pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import Nandina.Controls
import Nandina.Theme
import Nandina.Types

NanPage {
    id: root

    readonly property var _colorVariantTypes: ThemeVariant.ColorVariantTypes || ({})
    readonly property var _presetTypes: ThemeVariant.PresetTypes || ({})

    readonly property int _colorPrimary: _colorVariantTypes.Primary ?? 0
    readonly property int _colorSurface: _colorVariantTypes.Surface ?? 6

    readonly property int _presetFilled: _presetTypes.Filled ?? 0
    readonly property int _presetOutlined: _presetTypes.Outlined ?? 2

    readonly property var _window: Window.window
    readonly property string _currentTitleBarMode: _window && _window.titleBarMode ? _window.titleBarMode : "frameless"

    function _setTitleBarMode(mode) {
        if (!_window || _window.titleBarMode === undefined)
            return;
        _window.titleBarMode = mode;
    }

    NanCard {
        width: parent.width
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 15
        title: qsTr("window setting")
        description: qsTr("set window title bar behavior and appearance.")
        interactive: true
        enableBanner: false

        GridLayout {
            columns: 2
            anchors.margins: 15
            rowSpacing: 15
            columnSpacing: 30

            NanButton {
                preset: root._currentTitleBarMode === "system" ? root._presetFilled : root._presetOutlined
                colorVariant: root._currentTitleBarMode === "system" ? root._colorPrimary : root._colorSurface
                text: qsTr("system")
                Layout.fillWidth: true
                Layout.fillHeight: true

                onClicked: root._setTitleBarMode("system")
            }
            Text {
                text: qsTr("OS-native decoration; no custom title bar shown.Background = theme body-background color.")
                Layout.fillWidth: true
                Layout.fillHeight: true
                wrapMode: Text.WordWrap
                color: ThemeManager.colors.surface.shade600
            }

            NanButton {
                preset: root._currentTitleBarMode === "frameless" ? root._presetFilled : root._presetOutlined
                colorVariant: root._currentTitleBarMode === "frameless" ? root._colorPrimary : root._colorSurface
                text: qsTr("frameless")
                Layout.fillWidth: true
                Layout.fillHeight: true

                onClicked: root._setTitleBarMode("frameless")
            }
            Text {
                text: qsTr("Qt.FramelessWindowHint. Uses the built-in NanTitleBar(icon, title, minimize/maximize/close). Rounded corners, thin border, 8-direction system resize.")
                Layout.fillWidth: true
                Layout.fillHeight: true
                wrapMode: Text.WordWrap
                color: ThemeManager.colors.surface.shade600
            }

            NanButton {
                preset: root._currentTitleBarMode === "custom" ? root._presetFilled : root._presetOutlined
                colorVariant: root._currentTitleBarMode === "custom" ? root._colorPrimary : root._colorSurface
                text: qsTr("custom")
                Layout.fillWidth: true
                Layout.fillHeight: true

                onClicked: root._setTitleBarMode("custom")
            }
            Text {
                text: qsTr("Same as \"frameless\" but you supply your own title-bar\n via the titleBar property. Optionally inject the OS window-control buttons by setting injectControls: true.")
                Layout.fillWidth: true
                Layout.fillHeight: true
                wrapMode: Text.WordWrap
                color: ThemeManager.colors.surface.shade600
            }
        }
    }
}
