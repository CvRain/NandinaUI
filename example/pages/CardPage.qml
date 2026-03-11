pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nandina.Theme
import Nandina.Controls
import Nandina.Types

Item {
    id: root

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
                    preset: ThemeVariant.PresetTypes.Outlined
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
                    preset: ThemeVariant.PresetTypes.Tonal
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
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
                    preset: ThemeVariant.PresetTypes.Filled
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
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
                            preset: ThemeVariant.PresetTypes.Outlined,
                            variant: ThemeVariant.ColorVariantTypes.Surface,
                            variantLabel: "Surface"
                        },
                        {
                            label: "Tonal",
                            preset: ThemeVariant.PresetTypes.Tonal,
                            variant: ThemeVariant.ColorVariantTypes.Primary,
                            variantLabel: "Primary"
                        },
                        {
                            label: "Filled",
                            preset: ThemeVariant.PresetTypes.Filled,
                            variant: ThemeVariant.ColorVariantTypes.Secondary,
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
                            color: parent.preset === ThemeVariant.PresetTypes.Filled ? "#ffffff" : (ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade600)
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
