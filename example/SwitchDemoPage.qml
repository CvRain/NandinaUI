pragma ComponentBehavior: Bound

import QtQuick
import Nandina.Controls 1.0

Item {
    id: root

    property var themeManager: null
    property bool enabledSync: true

    Column {
        anchors.centerIn: parent
        spacing: 12

        Text {
            text: "NanSwitch Demo"
            color: root.themeManager.currentPaletteCollection.mainHeadline
            font.pixelSize: 22
        }

        Row {
            spacing: 10

            NanSwitch {
                checked: root.enabledSync
                themeManager: root.themeManager
                onToggled: function(checked) {
                    root.enabledSync = checked
                }
            }

            Text {
                text: root.enabledSync ? "已开启同步" : "已关闭同步"
                color: root.themeManager.currentPaletteCollection.bodyCopy
                verticalAlignment: Text.AlignVCenter
            }
        }

        NanSwitch {
            checked: false
            disabled: true
            themeManager: root.themeManager
        }

        Text {
            text: "示例: NanSwitch { checked: true; onToggled: (v) => console.log(v) }"
            color: root.themeManager.currentPaletteCollection.subHeadlines0
            font.pixelSize: 12
        }
    }
}
