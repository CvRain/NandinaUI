// LabelPage.qml
// Demonstration page for NanLabel component
// Shows various states, variants, and form integrations

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
    readonly property var _sizeTypes: ThemeVariant.SizeTypes || ({})

    readonly property int _colorSurface: _colorVariantTypes.Surface ?? 6
    readonly property int _colorPrimary: _colorVariantTypes.Primary ?? 0
    readonly property int _colorError: _colorVariantTypes.Error ?? 5

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 24

            // ── Page header ────────────────────────────────────────
            Text {
                text: "NanLabel"
                font.pixelSize: 28
                font.bold: true
                color: ThemeManager.colors.primary.shade700
            }
            Text {
                text: root.routeSpec?.summary ?? "Accessible form labels with state support and theme integration"
                font.pixelSize: 13
                color: ThemeManager.colors.surface.shade600
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            // ── Basic usage ───────────────────────────────────────
            Text {
                text: "基本用法"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade700
            }

            ColumnLayout {
                spacing: 12
                Layout.fillWidth: true

                NanLabel {
                    text: "Email address"
                }

                NanLabel {
                    text: "Password"
                    forId: "passwordInput"
                }

                NanLabel {
                    text: "Confirm password"
                    forId: "confirmPasswordInput"
                }
            }

            // ── Required fields ───────────────────────────────────
            Text {
                text: "必填字段"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade700
            }

            ColumnLayout {
                spacing: 12
                Layout.fillWidth: true

                NanLabel {
                    text: "Username"
                    required: true
                }

                NanLabel {
                    text: "Email"
                    required: true
                    error: true
                }

                NanLabel {
                    text: "Phone number (optional)"
                    required: false
                }
            }

            // ── States ────────────────────────────────────────────
            Text {
                text: "状态"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade700
            }

            GridLayout {
                columns: 2
                rowSpacing: 12
                columnSpacing: 24
                Layout.fillWidth: true

                // Normal
                ColumnLayout {
                    spacing: 4
                    NanLabel {
                        text: "Normal state"
                    }
                    Text {
                        text: "默认状态"
                        font.pixelSize: 11
                        color: ThemeManager.colors.surface.shade500
                    }
                }

                // Disabled
                ColumnLayout {
                    spacing: 4
                    NanLabel {
                        text: "Disabled state"
                        disabled: true
                    }
                    Text {
                        text: "禁用状态"
                        font.pixelSize: 11
                        color: ThemeManager.colors.surface.shade500
                    }
                }

                // Error
                ColumnLayout {
                    spacing: 4
                    NanLabel {
                        text: "Error state"
                        error: true
                    }
                    Text {
                        text: "错误状态"
                        font.pixelSize: 11
                        color: ThemeManager.colors.surface.shade500
                    }
                }

                // Error + Required
                ColumnLayout {
                    spacing: 4
                    NanLabel {
                        text: "Required field"
                        required: true
                        error: true
                    }
                    Text {
                        text: "错误 + 必填"
                        font.pixelSize: 11
                        color: ThemeManager.colors.surface.shade500
                    }
                }
            }

            // ── With icons ────────────────────────────────────────
            Text {
                text: "带图标"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade700
            }

            ColumnLayout {
                spacing: 12
                Layout.fillWidth: true

                NanLabel {
                    text: "Website URL"
                    leftIcon: Component {
                        Text {
                            text: "🔗"
                            font.pixelSize: 14
                        }
                    }
                }

                NanLabel {
                    text: "Search"
                    leftIcon: Component {
                        Text {
                            text: "🔍"
                            font.pixelSize: 14
                        }
                    }
                }

                NanLabel {
                    text: "Download"
                    rightIcon: Component {
                        Text {
                            text: "⬇"
                            font.pixelSize: 14
                        }
                    }
                }

                NanLabel {
                    text: "External link"
                    leftIcon: Component {
                        Text {
                            text: "📎"
                            font.pixelSize: 14
                        }
                    }
                    rightIcon: Component {
                        Text {
                            text: "↗"
                            font.pixelSize: 14
                        }
                    }
                }
            }

            // ── Form integration demo ─────────────────────────────
            Text {
                text: "表单集成示例"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade700
            }

            Rectangle {
                Layout.fillWidth: true
                radius: ThemeManager.primitives.radiusBase
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade100 : ThemeManager.colors.surface.shade50
                border.color: ThemeManager.colors.surface.shade200
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 16

                    // Email field
                    ColumnLayout {
                        spacing: 6
                        Layout.fillWidth: true

                        NanLabel {
                            text: "Email address"
                            required: true
                            forId: "emailField"
                        }

                        TextField {
                            id: emailField
                            objectName: "emailField"
                            Layout.fillWidth: true
                            placeholderText: "you@example.com"
                            font.pixelSize: 13

                            // Simulate focus ring using NanTokens colors
                            focus: true
                        }

                        Text {
                            visible: emailField.text.length > 0 && !emailField.text.includes("@")
                            text: "Please enter a valid email address"
                            font.pixelSize: 11
                            color: ThemeManager.colors.error.shade600
                        }
                    }

                    // Password field
                    ColumnLayout {
                        spacing: 6
                        Layout.fillWidth: true

                        NanLabel {
                            text: "Password"
                            required: true
                            forId: "passwordField"
                        }

                        TextField {
                            id: passwordField
                            objectName: "passwordField"
                            Layout.fillWidth: true
                            echoMode: TextInput.Password
                            placeholderText: "Enter your password"
                            font.pixelSize: 13
                        }
                    }

                    // Confirm password field (error state)
                    ColumnLayout {
                        spacing: 6
                        Layout.fillWidth: true

                        NanLabel {
                            text: "Confirm password"
                            required: true
                            forId: "confirmPasswordField"
                            error: passwordField.text.length > 0 && confirmPasswordField.text !== passwordField.text
                        }

                        TextField {
                            id: confirmPasswordField
                            objectName: "confirmPasswordField"
                            Layout.fillWidth: true
                            echoMode: TextInput.Password
                            placeholderText: "Confirm your password"
                            font.pixelSize: 13
                        }

                        Text {
                            visible: passwordField.text.length > 0 && confirmPasswordField.text.length > 0 && confirmPasswordField.text !== passwordField.text
                            text: "Passwords do not match"
                            font.pixelSize: 11
                            color: ThemeManager.colors.error.shade600
                        }
                    }

                    // Remember me checkbox
                    RowLayout {
                        spacing: 8

                        CheckBox {
                            id: rememberCheckBox
                            text: "Remember me"
                            checked: true

                            // Custom styling to match theme
                            indicator: Rectangle {
                                implicitWidth: 18
                                implicitHeight: 18
                                radius: 4
                                color: "transparent"
                                border.color: rememberCheckBox.checked ? ThemeManager.colors.primary.shade500 : ThemeManager.colors.surface.shade400
                                border.width: 2

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 10
                                    height: 10
                                    radius: 2
                                    color: ThemeManager.colors.primary.shade500
                                    visible: rememberCheckBox.checked
                                }
                            }

                            contentItem: Text {
                                text: rememberCheckBox.text
                                font.pixelSize: 13
                                color: ThemeManager.colors.surface.shade700
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: 4
                            }
                        }
                    }
                }
            }
        }
    }
}
