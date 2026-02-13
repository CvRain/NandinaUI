pragma ComponentBehavior: Bound

import QtQuick
import Nandina.Controls 1.0

Item {
    id: root

    property var themeManager: null

    Column {
        anchors.centerIn: parent
        spacing: 14

        Text {
            text: "NanButton Demo"
            color: root.themeManager.currentPaletteCollection.mainHeadline
            font.pixelSize: 22
        }

        Row {
            spacing: 10

            NanButton {
                text: "Default"
                themeManager: root.themeManager
            }

            NanButton {
                text: "Outline"
                variant: NanButton.Outline
                themeManager: root.themeManager
            }

            NanButton {
                text: "Destructive"
                variant: NanButton.Destructive
                themeManager: root.themeManager
            }

            NanButton {
                text: "Disabled"
                disabled: true
                themeManager: root.themeManager
            }
        }

        Text {
            text: "示例: NanButton { text: \"Submit\"; variant: NanButton.Outline }"
            color: root.themeManager.currentPaletteCollection.subHeadlines0
            font.pixelSize: 12
        }
    }
}
