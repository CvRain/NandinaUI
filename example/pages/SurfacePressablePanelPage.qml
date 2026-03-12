pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nandina.Theme
import Nandina.Controls
import Nandina.Types

Item {
    id: root

    readonly property var _colorVariantTypes: ThemeVariant.ColorVariantTypes || ({})

    readonly property int _colorSurface: _colorVariantTypes.Surface ?? 6
    readonly property int _colorPrimary: _colorVariantTypes.Primary ?? 0
    readonly property int _colorSecondary: _colorVariantTypes.Secondary ?? 1
    readonly property int _colorTertiary: _colorVariantTypes.Tertiary ?? 2
    readonly property int _colorSuccess: _colorVariantTypes.Success ?? 3
    readonly property int _colorWarning: _colorVariantTypes.Warning ?? 4
    readonly property int _colorError: _colorVariantTypes.Error ?? 5

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
                text: "聚合展示基础容器与交互原语，便于观察状态反馈与组合方式。"
                font.pixelSize: 13
                color: ThemeManager.colors.surface.shade600
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
                    model: [
                        {
                            label: "Surface",
                            value: root._colorSurface
                        },
                        {
                            label: "Primary",
                            value: root._colorPrimary
                        },
                        {
                            label: "Secondary",
                            value: root._colorSecondary
                        },
                        {
                            label: "Tertiary",
                            value: root._colorTertiary
                        },
                        {
                            label: "Success",
                            value: root._colorSuccess
                        },
                        {
                            label: "Warning",
                            value: root._colorWarning
                        },
                        {
                            label: "Error",
                            value: root._colorError
                        }
                    ]
                    delegate: NanSurface {
                        required property var modelData
                        width: 110
                        height: 72
                        colorVariant: modelData.value

                        Text {
                            anchors.centerIn: parent
                            text: parent.modelData.label
                            font.pixelSize: 12
                            // dark: shade700 = original shade300 = bright, high contrast on dark container bg
                            color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade800
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
                    colorVariant: root._colorPrimary
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
                        color: ThemeManager.darkMode ? ThemeManager.colors.primary.shade700 : ThemeManager.colors.primary.shade700
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
                    colorVariant: root._colorSurface
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
                    colorVariant: root._colorTertiary
                    property bool _triggered: false

                    Text {
                        anchors.centerIn: parent
                        text: _longPressCard._triggered ? "✔ Long pressed!" : "⏳ Hold me"
                        font.pixelSize: 13
                        color: ThemeManager.darkMode ? ThemeManager.colors.tertiary.shade700 : ThemeManager.colors.tertiary.shade700
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
                    color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade700
                }
            }

            NanPanel {
                Layout.fillWidth: true
                title: "Panel Title"
                Text {
                    text: "带标题 Panel — 自动显示分割线，手动指定标题"
                    font.pixelSize: 13
                    color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade700
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: 12

                Repeater {
                    model: [
                        {
                            label: "Primary",
                            value: root._colorPrimary
                        },
                        {
                            label: "Success",
                            value: root._colorSuccess
                        },
                        {
                            label: "Warning",
                            value: root._colorWarning
                        },
                        {
                            label: "Error",
                            value: root._colorError
                        }
                    ]
                    delegate: NanPanel {
                        required property var modelData
                        width: 220
                        colorVariant: modelData.value
                        backgroundShade: ThemeManager.darkMode ? 100 : 50
                        borderShade: ThemeManager.darkMode ? 300 : 200
                        title: modelData.label + " Panel"

                        Text {
                            text: "colorVariant: " + parent.modelData.label
                            font.pixelSize: 12
                            color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade600
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
