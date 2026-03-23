pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
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

    // Keep mode values aligned with NanWindow.TitleBarModes enum:
    // 0=System, 1=Frameless, 2=Custom
    readonly property int _modeSystem: 0
    readonly property int _modeFrameless: 1
    readonly property int _modeCustom: 2

    readonly property var _window: Window.window
    readonly property int _currentTitleBarMode: (_window && _window.titleBarMode !== undefined) ? _window.titleBarMode : root._modeFrameless

    function _setTitleBarMode(mode) {
        if (!_window || _window.titleBarMode === undefined)
            return;
        _window.titleBarMode = mode;
    }

    NanCard {
        id: titleBarSettingsCard
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
                preset: root._currentTitleBarMode === root._modeSystem ? root._presetFilled : root._presetOutlined
                colorVariant: root._currentTitleBarMode === root._modeSystem ? root._colorPrimary : root._colorSurface
                text: qsTr("system")
                Layout.fillWidth: true
                Layout.fillHeight: true

                onClicked: root._setTitleBarMode(root._modeSystem)
            }
            Text {
                text: qsTr("OS-native decoration; no custom title bar shown.Background = theme body-background color.")
                Layout.fillWidth: true
                Layout.fillHeight: true
                wrapMode: Text.WordWrap
                color: ThemeManager.colors.surface.shade600
            }

            NanButton {
                preset: root._currentTitleBarMode === root._modeFrameless ? root._presetFilled : root._presetOutlined
                colorVariant: root._currentTitleBarMode === root._modeFrameless ? root._colorPrimary : root._colorSurface
                text: qsTr("frameless")
                Layout.fillWidth: true
                Layout.fillHeight: true

                onClicked: root._setTitleBarMode(root._modeFrameless)
            }
            Text {
                text: qsTr("Qt.FramelessWindowHint. Uses the built-in NanTitleBar(icon, title, minimize/maximize/close). Rounded corners, thin border, 8-direction system resize.")
                Layout.fillWidth: true
                Layout.fillHeight: true
                wrapMode: Text.WordWrap
                color: ThemeManager.colors.surface.shade600
            }

            NanButton {
                preset: root._currentTitleBarMode === root._modeCustom ? root._presetFilled : root._presetOutlined
                colorVariant: root._currentTitleBarMode === root._modeCustom ? root._colorPrimary : root._colorSurface
                text: qsTr("custom")
                Layout.fillWidth: true
                Layout.fillHeight: true

                onClicked: root._setTitleBarMode(root._modeCustom)
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

    NanCard {
        id: themeSettingsCard
        width: parent.width
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: titleBarSettingsCard.bottom
        anchors.topMargin: 15
        title: qsTr("theme setting")
        description: qsTr("set application theme color and preset.")
        interactive: true
        enableBanner: false

        Flow {
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                model: ThemeManager.availableThemes
                delegate: NanButton {
                    required property string modelData
                    text: modelData
                    preset: ThemeManager.currentThemeName === modelData ? root._presetFilled : root._presetOutlined
                    colorVariant: root._colorPrimary
                    onClicked: ThemeManager.setThemeByName(modelData)
                }
            }

            NanButton {
                id: _darkModeBtn
                text: ThemeManager.darkMode ? "Light" : "Dark"
                leftIcon: ThemeManager.darkMode ? _darkLeftIconComponent : _lightModeBtnBackground
                onClicked: ThemeManager.darkMode = !ThemeManager.darkMode
                enabled: true

                Component {
                    id: _darkLeftIconComponent
                    // ☀️
                    Text {
                        text: "☀️"
                        font.pixelSize: 12
                        color: ThemeManager.darkMode ? "#ffffff" : ThemeManager.colors.surface.shade800
                    }
                }

                Component {
                    id: _lightModeBtnBackground
                    // 🌙
                    Text {
                        text: "🌙"
                        font.pixelSize: 12
                        color: ThemeManager.darkMode ? "#ffffff" : ThemeManager.colors.surface.shade800
                    }
                }
            }
        }
    }

    NanCard {
        id: aboutCard
        width: parent.width
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: themeSettingsCard.bottom
        anchors.topMargin: 15
        title: qsTr("about this example")
        description: qsTr("This example demonstrates how to use NanWindow's titleBarMode property to switch between different window title bar behaviors and appearances. It also shows how to use ThemeManager to change the application's theme and dark mode setting.")
        interactive: false
        enableBanner: false
    }

    NanCard {
        id: colorPaletteCard
        width: parent.width
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: aboutCard.bottom
        anchors.topMargin: 15
        title: qsTr("color palette")
        description: qsTr("The color palette for the current theme. Each color role (primary, secondary, etc.) may have multiple shades. The exact number of shades and their values depend on the theme.")
        interactive: false
        enableBanner: false

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                width: parent.width
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 14

                Text {
                    text: root.routeSpec?.navTitle ?? ""
                    font.pixelSize: 28
                    font.bold: true
                    color: ThemeManager.colors.primary.shade700
                }

                Text {
                    text: root.routeSpec?.summary ?? ""
                    font.pixelSize: 13
                    color: ThemeManager.colors.surface.shade600
                }

                ColorRow {
                    label: "Primary"
                    colorPalette: ThemeManager.colors.primary
                }
                ColorRow {
                    label: "Secondary"
                    colorPalette: ThemeManager.colors.secondary
                }
                ColorRow {
                    label: "Tertiary"
                    colorPalette: ThemeManager.colors.tertiary
                }
                ColorRow {
                    label: "Success"
                    colorPalette: ThemeManager.colors.success
                }
                ColorRow {
                    label: "Warning"
                    colorPalette: ThemeManager.colors.warning
                }
                ColorRow {
                    label: "Error"
                    colorPalette: ThemeManager.colors.error
                }
                ColorRow {
                    label: "Surface"
                    colorPalette: ThemeManager.colors.surface
                }

                Item {
                    Layout.preferredHeight: 24
                }
            }
        }
    }

    component ColorRow: ColumnLayout {
        id: colorRow
        property string label
        property var colorPalette
        spacing: 4

        Text {
            text: colorRow.label
            font.pixelSize: 13
            font.bold: true
            color: ThemeManager.colors.surface.shade700
        }

        Row {
            spacing: 2

            Repeater {
                model: [
                    {
                        idx: 0,
                        name: "50"
                    },
                    {
                        idx: 1,
                        name: "100"
                    },
                    {
                        idx: 2,
                        name: "200"
                    },
                    {
                        idx: 3,
                        name: "300"
                    },
                    {
                        idx: 4,
                        name: "400"
                    },
                    {
                        idx: 5,
                        name: "500"
                    },
                    {
                        idx: 6,
                        name: "600"
                    },
                    {
                        idx: 7,
                        name: "700"
                    },
                    {
                        idx: 8,
                        name: "800"
                    },
                    {
                        idx: 9,
                        name: "900"
                    },
                    {
                        idx: 10,
                        name: "950"
                    }
                ]

                delegate: Rectangle {
                    required property var modelData
                    required property int index

                    readonly property color shadeColor: colorRow.colorPalette ? colorRow.colorPalette.shade(modelData.idx) : "transparent"

                    width: 56
                    height: 40
                    radius: 4
                    color: shadeColor
                    border.color: Qt.darker(shadeColor, 1.15)
                    border.width: 0.5

                    Text {
                        anchors.centerIn: parent
                        text: parent.modelData.name
                        font.pixelSize: 9
                        color: parent.index < 5 ? "#333333" : "#ffffff"
                    }
                }
            }
        }
    }
}
