pragma ComponentBehavior: Bound

import QtQuick
import Nandina.Controls 1.0

Item {
    id: root

    property var themeManager: null
    property bool accepted: false

    Column {
        anchors.centerIn: parent
        spacing: 12

        Text {
            text: "NanCheckbox Demo"
            color: root.themeManager.currentPaletteCollection.mainHeadline
            font.pixelSize: 22
        }

        NanCheckbox {
            text: "我已阅读并同意条款"
            checked: root.accepted
            themeManager: root.themeManager
            onToggled: function(checked) {
                root.accepted = checked
            }
        }

        NanCheckbox {
            text: "禁用状态"
            checked: true
            disabled: true
            themeManager: root.themeManager
        }

        Text {
            text: "示例: NanCheckbox { text: \"同意条款\"; checked: agreed }"
            color: root.themeManager.currentPaletteCollection.subHeadlines0
            font.pixelSize: 12
        }
    }
}
