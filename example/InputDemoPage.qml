pragma ComponentBehavior: Bound

import QtQuick
import Nandina.Controls 1.0 as NC

Item {
    id: root

    property var themeManager: null
    property string validationLog: "等待输入..."

    function evenLengthValidator(value) {
        if (value.length === 0)
            return true;
        if (value.length % 2 === 0)
            return true;
        return "长度必须为偶数";
    }

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: contentColumn.implicitHeight + 28
        clip: true

        Column {
            id: contentColumn
            width: Math.min(620, parent.width - 28)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 12
            spacing: 14

            Text {
                text: "NanInput Demo"
                color: root.themeManager.currentPaletteCollection.mainHeadline
                font.pixelSize: 24
                font.weight: Font.DemiBold
            }

            Text {
                text: "支持类型校验、自定义校验函数、校验触发时机、清空按钮与密码显隐。"
                color: root.themeManager.currentPaletteCollection.bodyCopy
                font.pixelSize: 14
                wrapMode: Text.Wrap
            }

            Rectangle {
                width: parent.width
                radius: 10
                color: root.themeManager.currentPaletteCollection.secondaryPane
                border.width: 1
                border.color: root.themeManager.currentPaletteCollection.surfaceElement0
                implicitHeight: basicColumn.implicitHeight + 18

                Column {
                    id: basicColumn
                    anchors.fill: parent
                    anchors.margins: 9
                    spacing: 10

                    Text {
                        text: "基础与自动校验"
                        color: root.themeManager.currentPaletteCollection.mainHeadline
                        font.pixelSize: 16
                        font.weight: Font.Medium
                    }

                    NC.NanInput {
                        id: usernameInput
                        width: parent.width
                        placeholderText: "请输入用户名"
                        helperText: usernameInput.text.length === 0 ? "至少输入 3 个字符" : ""
                        invalid: usernameInput.text.length > 0 && usernameInput.text.length < 3
                        errorText: usernameInput.invalid ? "用户名长度不足" : ""
                        clearable: true
                        themeManager: root.themeManager
                    }

                    NC.NanInput {
                        width: parent.width
                        inputType: NC.NanInput.InputType.Email
                        validateTrigger: NC.NanInput.ValidateTrigger.OnBlur
                        clearable: true
                        placeholderText: "Email（失焦时校验）"
                        helperText: "示例：name@example.com"
                        themeManager: root.themeManager
                    }

                    NC.NanInput {
                        width: parent.width
                        inputType: NC.NanInput.InputType.Url
                        validateTrigger: NC.NanInput.ValidateTrigger.OnInput
                        clearable: true
                        placeholderText: "URL（输入时校验）"
                        helperText: "示例：https://www.example.com"
                        themeManager: root.themeManager
                    }

                    NC.NanInput {
                        width: parent.width
                        inputType: NC.NanInput.InputType.Number
                        validateTrigger: NC.NanInput.ValidateTrigger.OnInput
                        clearable: true
                        placeholderText: "Number（输入时校验）"
                        helperText: "支持负号与小数，例如 -12.5"
                        themeManager: root.themeManager
                    }
                }
            }

            Rectangle {
                width: parent.width
                radius: 10
                color: root.themeManager.currentPaletteCollection.secondaryPane
                border.width: 1
                border.color: root.themeManager.currentPaletteCollection.surfaceElement0
                implicitHeight: advancedColumn.implicitHeight + 18

                Column {
                    id: advancedColumn
                    anchors.fill: parent
                    anchors.margins: 9
                    spacing: 10

                    Text {
                        text: "高级用法"
                        color: root.themeManager.currentPaletteCollection.mainHeadline
                        font.pixelSize: 16
                        font.weight: Font.Medium
                    }

                    NC.NanInput {
                        width: parent.width
                        inputType: NC.NanInput.InputType.Password
                        validateTrigger: NC.NanInput.ValidateTrigger.OnSubmit
                        clearable: true
                        showPasswordToggle: true
                        placeholderText: "Password（回车时校验，可显隐）"
                        helperText: "点击右侧“显示/隐藏”切换明文"
                        themeManager: root.themeManager
                    }

                    NC.NanInput {
                        id: phoneInput
                        width: parent.width
                        inputType: NC.NanInput.InputType.Telephone
                        clearable: true
                        placeholderText: "Telephone（不做格式校验）"
                        helperText: "仅提供电话号码输入键盘提示"
                        themeManager: root.themeManager
                    }

                    NC.NanInput {
                        id: customValidatedInput
                        width: parent.width
                        style: NC.NanInput.Style.Underline
                        validateTrigger: NC.NanInput.ValidateTrigger.OnBlur
                        placeholderText: "自定义校验（长度需为偶数）"
                        useCustomValidator: true
                        validatorFn: root.evenLengthValidator
                        helperText: "当前展示下划线风格"
                        themeManager: root.themeManager
                        onValidationChanged: function (valid, message) {
                            root.validationLog = valid ? "自定义校验通过" : ("自定义校验失败: " + message);
                        }
                    }

                    NC.NanInput {
                        width: parent.width
                        placeholderText: "只读示例"
                        text: "readonly value"
                        readOnly: true
                        themeManager: root.themeManager
                    }

                    Text {
                        text: root.validationLog
                        color: root.themeManager.currentPaletteCollection.bodyCopy
                        font.pixelSize: 13
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
    }
}
