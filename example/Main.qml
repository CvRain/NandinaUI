import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nandina.Theme

ApplicationWindow {
    id: root
    visible: true
    width: 800
    height: 600
    title: "Nandina Theme Example — " + ThemeManager.currentThemeName
    color: ThemeManager.colors.surface.shade50

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            anchors {
                left: parent.left; right: parent.right
                margins: 24
            }
            spacing: 20

            // ── Header ─────────────────────────────────────────
            Text {
                Layout.topMargin: 24
                text: "Nandina Theme Preview"
                font.pixelSize: 28
                font.bold: true
                color: ThemeManager.colors.primary.shade700
            }

            Text {
                text: "Current theme: " + ThemeManager.currentThemeName
                      + (ThemeManager.darkMode ? " (dark)" : " (light)")
                font.pixelSize: 14
                color: ThemeManager.colors.surface.shade700
            }

            // ── Theme switch buttons ───────────────────────────
            Text {
                Layout.topMargin: 12
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
                        text: modelData
                        highlighted: ThemeManager.currentThemeName === modelData
                        onClicked: ThemeManager.setThemeByName(modelData)

                        background: Rectangle {
                            radius: 6
                            color: parent.highlighted
                                   ? ThemeManager.colors.primary.shade500
                                   : (parent.hovered
                                      ? ThemeManager.colors.surface.shade200
                                      : ThemeManager.colors.surface.shade100)
                            border.color: ThemeManager.colors.primary.shade300
                            border.width: parent.highlighted ? 0 : 1
                        }
                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: 13
                            font.capitalization: Font.Capitalize
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: parent.highlighted
                                   ? "#ffffff"
                                   : ThemeManager.colors.surface.shade800
                        }
                    }
                }

                Button {
                    text: ThemeManager.darkMode ? "☀ Light" : "🌙 Dark"
                    onClicked: ThemeManager.darkMode = !ThemeManager.darkMode

                    background: Rectangle {
                        radius: 6
                        color: parent.hovered
                               ? ThemeManager.colors.tertiary.shade200
                               : ThemeManager.colors.tertiary.shade100
                        border.color: ThemeManager.colors.tertiary.shade400
                        border.width: 1
                    }
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: ThemeManager.colors.tertiary.shade800
                    }
                }
            }

            // ── Color palettes ─────────────────────────────────
            Text {
                Layout.topMargin: 16
                text: "Color Palettes"
                font.pixelSize: 18
                font.bold: true
                color: ThemeManager.colors.primary.shade600
            }

            // Helper component for each color row
            component ColorRow: ColumnLayout {
                property string label
                property var colorPalette
                spacing: 4

                Text {
                    text: label
                    font.pixelSize: 13
                    font.bold: true
                    color: ThemeManager.colors.surface.shade700
                }

                Row {
                    spacing: 2
                    Repeater {
                        model: [
                            { idx: 0, name: "50"  }, { idx: 1, name: "100" },
                            { idx: 2, name: "200" }, { idx: 3, name: "300" },
                            { idx: 4, name: "400" }, { idx: 5, name: "500" },
                            { idx: 6, name: "600" }, { idx: 7, name: "700" },
                            { idx: 8, name: "800" }, { idx: 9, name: "900" },
                            { idx: 10, name: "950" }
                        ]
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            readonly property color shadeColor: colorPalette ? colorPalette.shade(modelData.idx) : "transparent"

                            width: 56; height: 40
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

            ColorRow { label: "Primary";   colorPalette: ThemeManager.colors.primary }
            ColorRow { label: "Secondary"; colorPalette: ThemeManager.colors.secondary }
            ColorRow { label: "Tertiary";  colorPalette: ThemeManager.colors.tertiary }
            ColorRow { label: "Success";   colorPalette: ThemeManager.colors.success }
            ColorRow { label: "Warning";   colorPalette: ThemeManager.colors.warning }
            ColorRow { label: "Error";     colorPalette: ThemeManager.colors.error }
            ColorRow { label: "Surface";   colorPalette: ThemeManager.colors.surface }

            // ── Sample UI elements ─────────────────────────────
            Text {
                Layout.topMargin: 16
                text: "Sample Elements"
                font.pixelSize: 18
                font.bold: true
                color: ThemeManager.colors.primary.shade600
            }

            Row {
                spacing: 12

                Rectangle {
                    width: 120; height: 80
                    radius: 8
                    color: ThemeManager.colors.primary.shade500
                    Text {
                        anchors.centerIn: parent
                        text: "Primary"
                        color: "#ffffff"
                        font.pixelSize: 14
                    }
                }
                Rectangle {
                    width: 120; height: 80
                    radius: 8
                    color: ThemeManager.colors.success.shade500
                    Text {
                        anchors.centerIn: parent
                        text: "Success"
                        color: "#ffffff"
                        font.pixelSize: 14
                    }
                }
                Rectangle {
                    width: 120; height: 80
                    radius: 8
                    color: ThemeManager.colors.warning.shade500
                    Text {
                        anchors.centerIn: parent
                        text: "Warning"
                        color: "#ffffff"
                        font.pixelSize: 14
                    }
                }
                Rectangle {
                    width: 120; height: 80
                    radius: 8
                    color: ThemeManager.colors.error.shade500
                    Text {
                        anchors.centerIn: parent
                        text: "Error"
                        color: "#ffffff"
                        font.pixelSize: 14
                    }
                }
            }

            Item { Layout.fillHeight: true; Layout.bottomMargin: 24 }
        }
    }
}