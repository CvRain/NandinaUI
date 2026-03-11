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
            spacing: 16

            // ── Page header ────────────────────────────────────────
            Text {
                text: "NanButton"
                font.pixelSize: 28
                font.bold: true
                color: ThemeManager.colors.primary.shade700
            }
            Text {
                text: "五种预设 × 七种色彩变体 × 三种尺寸，支持左右图标与键盘交互。"
                font.pixelSize: 13
                color: ThemeManager.colors.surface.shade600
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            // ── Preset 对比 (primary variant) ─────────────────────
            Text {
                text: "Preset 对比（colorVariant: ThemeVariant.ColorVariantTypes.Primary）"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade700
            }

            Flow {
                Layout.fillWidth: true
                spacing: 12

                NanButton {
                    text: "Filled"
                    preset: ThemeVariant.PresetTypes.Filled
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                }
                NanButton {
                    text: "Tonal"
                    preset: ThemeVariant.PresetTypes.Tonal
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                }
                NanButton {
                    text: "Outlined"
                    preset: ThemeVariant.PresetTypes.Outlined
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                }
                NanButton {
                    text: "Ghost"
                    preset: ThemeVariant.PresetTypes.Ghost
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                }
                NanButton {
                    text: "Link"
                    preset: ThemeVariant.PresetTypes.Link
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                }
            }

            // ── Color variant 对比 (filled preset) ────────────────
            Text {
                text: "Color Variant 对比（preset: ThemeVariant.PresetTypes.Filled）"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade700
            }

            Flow {
                Layout.fillWidth: true
                spacing: 12

                Repeater {
                    model: [
                        {
                            label: "Surface",
                            value: ThemeVariant.ColorVariantTypes.Surface
                        },
                        {
                            label: "Primary",
                            value: ThemeVariant.ColorVariantTypes.Primary
                        },
                        {
                            label: "Secondary",
                            value: ThemeVariant.ColorVariantTypes.Secondary
                        },
                        {
                            label: "Tertiary",
                            value: ThemeVariant.ColorVariantTypes.Tertiary
                        },
                        {
                            label: "Success",
                            value: ThemeVariant.ColorVariantTypes.Success
                        },
                        {
                            label: "Warning",
                            value: ThemeVariant.ColorVariantTypes.Warning
                        },
                        {
                            label: "Error",
                            value: ThemeVariant.ColorVariantTypes.Error
                        }
                    ]
                    delegate: NanButton {
                        required property var modelData
                        preset: ThemeVariant.PresetTypes.Filled
                        colorVariant: modelData.value
                        text: modelData.label
                    }
                }
            }

            // ── Outlined across all variants ───────────────────────
            Text {
                text: "Color Variant 对比（preset: ThemeVariant.PresetTypes.Outlined）"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade700
            }

            Flow {
                Layout.fillWidth: true
                spacing: 12

                Repeater {
                    model: [
                        {
                            label: "Surface",
                            value: ThemeVariant.ColorVariantTypes.Surface
                        },
                        {
                            label: "Primary",
                            value: ThemeVariant.ColorVariantTypes.Primary
                        },
                        {
                            label: "Secondary",
                            value: ThemeVariant.ColorVariantTypes.Secondary
                        },
                        {
                            label: "Tertiary",
                            value: ThemeVariant.ColorVariantTypes.Tertiary
                        },
                        {
                            label: "Success",
                            value: ThemeVariant.ColorVariantTypes.Success
                        },
                        {
                            label: "Warning",
                            value: ThemeVariant.ColorVariantTypes.Warning
                        },
                        {
                            label: "Error",
                            value: ThemeVariant.ColorVariantTypes.Error
                        }
                    ]
                    delegate: NanButton {
                        required property var modelData
                        preset: ThemeVariant.PresetTypes.Outlined
                        colorVariant: modelData.value
                        text: modelData.label
                    }
                }
            }

            // ── Size 对比 ──────────────────────────────────────────
            Text {
                text: "Size 对比"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade700
            }

            Row {
                spacing: 12

                NanButton {
                    text: "Small"
                    size: ThemeVariant.SizeTypes.Sm
                    preset: ThemeVariant.PresetTypes.Filled
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                }
                NanButton {
                    text: "Medium"
                    size: ThemeVariant.SizeTypes.Md
                    preset: ThemeVariant.PresetTypes.Filled
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                }
                NanButton {
                    text: "Large"
                    size: ThemeVariant.SizeTypes.Lg
                    preset: ThemeVariant.PresetTypes.Filled
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                }
            }

            Row {
                spacing: 12

                NanButton {
                    text: "Small"
                    size: ThemeVariant.SizeTypes.Sm
                    preset: ThemeVariant.PresetTypes.Outlined
                    colorVariant: ThemeVariant.ColorVariantTypes.Secondary
                }
                NanButton {
                    text: "Medium"
                    size: ThemeVariant.SizeTypes.Md
                    preset: ThemeVariant.PresetTypes.Outlined
                    colorVariant: ThemeVariant.ColorVariantTypes.Secondary
                }
                NanButton {
                    text: "Large"
                    size: ThemeVariant.SizeTypes.Lg
                    preset: ThemeVariant.PresetTypes.Outlined
                    colorVariant: ThemeVariant.ColorVariantTypes.Secondary
                }
            }

            // ── 带图标 ─────────────────────────────────────────────
            Text {
                text: "左 / 右图标"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade700
            }

            Flow {
                Layout.fillWidth: true
                spacing: 12

                NanButton {
                    id: _uploadBtn
                    text: "Upload"
                    preset: ThemeVariant.PresetTypes.Filled
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                    leftIcon: Component {
                        Text {
                            text: "↑"
                            font.pixelSize: 14
                            font.bold: true
                            color: _uploadBtn.resolvedTextColor
                        }
                    }
                }

                NanButton {
                    id: _deleteBtn
                    text: "Delete"
                    preset: ThemeVariant.PresetTypes.Outlined
                    colorVariant: ThemeVariant.ColorVariantTypes.Error
                    leftIcon: Component {
                        Text {
                            text: "✕"
                            font.pixelSize: 12
                            color: _deleteBtn.resolvedTextColor
                        }
                    }
                }

                NanButton {
                    id: _nextBtn
                    text: "Next"
                    preset: ThemeVariant.PresetTypes.Tonal
                    colorVariant: ThemeVariant.ColorVariantTypes.Tertiary
                    rightIcon: Component {
                        Text {
                            text: "→"
                            font.pixelSize: 14
                            color: _nextBtn.resolvedTextColor
                        }
                    }
                }

                NanButton {
                    id: _settingsBtn
                    text: "Settings"
                    preset: ThemeVariant.PresetTypes.Ghost
                    colorVariant: ThemeVariant.ColorVariantTypes.Surface
                    leftIcon: Component {
                        Text {
                            text: "⚙"
                            font.pixelSize: 13
                            color: _settingsBtn.resolvedTextColor
                        }
                    }
                }
            }

            // ── 交互状态演示 ───────────────────────────────────────
            Text {
                text: "交互状态（hover / press）"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade700
            }

            Text {
                text: "将鼠标悬停至按钮可观察缩放与颜色变化，点击触发回弹动画。"
                font.pixelSize: 12
                color: ThemeManager.colors.surface.shade600
            }

            Row {
                spacing: 12

                Repeater {
                    model: [
                        {
                            label: "Filled",
                            value: ThemeVariant.PresetTypes.Filled
                        },
                        {
                            label: "Tonal",
                            value: ThemeVariant.PresetTypes.Tonal
                        },
                        {
                            label: "Outlined",
                            value: ThemeVariant.PresetTypes.Outlined
                        },
                        {
                            label: "Ghost",
                            value: ThemeVariant.PresetTypes.Ghost
                        }
                    ]
                    delegate: NanButton {
                        id: _interactiveBtn
                        required property var modelData
                        preset: modelData.value
                        colorVariant: ThemeVariant.ColorVariantTypes.Primary
                        text: modelData.label
                        onClicked: _clickLog.lastPreset = modelData.label
                    }
                }
            }

            Text {
                id: _clickLog
                property string lastPreset: ""
                text: lastPreset === "" ? "— 点击上方按钮 —" : "✔ 点击了: " + lastPreset
                font.pixelSize: 13
                color: ThemeManager.colors.primary.shade600
            }

            // ── 禁用状态 ───────────────────────────────────────────
            Text {
                text: "禁用状态（enabled: false）"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade700
            }

            Flow {
                Layout.fillWidth: true
                spacing: 12

                NanButton {
                    text: "Filled"
                    preset: ThemeVariant.PresetTypes.Filled
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                    enabled: false
                }
                NanButton {
                    text: "Tonal"
                    preset: ThemeVariant.PresetTypes.Tonal
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                    enabled: false
                }
                NanButton {
                    text: "Outlined"
                    preset: ThemeVariant.PresetTypes.Outlined
                    colorVariant: ThemeVariant.ColorVariantTypes.Primary
                    enabled: false
                }
                NanButton {
                    text: "Ghost"
                    preset: ThemeVariant.PresetTypes.Ghost
                    colorVariant: ThemeVariant.ColorVariantTypes.Surface
                    enabled: false
                }
            }

            // ── 键盘导航提示 ───────────────────────────────────────
            Text {
                text: "键盘导航：Tab 聚焦，Enter / Space 触发点击，焦点时显示聚焦环。"
                font.pixelSize: 12
                color: ThemeManager.colors.surface.shade600
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Item {
                Layout.preferredHeight: 24
            }
        }
    }
}
