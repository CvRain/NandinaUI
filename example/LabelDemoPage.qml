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
            text: "NanLabel Demo"
            color: root.themeManager.currentPaletteCollection.mainHeadline
            font.pixelSize: 22
        }

        NanLabel {
            text: "用户名"
            required: true
            forControl: inputRef
            themeManager: root.themeManager
        }

        NanInput {
            id: inputRef
            width: contentColumn.width
            placeholderText: "点击上面的 Label 会聚焦这里"
            themeManager: root.themeManager
        }

        NanLabel {
            text: "禁用标签"
            disabled: true
            themeManager: root.themeManager
        }

        Text {
            text: "示例: NanLabel { text: \"用户名\"; required: true; forControl: userInput }"
            color: root.themeManager.currentPaletteCollection.subHeadlines0
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }
    }
}
