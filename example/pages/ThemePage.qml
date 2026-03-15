pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nandina.Theme
import Nandina.Controls
import Nandina.Types

NanPage {
    id: root

    readonly property var _colorVariantTypes: ThemeVariant.ColorVariantTypes || ({})
    readonly property var _presetTypes: ThemeVariant.PresetTypes || ({})

    readonly property int _colorPrimary: _colorVariantTypes.Primary ?? 0
    readonly property int _presetFilled: _presetTypes.Filled ?? 0
    readonly property int _presetOutlined: _presetTypes.Outlined ?? 2

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 16

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

            Text {
                text: "Current theme: " + ThemeManager.currentThemeName + (ThemeManager.darkMode ? " (dark)" : " (light)")
                font.pixelSize: 14
                color: ThemeManager.colors.surface.shade700
            }

            Text {
                text: "Switch Theme"
                font.pixelSize: 18
                font.bold: true
                color: ThemeManager.colors.primary.shade600
            }

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

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 120
                radius: ThemeManager.primitives.radiusContainer
                color: ThemeManager.colors.surface.shade100
                border.color: ThemeManager.colors.surface.shade300
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "Use left sidebar to switch demo pages"
                    font.pixelSize: 14
                    color: ThemeManager.colors.surface.shade600
                }
            }

            Item {
                Layout.preferredHeight: 24
            }
        }
    }
}
