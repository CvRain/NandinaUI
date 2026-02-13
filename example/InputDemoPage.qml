pragma ComponentBehavior: Bound

import QtQuick
import Nandina.Controls 1.0

Item {
    id: root

    property var themeManager: null

    Column {
        id: contentColumn
        anchors.centerIn: parent
        width: 420
        spacing: 12

        Text {
            text: "NanInput Demo"
            color: root.themeManager.currentPaletteCollection.mainHeadline
            font.pixelSize: 22
        }

        NanInput {
            id: usernameInput
            width: contentColumn.width
            placeholderText: "请输入用户名"
            helperText: usernameInput.text.length === 0 ? "至少输入 3 个字符" : ""
            invalid: usernameInput.text.length > 0 && usernameInput.text.length < 3
            errorText: usernameInput.invalid ? "用户名长度不足" : ""
            themeManager: root.themeManager
        }

        NanInput {
            width: contentColumn.width
            placeholderText: "只读示例"
            text: "readonly value"
            readOnly: true
            themeManager: root.themeManager
        }

        Text {
            text: "示例: NanInput { placeholderText: \"Email\"; invalid: true; errorText: \"格式错误\" }"
            color: root.themeManager.currentPaletteCollection.subHeadlines0
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }
    }
}
