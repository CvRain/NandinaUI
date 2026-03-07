pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nandina.Theme

Item {
    id: root

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 16

            Text {
                text: "Theme"
                font.pixelSize: 28
                font.bold: true
                color: ThemeManager.colors.primary.shade700
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
                    delegate: Button {
                        id: _themeBtn
                        required property string modelData
                        text: modelData
                        highlighted: ThemeManager.currentThemeName === modelData
                        onClicked: ThemeManager.setThemeByName(modelData)

                        background: Rectangle {
                            radius: 6
                            color: _themeBtn.highlighted ? ThemeManager.colors.primary.shade500 : (_themeBtn.hovered ? ThemeManager.colors.surface.shade200 : ThemeManager.colors.surface.shade100)
                            border.color: ThemeManager.colors.primary.shade300
                            border.width: _themeBtn.highlighted ? 0 : 1
                        }

                        contentItem: Text {
                            text: _themeBtn.text
                            font.pixelSize: 13
                            font.capitalization: Font.Capitalize
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: _themeBtn.highlighted ? "#ffffff" : ThemeManager.colors.surface.shade800
                        }
                    }
                }

                Button {
                    id: _darkModeBtn
                    text: ThemeManager.darkMode ? "☀ Light" : "🌙 Dark"
                    onClicked: ThemeManager.darkMode = !ThemeManager.darkMode

                    background: Rectangle {
                        radius: 6
                        color: _darkModeBtn.hovered ? ThemeManager.colors.tertiary.shade200 : ThemeManager.colors.tertiary.shade100
                        border.color: ThemeManager.colors.tertiary.shade400
                        border.width: 1
                    }

                    contentItem: Text {
                        text: _darkModeBtn.text
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: ThemeManager.colors.tertiary.shade800
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
