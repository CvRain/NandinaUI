import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nandina.Core
import Nandina.Theme

ApplicationWindow {
    id: root
    visible: true
    width: 720
    height: 580
    title: "Nandina Theme Demo — " + ThemeManager.currentThemeName

    color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade950 : ThemeManager.colors.surface.shade50

    // ── Helper: draw a triangle via Canvas ──
    component ColorTriangle: Canvas {
        id: tri
        required property color fillColor
        property real triSize: 80
        width: triSize
        height: triSize * 0.866  // √3/2

        onFillColorChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            ctx.beginPath();
            ctx.moveTo(width / 2, 0);
            ctx.lineTo(width, height);
            ctx.lineTo(0, height);
            ctx.closePath();
            ctx.fillStyle = fillColor;
            ctx.fill();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ThemeManager.primitives.spacing * 4
        spacing: ThemeManager.primitives.spacing * 4

        // ── Title ──
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Nandina Theme Preview"
            font.pixelSize: 28 * ThemeManager.primitives.textScaling
            font.family: ThemeManager.primitives.headingFont.fontFamily
            font.bold: true
            color: ThemeManager.darkMode ? ThemeManager.primitives.headingFont.fontColorDark : ThemeManager.primitives.headingFont.fontColor
        }

        // ── Current theme label ──
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Current: " + ThemeManager.currentThemeName + (ThemeManager.darkMode ? " (dark)" : " (light)")
            font.pixelSize: 14
            font.family: ThemeManager.primitives.baseFont.fontFamily
            color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade600
        }

        // ── Triangles row (using enum-indexed color access) ──
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: ThemeManager.primitives.spacing * 6

            Repeater {
                model: [
                    {
                        label: "Primary",
                        variant: NandinaType.Primary
                    },
                    {
                        label: "Secondary",
                        variant: NandinaType.Secondary
                    },
                    {
                        label: "Tertiary",
                        variant: NandinaType.Tertiary
                    },
                    {
                        label: "Success",
                        variant: NandinaType.Success
                    },
                    {
                        label: "Warning",
                        variant: NandinaType.Warning
                    },
                    {
                        label: "Error",
                        variant: NandinaType.Error
                    }
                ]

                delegate: ColumnLayout {
                    spacing: ThemeManager.primitives.spacing * 2

                    ColorTriangle {
                        Layout.alignment: Qt.AlignHCenter
                        // Demonstrate enum-indexed access: color(variant, accent)
                        fillColor: ThemeManager.colors.color(modelData.variant, NandinaType.Shade500)
                        triSize: 64
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: modelData.label
                        font.pixelSize: 12
                        font.family: ThemeManager.primitives.baseFont.fontFamily
                        color: ThemeManager.darkMode ? ThemeManager.primitives.baseFont.fontColorDark : ThemeManager.primitives.baseFont.fontColor
                    }
                }
            }
        }

        // ── Color shades strip (primary) — named property access ──
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: ThemeManager.primitives.spacing

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "Primary Palette"
                font.pixelSize: 16 * ThemeManager.primitives.textScaling
                font.family: ThemeManager.primitives.headingFont.fontFamily
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.primitives.headingFont.fontColorDark : ThemeManager.primitives.headingFont.fontColor
            }

            Row {
                Layout.alignment: Qt.AlignHCenter
                spacing: 2

                Repeater {
                    model: [ThemeManager.colors.primary.shade50, ThemeManager.colors.primary.shade100, ThemeManager.colors.primary.shade200, ThemeManager.colors.primary.shade300, ThemeManager.colors.primary.shade400, ThemeManager.colors.primary.shade500, ThemeManager.colors.primary.shade600, ThemeManager.colors.primary.shade700, ThemeManager.colors.primary.shade800, ThemeManager.colors.primary.shade900, ThemeManager.colors.primary.shade950]

                    delegate: Rectangle {
                        width: 48
                        height: 32
                        radius: ThemeManager.primitives.radiusBase
                        color: modelData
                    }
                }
            }
        }

        // ── Surface shades strip — enum-indexed access demo ──
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: ThemeManager.primitives.spacing

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "Surface Palette"
                font.pixelSize: 16 * ThemeManager.primitives.textScaling
                font.family: ThemeManager.primitives.headingFont.fontFamily
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.primitives.headingFont.fontColorDark : ThemeManager.primitives.headingFont.fontColor
            }

            Row {
                Layout.alignment: Qt.AlignHCenter
                spacing: 2

                Repeater {
                    // Enum-indexed: iterate shade accents by enum value
                    model: [NandinaType.Shade50, NandinaType.Shade100, NandinaType.Shade200, NandinaType.Shade300, NandinaType.Shade400, NandinaType.Shade500, NandinaType.Shade600, NandinaType.Shade700, NandinaType.Shade800, NandinaType.Shade900, NandinaType.Shade950]

                    delegate: Rectangle {
                        width: 48
                        height: 32
                        radius: ThemeManager.primitives.radiusBase
                        color: ThemeManager.colors.color(NandinaType.Surface, modelData)
                        border.width: ThemeManager.primitives.borderWidth
                        border.color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade300
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }

        // ── Theme switch buttons ──
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: ThemeManager.primitives.spacing * 3

            Repeater {
                model: ThemeManager.availableThemes

                delegate: Button {
                    text: modelData
                    highlighted: ThemeManager.currentThemeName === modelData
                    onClicked: ThemeManager.setThemeByName(modelData)

                    background: Rectangle {
                        radius: ThemeManager.primitives.radiusBase
                        color: ThemeManager.currentThemeName === modelData ? ThemeManager.colors.primary.shade500 : (ThemeManager.darkMode ? ThemeManager.colors.surface.shade800 : ThemeManager.colors.surface.shade200)
                        border.width: ThemeManager.primitives.borderWidth
                        border.color: ThemeManager.colors.primary.shade400
                    }

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 13
                        font.family: ThemeManager.primitives.baseFont.fontFamily
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: ThemeManager.currentThemeName === modelData ? ThemeManager.colors.primary.contrastLight : (ThemeManager.darkMode ? ThemeManager.colors.surface.shade100 : ThemeManager.colors.surface.shade800)
                    }
                }
            }

            // Dark mode toggle
            Button {
                text: ThemeManager.darkMode ? "☀ Light" : "🌙 Dark"
                onClicked: ThemeManager.darkMode = !ThemeManager.darkMode

                background: Rectangle {
                    radius: ThemeManager.primitives.radiusBase
                    color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade300
                }

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 13
                    font.family: ThemeManager.primitives.baseFont.fontFamily
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade50 : ThemeManager.colors.surface.shade900
                }
            }
        }
    }
}
