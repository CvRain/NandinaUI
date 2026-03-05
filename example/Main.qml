import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nandina.Theme
import Nandina.Controls

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
                left: parent.left
                right: parent.right
                margins: 24
            }
            spacing: 20

            // ── Header ─────────────────────────────────────────────────
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

            // ── Theme switch buttons ────────────────────────────────────
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
                                   : (parent.hovered ? ThemeManager.colors.surface.shade200
                                                     : ThemeManager.colors.surface.shade100)
                            border.color: ThemeManager.colors.primary.shade300
                            border.width: parent.highlighted ? 0 : 1
                        }
                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: 13
                            font.capitalization: Font.Capitalize
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment:   Text.AlignVCenter
                            color: parent.highlighted ? "#ffffff"
                                                      : ThemeManager.colors.surface.shade800
                        }
                    }
                }

                Button {
                    text: ThemeManager.darkMode ? "☀ Light" : "🌙 Dark"
                    onClicked: ThemeManager.darkMode = !ThemeManager.darkMode

                    background: Rectangle {
                        radius: 6
                        color: parent.hovered ? ThemeManager.colors.tertiary.shade200
                                              : ThemeManager.colors.tertiary.shade100
                        border.color: ThemeManager.colors.tertiary.shade400
                        border.width: 1
                    }
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment:   Text.AlignVCenter
                        color: ThemeManager.colors.tertiary.shade800
                    }
                }
            }

            // ── Color palettes ─────────────────────────────────────────
            Text {
                Layout.topMargin: 16
                text: "Color Palettes"
                font.pixelSize: 18
                font.bold: true
                color: ThemeManager.colors.primary.shade600
            }

            ColorRow { label: "Primary";   colorPalette: ThemeManager.colors.primary   }
            ColorRow { label: "Secondary"; colorPalette: ThemeManager.colors.secondary }
            ColorRow { label: "Tertiary";  colorPalette: ThemeManager.colors.tertiary  }
            ColorRow { label: "Success";   colorPalette: ThemeManager.colors.success   }
            ColorRow { label: "Warning";   colorPalette: ThemeManager.colors.warning   }
            ColorRow { label: "Error";     colorPalette: ThemeManager.colors.error     }
            ColorRow { label: "Surface";   colorPalette: ThemeManager.colors.surface   }

            // ── NanSurface 演示 ────────────────────────────────────────
            Text {
                Layout.topMargin: 16
                text: "NanSurface — 主题感知容器"
                font.pixelSize: 18
                font.bold: true
                color: ThemeManager.colors.primary.shade600
            }
            Text {
                text: "colorVariant 与 shade 自动跟随主题，无需手动切换"
                font.pixelSize: 13
                color: ThemeManager.colors.surface.shade600
                Layout.bottomMargin: 4
            }

            Flow {
                Layout.fillWidth: true
                spacing: 12

                Repeater {
                    model: ["surface","primary","secondary","tertiary","success","warning","error"]
                    delegate: NanSurface {
                        required property string modelData
                        width: 110; height: 72
                        colorVariant: modelData

                        Text {
                            anchors.centerIn: parent
                            text: parent.modelData
                            font.pixelSize: 12
                            font.capitalization: Font.Capitalize
                            color: ThemeManager.darkMode
                                   ? ThemeManager.colors.surface.shade200
                                   : ThemeManager.colors.surface.shade800
                        }
                    }
                }
            }

            // ── NanPressable 演示 ──────────────────────────────────────
            Text {
                Layout.topMargin: 16
                text: "NanPressable — 纯交互原语"
                font.pixelSize: 18
                font.bold: true
                color: ThemeManager.colors.primary.shade600
            }
            Text {
                text: "NanSurface + NanPressable 组合：hover / press / click 状态可观察"
                font.pixelSize: 13
                color: ThemeManager.colors.surface.shade600
                Layout.bottomMargin: 4
            }

            Row {
                spacing: 16

                // 普通点击卡片
                NanSurface {
                    id: _pressDemo
                    width: 160; height: 80
                    colorVariant: "primary"
                    backgroundShade: _pressable.pressed ? 400
                                   : _pressable.hovered ? 100 : -1
                    borderShade: _pressable.hovered ? 400 : -1

                    Behavior on backgroundShade {
                        NumberAnimation { duration: 80 }
                    }

                    Text {
                        anchors.centerIn: parent
                        text: _pressable.pressed ? "▼ Pressed"
                             : _pressable.hovered ? "▲ Hovered"
                             : "👆 Click me"
                        font.pixelSize: 13
                        color: ThemeManager.darkMode
                               ? ThemeManager.colors.primary.shade200
                               : ThemeManager.colors.primary.shade700
                    }

                    NanPressable {
                        id: _pressable
                        anchors.fill: parent
                        onClicked: _clickCount.count++
                    }
                }

                // 禁用状态卡片
                NanSurface {
                    width: 160; height: 80
                    colorVariant: "surface"
                    opacity: 0.45

                    Text {
                        anchors.centerIn: parent
                        text: "🚫 Disabled"
                        font.pixelSize: 13
                        color: ThemeManager.colors.surface.shade500
                    }

                    NanPressable { anchors.fill: parent; enabled: false }
                }

                // 长按卡片
                NanSurface {
                    id: _longPressCard
                    width: 160; height: 80
                    colorVariant: "tertiary"
                    property bool _triggered: false

                    Text {
                        anchors.centerIn: parent
                        text: _longPressCard._triggered ? "✔ Long pressed!" : "⏳ Hold me"
                        font.pixelSize: 13
                        color: ThemeManager.darkMode
                               ? ThemeManager.colors.tertiary.shade200
                               : ThemeManager.colors.tertiary.shade700
                    }

                    NanPressable {
                        anchors.fill: parent
                        longPressInterval: 800
                        onLongPressed: _longPressCard._triggered = !_longPressCard._triggered
                    }
                }
            }

            Text {
                id: _clickCount
                property int count: 0
                text: "Click count: " + count
                font.pixelSize: 14
                color: ThemeManager.colors.primary.shade500
            }

            // ── NanPanel 演示 ──────────────────────────────────────────
            Text {
                Layout.topMargin: 16
                text: "NanPanel — 主题容器 + 可选标题"
                font.pixelSize: 18
                font.bold: true
                color: ThemeManager.colors.primary.shade600
            }
            Text {
                text: "内边距、分割线、标题字体均自动追踪主题 token"
                font.pixelSize: 13
                color: ThemeManager.colors.surface.shade600
                Layout.bottomMargin: 8
            }

            // 无标题 panel
            NanPanel {
                Layout.fillWidth: true
                Text {
                    text: "无标题 Panel — 只有内容区，无分割线"
                    font.pixelSize: 13
                    color: ThemeManager.darkMode
                           ? ThemeManager.colors.surface.shade200
                           : ThemeManager.colors.surface.shade700
                }
            }

            // 带标题 panel
            NanPanel {
                Layout.fillWidth: true
                title: "Panel Title"
                Text {
                    text: "带标题 Panel — 自动显示分割线，手动指定标题"
                    font.pixelSize: 13
                    color: ThemeManager.darkMode
                           ? ThemeManager.colors.surface.shade200
                           : ThemeManager.colors.surface.shade700
                }
            }

            // variant 演示
            Flow {
                Layout.fillWidth: true
                spacing: 12
                Layout.bottomMargin: 12

                Repeater {
                    model: ["primary","success","warning","error"]
                    delegate: NanPanel {
                        required property string modelData
                        width:           220
                        colorVariant:    modelData
                        backgroundShade: ThemeManager.darkMode ? 900 : 50
                        borderShade:     ThemeManager.darkMode ? 700 : 200
                        title: modelData.charAt(0).toUpperCase() + modelData.slice(1) + " Panel"

                        Text {
                            text: "colorVariant: \"" + parent.colorVariant + "\""
                            font.pixelSize: 12
                            color: ThemeManager.darkMode
                                   ? ThemeManager.colors.surface.shade300
                                   : ThemeManager.colors.surface.shade600
                        }
                    }
                }
            }

            // 嵌套 panels
            NanPanel {
                Layout.fillWidth: true
                title: "Outer Panel"

                ColumnLayout {
                    width:   parent.width
                    spacing: 8

                    Text {
                        text: "Panels 可以嵌套，外层用 surface，内层可主动指定 variant"
                        font.pixelSize: 13
                        color: ThemeManager.darkMode
                               ? ThemeManager.colors.surface.shade200
                               : ThemeManager.colors.surface.shade700
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }

                    Row {
                        spacing: 8

                        NanPanel {
                            width:           160
                            title:           "Inner A"
                            colorVariant:    "primary"
                            backgroundShade: ThemeManager.darkMode ? 900 : 50
                            Text {
                                text: "Primary tint"
                                font.pixelSize: 12
                                color: ThemeManager.darkMode
                                       ? ThemeManager.colors.primary.shade200
                                       : ThemeManager.colors.primary.shade700
                            }
                        }

                        NanPanel {
                            width:           160
                            title:           "Inner B"
                            colorVariant:    "tertiary"
                            backgroundShade: ThemeManager.darkMode ? 900 : 50
                            Text {
                                text: "Tertiary tint"
                                font.pixelSize: 12
                                color: ThemeManager.darkMode
                                       ? ThemeManager.colors.tertiary.shade200
                                       : ThemeManager.colors.tertiary.shade700
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 32 }
        }
    }

    // ── Inline helper component ────────────────────────────────────
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
                    { idx: 0,  name: "50"  },
                    { idx: 1,  name: "100" },
                    { idx: 2,  name: "200" },
                    { idx: 3,  name: "300" },
                    { idx: 4,  name: "400" },
                    { idx: 5,  name: "500" },
                    { idx: 6,  name: "600" },
                    { idx: 7,  name: "700" },
                    { idx: 8,  name: "800" },
                    { idx: 9,  name: "900" },
                    { idx: 10, name: "950" }
                ]
                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    readonly property color shadeColor: colorPalette
                        ? colorPalette.shade(modelData.idx) : "transparent"

                    width: 56; height: 40; radius: 4
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
