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
    readonly property var _presetTypes: ThemeVariant.PresetTypes || ({})

    readonly property int _colorSurface: _colorVariantTypes.Surface ?? 6
    readonly property int _colorPrimary: _colorVariantTypes.Primary ?? 0
    readonly property int _colorSecondary: _colorVariantTypes.Secondary ?? 1

    readonly property int _presetFilled: _presetTypes.Filled ?? 0
    readonly property int _presetTonal: _presetTypes.Tonal ?? 1
    readonly property int _presetOutlined: _presetTypes.Outlined ?? 2

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 14

            Text {
                text: "NanCard"
                font.pixelSize: 28
                font.bold: true
                color: ThemeManager.colors.primary.shade700
            }

            Text {
                text: "聚焦 Card 的结构分区、视觉预设与交互状态。"
                font.pixelSize: 13
                color: ThemeManager.colors.surface.shade600
            }

            Text {
                text: "支持 outlined / tonal / filled 三种预设，header / content / footer 分区，可选图片横幅与交互状态"
                font.pixelSize: 13
                color: ThemeManager.colors.surface.shade600
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Text {
                text: "Preset 对比（surface variant）"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade700
            }

            Flow {
                Layout.fillWidth: true
                spacing: 12

                NanCard {
                    width: 240
                    preset: root._presetOutlined
                    title: "Outlined Card"
                    description: "Near-transparent bg with subtle border — default preset"

                    Text {
                        text: "Body content area.\nAdd any items here."
                        font.pixelSize: 13
                        color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade600
                        lineHeight: 1.5
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                }

                NanCard {
                    width: 240
                    preset: root._presetTonal
                    colorVariant: root._colorPrimary
                    title: "Tonal Card"
                    description: "Lightly tinted background that adapts to dark mode"

                    Text {
                        text: "Tonal preset with primary variant."
                        font.pixelSize: 13
                        color: ThemeManager.darkMode ? ThemeManager.colors.primary.shade300 : ThemeManager.colors.primary.shade700
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                }

                NanCard {
                    width: 240
                    preset: root._presetFilled
                    colorVariant: root._colorPrimary
                    title: "Filled Card"
                    description: "Solid colour fill — white text for strong visual weight"

                    Text {
                        text: "Filled preset with high contrast white text."
                        font.pixelSize: 13
                        color: "#ffffff"
                        opacity: 0.85
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                }
            }

            Text {
                text: "interactive: true — 悬停 / 按压 / 点击"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade700
            }

            Flow {
                Layout.fillWidth: true
                spacing: 12

                Repeater {
                    model: [
                        {
                            label: "Outlined",
                            preset: root._presetOutlined,
                            variant: root._colorSurface,
                            variantLabel: "Surface"
                        },
                        {
                            label: "Tonal",
                            preset: root._presetTonal,
                            variant: root._colorPrimary,
                            variantLabel: "Primary"
                        },
                        {
                            label: "Filled",
                            preset: root._presetFilled,
                            variant: root._colorSecondary,
                            variantLabel: "Secondary"
                        }
                    ]
                    delegate: NanCard {
                        required property var modelData
                        width: 220
                        interactive: true
                        preset: modelData.preset
                        colorVariant: modelData.variant
                        title: modelData.label
                        description: "variant: " + modelData.variantLabel

                        Text {
                            text: parent.pressed ? "▼ Pressed" : parent.hovered ? "▲ Hovered" : "👆 Click me"
                            font.pixelSize: 13
                            color: parent.preset === root._presetFilled ? "#ffffff" : (ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade600)
                            width: parent.width
                            wrapMode: Text.WordWrap
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
