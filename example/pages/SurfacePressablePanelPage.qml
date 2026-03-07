pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nandina.Theme
import Nandina.Controls

Item {
    id: root

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 16

            Text {
                text: "Surface / Pressable / Panel"
                font.pixelSize: 28
                font.bold: true
                color: ThemeManager.colors.primary.shade700
            }

            Text {
                text: "NanSurface — 主题感知容器"
                font.pixelSize: 18
                font.bold: true
                color: ThemeManager.colors.primary.shade600
            }

            Flow {
                Layout.fillWidth: true
                spacing: 12

                Repeater {
                    model: ["surface", "primary", "secondary", "tertiary", "success", "warning", "error"]
                    delegate: NanSurface {
                        required property string modelData
                        width: 110
                        height: 72
                        colorVariant: modelData

                        Text {
                            anchors.centerIn: parent
                            text: parent.modelData
                            font.pixelSize: 12
                            font.capitalization: Font.Capitalize
                            color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade200 : ThemeManager.colors.surface.shade800
                        }
                    }
                }
            }

            Text {
                text: "NanPressable — 纯交互原语"
                font.pixelSize: 18
                font.bold: true
                color: ThemeManager.colors.primary.shade600
            }

            Row {
                spacing: 16

                NanSurface {
                    id: _pressDemo
                    width: 160
                    height: 80
                    colorVariant: "primary"
                    backgroundShade: _pressable.pressed ? 400 : _pressable.hovered ? 100 : -1
                    borderShade: _pressable.hovered ? 400 : -1

                    Behavior on backgroundShade {
                        NumberAnimation {
                            duration: 80
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        text: _pressable.pressed ? "▼ Pressed" : _pressable.hovered ? "▲ Hovered" : "👆 Click me"
                        font.pixelSize: 13
                        color: ThemeManager.darkMode ? ThemeManager.colors.primary.shade200 : ThemeManager.colors.primary.shade700
                    }

                    NanPressable {
                        id: _pressable
                        anchors.fill: parent
                        onClicked: _clickCount.count++
                    }
                }

                NanSurface {
                    width: 160
                    height: 80
                    colorVariant: "surface"
                    opacity: 0.45

                    Text {
                        anchors.centerIn: parent
                        text: "🚫 Disabled"
                        font.pixelSize: 13
                        color: ThemeManager.colors.surface.shade500
                    }

                    NanPressable {
                        anchors.fill: parent
                        enabled: false
                    }
                }

                NanSurface {
                    id: _longPressCard
                    width: 160
                    height: 80
                    colorVariant: "tertiary"
                    property bool _triggered: false

                    Text {
                        anchors.centerIn: parent
                        text: _longPressCard._triggered ? "✔ Long pressed!" : "⏳ Hold me"
                        font.pixelSize: 13
                        color: ThemeManager.darkMode ? ThemeManager.colors.tertiary.shade200 : ThemeManager.colors.tertiary.shade700
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

            Text {
                text: "NanPanel — 主题容器 + 可选标题"
                font.pixelSize: 18
                font.bold: true
                color: ThemeManager.colors.primary.shade600
            }

            NanPanel {
                Layout.fillWidth: true
                Text {
                    text: "无标题 Panel — 只有内容区，无分割线"
                    font.pixelSize: 13
                    color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade200 : ThemeManager.colors.surface.shade700
                }
            }

            NanPanel {
                Layout.fillWidth: true
                title: "Panel Title"
                Text {
                    text: "带标题 Panel — 自动显示分割线，手动指定标题"
                    font.pixelSize: 13
                    color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade200 : ThemeManager.colors.surface.shade700
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: 12

                Repeater {
                    model: ["primary", "success", "warning", "error"]
                    delegate: NanPanel {
                        required property string modelData
                        width: 220
                        colorVariant: modelData
                        backgroundShade: ThemeManager.darkMode ? 900 : 50
                        borderShade: ThemeManager.darkMode ? 700 : 200
                        title: modelData.charAt(0).toUpperCase() + modelData.slice(1) + " Panel"

                        Text {
                            text: "colorVariant: \"" + parent.colorVariant + "\""
                            font.pixelSize: 12
                            color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade600
                        }
                    }
                }
            }

            Item {
                Layout.preferredHeight: 24
            }
        }
    }
}
