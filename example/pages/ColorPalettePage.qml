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
            spacing: 14

            Text {
                text: "Color Palettes"
                font.pixelSize: 28
                font.bold: true
                color: ThemeManager.colors.primary.shade700
            }

            Text {
                text: "展示语义色板在 50~950 各色阶的实际视觉效果。"
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
