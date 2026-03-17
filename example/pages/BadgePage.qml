pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Nandina.Theme
import Nandina.Controls
import Nandina.Types

NanPage {
    id: root

    readonly property var _colorVariantTypes: ThemeVariant.ColorVariantTypes || ({})
    readonly property var _presetTypes: ThemeVariant.PresetTypes || ({})
    readonly property var _sizeTypes: ThemeVariant.SizeTypes || ({})

    readonly property int _colorPrimary: _colorVariantTypes.Primary ?? 0
    readonly property int _colorSecondary: _colorVariantTypes.Secondary ?? 1
    readonly property int _colorTertiary: _colorVariantTypes.Tertiary ?? 2
    readonly property int _colorSuccess: _colorVariantTypes.Success ?? 3
    readonly property int _colorWarning: _colorVariantTypes.Warning ?? 4
    readonly property int _colorError: _colorVariantTypes.Error ?? 5
    readonly property int _colorSurface: _colorVariantTypes.Surface ?? 6

    readonly property int _presetFilled: _presetTypes.Filled ?? 0
    readonly property int _presetTonal: _presetTypes.Tonal ?? 1
    readonly property int _presetOutlined: _presetTypes.Outlined ?? 2
    readonly property int _presetGhost: _presetTypes.Ghost ?? 3

    readonly property int _sizeSm: _sizeTypes.Sm ?? 0
    readonly property int _sizeMd: _sizeTypes.Md ?? 1
    readonly property int _sizeLg: _sizeTypes.Lg ?? 2

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 24

            // ── Title ─────────────────────────────────────────────
            Text {
                text: root.routeSpec?.navTitle ?? ""
                font.pixelSize: 28
                font.bold: true
                color: ThemeManager.colors.primary.shade700
            }
            Text {
                text: root.routeSpec?.summary ?? ""
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
                    preset: root._presetFilled
                    colorVariant: root._colorPrimary
                }
                NanBadge {
                    text: "tonal"
                    preset: root._presetTonal
                    colorVariant: root._colorPrimary
                }
                NanBadge {
                    text: "outlined"
                    preset: root._presetOutlined
                    colorVariant: root._colorPrimary
                }
                NanBadge {
                    text: "ghost"
                    preset: root._presetGhost
                    colorVariant: root._colorPrimary
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
                    colorVariant: root._colorPrimary
                }
                NanBadge {
                    text: "secondary"
                    colorVariant: root._colorSecondary
                }
                NanBadge {
                    text: "tertiary"
                    colorVariant: root._colorTertiary
                }
                NanBadge {
                    text: "success"
                    colorVariant: root._colorSuccess
                }
                NanBadge {
                    text: "warning"
                    colorVariant: root._colorWarning
                }
                NanBadge {
                    text: "error"
                    colorVariant: root._colorError
                }
                NanBadge {
                    text: "surface"
                    colorVariant: root._colorSurface
                    preset: root._presetTonal
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
                    size: root._sizeSm
                    colorVariant: root._colorSecondary
                }
                NanBadge {
                    text: "md"
                    size: root._sizeMd
                    colorVariant: root._colorSecondary
                }
                NanBadge {
                    text: "lg"
                    size: root._sizeLg
                    colorVariant: root._colorSecondary
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
                            variant: root._colorSuccess
                        },
                        {
                            label: "Idle",
                            variant: root._colorWarning
                        },
                        {
                            label: "Busy",
                            variant: root._colorError
                        },
                        {
                            label: "Offline",
                            variant: root._colorSurface
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
                            size: root._sizeMd
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
                    colorVariant: root._colorPrimary
                    preset: root._presetFilled
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
                    colorVariant: root._colorSecondary
                    preset: root._presetTonal
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
                    colorVariant: root._colorTertiary
                    preset: root._presetOutlined
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
                    colorVariant: root._colorSuccess
                    preset: root._presetTonal
                }
                NanBadge {
                    text: "Deprecated"
                    colorVariant: root._colorWarning
                    preset: root._presetTonal
                }
                NanBadge {
                    text: "Removed"
                    colorVariant: root._colorError
                    preset: root._presetTonal
                }
                NanBadge {
                    text: "v2.0"
                    colorVariant: root._colorTertiary
                    preset: root._presetOutlined
                }
                NanBadge {
                    text: "Community"
                    colorVariant: root._colorSecondary
                    preset: root._presetGhost
                }
                NanBadge {
                    text: "Pro"
                    colorVariant: root._colorPrimary
                }
            }

            Item {
                Layout.preferredHeight: 24
            }
        }
    }
}
