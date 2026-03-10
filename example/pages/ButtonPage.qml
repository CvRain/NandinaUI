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
                text: "Preset 对比（colorVariant: \"primary\"）"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade700
            }

            Flow {
                Layout.fillWidth: true
                spacing: 12

                NanButton {
                    text: "Filled"
                    preset: "filled"
                    colorVariant: "primary"
                }
                NanButton {
                    text: "Tonal"
                    preset: "tonal"
                    colorVariant: "primary"
                }
                NanButton {
                    text: "Outlined"
                    preset: "outlined"
                    colorVariant: "primary"
                }
                NanButton {
                    text: "Ghost"
                    preset: "ghost"
                    colorVariant: "primary"
                }
                NanButton {
                    text: "Link"
                    preset: "link"
                    colorVariant: "primary"
                }
            }

            // ── Color variant 对比 (filled preset) ────────────────
            Text {
                text: "Color Variant 对比（preset: \"filled\"）"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade700
            }

            Flow {
                Layout.fillWidth: true
                spacing: 12

                Repeater {
                    model: ["surface", "primary", "secondary", "tertiary", "success", "warning", "error"]
                    delegate: NanButton {
                        required property string modelData
                        preset: "filled"
                        colorVariant: modelData
                        text: modelData.charAt(0).toUpperCase() + modelData.slice(1)
                    }
                }
            }

            // ── Outlined across all variants ───────────────────────
            Text {
                text: "Color Variant 对比（preset: \"outlined\"）"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade700
            }

            Flow {
                Layout.fillWidth: true
                spacing: 12

                Repeater {
                    model: ["surface", "primary", "secondary", "tertiary", "success", "warning", "error"]
                    delegate: NanButton {
                        required property string modelData
                        preset: "outlined"
                        colorVariant: modelData
                        text: modelData.charAt(0).toUpperCase() + modelData.slice(1)
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
                    size: "sm"
                    preset: "filled"
                    colorVariant: "primary"
                }
                NanButton {
                    text: "Medium"
                    size: "md"
                    preset: "filled"
                    colorVariant: "primary"
                }
                NanButton {
                    text: "Large"
                    size: "lg"
                    preset: "filled"
                    colorVariant: "primary"
                }
            }

            Row {
                spacing: 12

                NanButton {
                    text: "Small"
                    size: "sm"
                    preset: "outlined"
                    colorVariant: "secondary"
                }
                NanButton {
                    text: "Medium"
                    size: "md"
                    preset: "outlined"
                    colorVariant: "secondary"
                }
                NanButton {
                    text: "Large"
                    size: "lg"
                    preset: "outlined"
                    colorVariant: "secondary"
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
                    preset: "filled"
                    colorVariant: "primary"
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
                    preset: "outlined"
                    colorVariant: "error"
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
                    preset: "tonal"
                    colorVariant: "tertiary"
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
                    preset: "ghost"
                    colorVariant: "surface"
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
                    model: ["filled", "tonal", "outlined", "ghost"]
                    delegate: NanButton {
                        id: _interactiveBtn
                        required property string modelData
                        preset: modelData
                        colorVariant: "primary"
                        text: modelData.charAt(0).toUpperCase() + modelData.slice(1)
                        onClicked: _clickLog.lastPreset = modelData
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
                    preset: "filled"
                    colorVariant: "primary"
                    enabled: false
                }
                NanButton {
                    text: "Tonal"
                    preset: "tonal"
                    colorVariant: "primary"
                    enabled: false
                }
                NanButton {
                    text: "Outlined"
                    preset: "outlined"
                    colorVariant: "primary"
                    enabled: false
                }
                NanButton {
                    text: "Ghost"
                    preset: "ghost"
                    colorVariant: "surface"
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
