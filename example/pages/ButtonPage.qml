pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nandina.Theme
import Nandina.Controls
import Nandina.Types

NanPage {
    id: root

    readonly property var _colorVariantTypes: ThemeVariant.ColorVariantTypes || ({})
    readonly property var _presetTypes: ThemeVariant.PresetTypes || ({})
    readonly property var _sizeTypes: ThemeVariant.SizeTypes || ({})

    readonly property int _colorSurface: _colorVariantTypes.Surface ?? 6
    readonly property int _colorPrimary: _colorVariantTypes.Primary ?? 0
    readonly property int _colorSecondary: _colorVariantTypes.Secondary ?? 1
    readonly property int _colorTertiary: _colorVariantTypes.Tertiary ?? 2
    readonly property int _colorSuccess: _colorVariantTypes.Success ?? 3
    readonly property int _colorWarning: _colorVariantTypes.Warning ?? 4
    readonly property int _colorError: _colorVariantTypes.Error ?? 5

    readonly property int _presetFilled: _presetTypes.Filled ?? 0
    readonly property int _presetTonal: _presetTypes.Tonal ?? 1
    readonly property int _presetOutlined: _presetTypes.Outlined ?? 2
    readonly property int _presetGhost: _presetTypes.Ghost ?? 3
    readonly property int _presetLink: _presetTypes.Link ?? 4

    readonly property int _sizeSm: _sizeTypes.Sm ?? 0
    readonly property int _sizeMd: _sizeTypes.Md ?? 1
    readonly property int _sizeLg: _sizeTypes.Lg ?? 2

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
                text: root.routeSpec?.summary ?? ""
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
                    preset: root._presetFilled
                    colorVariant: root._colorPrimary
                }
                NanButton {
                    text: "Tonal"
                    preset: root._presetTonal
                    colorVariant: root._colorPrimary
                }
                NanButton {
                    text: "Outlined"
                    preset: root._presetOutlined
                    colorVariant: root._colorPrimary
                }
                NanButton {
                    text: "Ghost"
                    preset: root._presetGhost
                    colorVariant: root._colorPrimary
                }
                NanButton {
                    text: "Link"
                    preset: root._presetLink
                    colorVariant: root._colorPrimary
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
                    delegate: NanButton {
                        required property var modelData
                        preset: root._presetFilled
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
                    delegate: NanButton {
                        required property var modelData
                        preset: root._presetOutlined
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
                    size: root._sizeSm
                    preset: root._presetFilled
                    colorVariant: root._colorPrimary
                }
                NanButton {
                    text: "Medium"
                    size: root._sizeMd
                    preset: root._presetFilled
                    colorVariant: root._colorPrimary
                }
                NanButton {
                    text: "Large"
                    size: root._sizeLg
                    preset: root._presetFilled
                    colorVariant: root._colorPrimary
                }
            }

            Row {
                spacing: 12

                NanButton {
                    text: "Small"
                    size: root._sizeSm
                    preset: root._presetOutlined
                    colorVariant: root._colorSecondary
                }
                NanButton {
                    text: "Medium"
                    size: root._sizeMd
                    preset: root._presetOutlined
                    colorVariant: root._colorSecondary
                }
                NanButton {
                    text: "Large"
                    size: root._sizeLg
                    preset: root._presetOutlined
                    colorVariant: root._colorSecondary
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
                    preset: root._presetFilled
                    colorVariant: root._colorPrimary
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
                    preset: root._presetOutlined
                    colorVariant: root._colorError
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
                    preset: root._presetTonal
                    colorVariant: root._colorTertiary
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
                    preset: root._presetGhost
                    colorVariant: root._colorSurface
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
                            value: root._presetFilled
                        },
                        {
                            label: "Tonal",
                            value: root._presetTonal
                        },
                        {
                            label: "Outlined",
                            value: root._presetOutlined
                        },
                        {
                            label: "Ghost",
                            value: root._presetGhost
                        }
                    ]
                    delegate: NanButton {
                        id: _interactiveBtn
                        required property var modelData
                        preset: modelData.value
                        colorVariant: root._colorPrimary
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
                    preset: root._presetFilled
                    colorVariant: root._colorPrimary
                    enabled: false
                }
                NanButton {
                    text: "Tonal"
                    preset: root._presetTonal
                    colorVariant: root._colorPrimary
                    enabled: false
                }
                NanButton {
                    text: "Outlined"
                    preset: root._presetOutlined
                    colorVariant: root._colorPrimary
                    enabled: false
                }
                NanButton {
                    text: "Ghost"
                    preset: root._presetGhost
                    colorVariant: root._colorSurface
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
