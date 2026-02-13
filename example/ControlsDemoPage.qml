pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC
import Nandina.Controls as NanControls

Item {
    id: root

    property var themeManager: null

    property bool acceptedTerms: false
    property bool receiveUpdates: true

    Column {
        anchors.centerIn: parent
        width: 420
        spacing: 14

        Text {
            text: "Nandina.Controls 基础组件演示"
            color: root.themeManager.currentPaletteCollection.mainHeadline
            font.pixelSize: 22
            font.weight: Font.Medium
        }

        NanControls.Label {
            text: "用户名"
            themeManager: root.themeManager
            required: true
            forControl: usernameInput
        }

        NanControls.Input {
            id: usernameInput
            width: parent.width
            placeholderText: "请输入用户名"
            helperText: text.length === 0 ? "至少输入 3 个字符" : ""
            invalid: text.length > 0 && text.length < 3
            errorText: invalid ? "用户名长度不足" : ""
            themeManager: root.themeManager
        }

        NanControls.Label {
            text: "操作"
            themeManager: root.themeManager
        }

        Row {
            spacing: 10

            NanControls.Button {
                text: "Primary"
                themeManager: root.themeManager
            }

            NanControls.Button {
                text: "Outline"
                variant: NanControls.Button.Outline
                themeManager: root.themeManager
            }

            NanControls.Button {
                text: "Destructive"
                variant: NanControls.Button.Destructive
                themeManager: root.themeManager
            }
        }

        Row {
            spacing: 16

            NanControls.Switch {
                checked: root.receiveUpdates
                themeManager: root.themeManager
                onToggled: function(checked) {
                    root.receiveUpdates = checked
                }
            }

            Text {
                text: root.receiveUpdates ? "接收更新通知" : "关闭更新通知"
                color: root.themeManager.currentPaletteCollection.bodyCopy
                verticalAlignment: Text.AlignVCenter
            }
        }

        NanControls.Checkbox {
            text: "我已阅读并同意相关条款"
            checked: root.acceptedTerms
            themeManager: root.themeManager
            onToggled: function(checked) {
                root.acceptedTerms = checked
            }
        }

        NanControls.Button {
            width: 160
            text: "提交"
            disabled: !root.acceptedTerms || usernameInput.text.length < 3
            themeManager: root.themeManager
        }
    }
}
