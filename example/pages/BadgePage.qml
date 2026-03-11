pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
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
            spacing: 24

            // ── Title ─────────────────────────────────────────────
            Text {
                text: "Badge"
                font.pixelSize: 28
                font.bold: true
                color: ThemeManager.colors.primary.shade700
            }
            Text {
                text: "小型内联标签，用于分类、状态标注或数字计数。无交互行为。"
                font.pixelSize: 13
                color: ThemeManager.colors.surface.shade600
            }

            // ── Divider ───────────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: ThemeManager.colors.surface.shade200
            }

            // ── Presets ───────────────────────────────────────────
            Text {
                text: "Presets"
                font.pixelSize: 16
                font.bold: true
                color: ThemeManager.colors.surface.shade800
            }
            Text {
                text: "同一 colorVariant 下四种视觉风格：filled / tonal / outlined / ghost"
                font.pixelSize: 12
                color: ThemeManager.colors.surface.shade500
            }
            Flow {
                Layout.fillWidth: true
                spacing: 8
                NanBadge {
                    text: "filled"
                    preset: ThemeVariant.PresetTypes.Filled
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                }
                NanBadge {
                    text: "tonal"
                    preset: ThemeVariant.PresetTypes.Tonal
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                }
                NanBadge {
                    text: "outlined"
                    preset: ThemeVariant.PresetTypes.Outlined
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                }
                NanBadge {
                    text: "ghost"
                    preset: ThemeVariant.PresetTypes.Ghost
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                }
            }

            // ── Color variants ─────────────────────────────────────
            Text {
                text: "Color Variants"
                font.pixelSize: 16
                font.bold: true
                color: ThemeManager.colors.surface.shade800
            }
            Text {
                text: "filled preset 下全部七种语义色族"
                font.pixelSize: 12
                color: ThemeManager.colors.surface.shade500
            }
            Flow {
                Layout.fillWidth: true
                spacing: 8
                NanBadge {
                    text: "primary"
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                }
                NanBadge {
                    text: "secondary"
                    colorVariant: ThemeVariant.ColorVariantTypes.Secondary
                }
                NanBadge {
                    text: "tertiary"
                    colorVariant: ThemeVariant.ColorVariantTypes.Tertiary
                }
                NanBadge {
                    text: "success"
                    colorVariant: ThemeVariant.ColorVariantTypes.Success
                }
                NanBadge {
                    text: "warning"
                    colorVariant: ThemeVariant.ColorVariantTypes.Warning
                }
                NanBadge {
                    text: "error"
                    colorVariant: ThemeVariant.ColorVariantTypes.Error
                }
                NanBadge {
                    text: "surface"
                    colorVariant: ThemeVariant.ColorVariantTypes.Surface
                    preset: ThemeVariant.PresetTypes.Tonal
                }
            }

            // ── Sizes ──────────────────────────────────────────────
            Text {
                text: "Sizes"
                font.pixelSize: 16
                font.bold: true
                color: ThemeManager.colors.surface.shade800
            }
            Flow {
                Layout.fillWidth: true
                spacing: 8
                layoutDirection: Qt.LeftToRight
                NanBadge {
                    text: "sm"
                    size: ThemeVariant.SizeTypes.Sm
                    colorVariant: ThemeVariant.ColorVariantTypes.Secondary
                }
                NanBadge {
                    text: "md"
                    size: ThemeVariant.SizeTypes.Md
                    colorVariant: ThemeVariant.ColorVariantTypes.Secondary
                }
                NanBadge {
                    text: "lg"
                    size: ThemeVariant.SizeTypes.Lg
                    colorVariant: ThemeVariant.ColorVariantTypes.Secondary
                }
            }

            // ── Dot indicators ─────────────────────────────────────
            Text {
                text: "Dot Indicators"
                font.pixelSize: 16
                font.bold: true
                color: ThemeManager.colors.surface.shade800
            }
            Text {
                text: "用于在线状态、通知圆点等场景"
                font.pixelSize: 12
                color: ThemeManager.colors.surface.shade500
            }
            Row {
                spacing: 20
                Repeater {
                    model: [
                        {
                            label: "Online",
                            variant: ThemeVariant.ColorVariantTypes.Success
                        },
                        {
                            label: "Idle",
                            variant: ThemeVariant.ColorVariantTypes.Warning
                        },
                        {
                            label: "Busy",
                            variant: ThemeVariant.ColorVariantTypes.Error
                        },
                        {
                            label: "Offline",
                            variant: ThemeVariant.ColorVariantTypes.Surface
                        },
                    ]
                    delegate: Row {
                        id: statusRow
                        required property var modelData
                        spacing: 6
                        NanBadge {
                            anchors.verticalCenter: parent.verticalCenter
                            dot: true
                            colorVariant: statusRow.modelData.variant
                            size: ThemeVariant.SizeTypes.Md
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: statusRow.modelData.label
                            font.pixelSize: 12
                            color: ThemeManager.colors.surface.shade600
                        }
                    }
                }
            }

            // ── With icon ──────────────────────────────────────────
            Text {
                text: "With Icon"
                font.pixelSize: 16
                font.bold: true
                color: ThemeManager.colors.surface.shade800
            }
            Flow {
                Layout.fillWidth: true
                spacing: 8

                NanBadge {
                    id: _adminBadge
                    text: "Admin"
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                    preset: ThemeVariant.PresetTypes.Filled
                    leftIcon: _starIconComponent
                    Component {
                        id: _starIconComponent
                        Text {
                            text: "★"
                            font.pixelSize: 9
                            color: _adminBadge.resolvedTextColor
                        }
                    }
                }

                NanBadge {
                    id: _newBadge
                    text: "New"
                    colorVariant: ThemeVariant.ColorVariantTypes.Secondary
                    preset: ThemeVariant.PresetTypes.Tonal
                    leftIcon: _sparkleComponent
                    Component {
                        id: _sparkleComponent
                        Text {
                            text: "✦"
                            font.pixelSize: 9
                            color: _newBadge.resolvedTextColor
                        }
                    }
                }

                NanBadge {
                    id: _betaBadge
                    text: "Beta"
                    colorVariant: ThemeVariant.ColorVariantTypes.Tertiary
                    preset: ThemeVariant.PresetTypes.Outlined
                    rightIcon: _betaIconComponent
                    Component {
                        id: _betaIconComponent
                        Text {
                            text: "⚡"
                            font.pixelSize: 9
                            color: _betaBadge.resolvedTextColor
                        }
                    }
                }
            }

            // ── Semantic usage ────────────────────────────────────
            Text {
                text: "Semantic Usage"
                font.pixelSize: 16
                font.bold: true
                color: ThemeManager.colors.surface.shade800
            }
            Text {
                text: "常见场景组合参考"
                font.pixelSize: 12
                color: ThemeManager.colors.surface.shade500
            }
            Flow {
                Layout.fillWidth: true
                spacing: 8
                NanBadge {
                    text: "Stable"
                    colorVariant: ThemeVariant.ColorVariantTypes.Success
                    preset: ThemeVariant.PresetTypes.Tonal
                }
                NanBadge {
                    text: "Deprecated"
                    colorVariant: ThemeVariant.ColorVariantTypes.Warning
                    preset: ThemeVariant.PresetTypes.Tonal
                }
                NanBadge {
                    text: "Removed"
                    colorVariant: ThemeVariant.ColorVariantTypes.Error
                    preset: ThemeVariant.PresetTypes.Tonal
                }
                NanBadge {
                    text: "v2.0"
                    colorVariant: ThemeVariant.ColorVariantTypes.Tertiary
                    preset: ThemeVariant.PresetTypes.Outlined
                }
                NanBadge {
                    text: "Community"
                    colorVariant: ThemeVariant.ColorVariantTypes.Secondary
                    preset: ThemeVariant.PresetTypes.Ghost
                }
                NanBadge {
                    text: "Pro"
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                }
            }

            Item {
                Layout.preferredHeight: 24
            }
        }
    }
}
