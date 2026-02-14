pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Nandina.Color

Item {
    id: root

    property var hostWindow: null
    property var themeManager: null
    property var paletteValues: [NandinaColor.Latte, NandinaColor.Frappe, NandinaColor.Macchiato, NandinaColor.Mocha, NandinaColor.Custom]

    Column {
        anchors.centerIn: parent
        spacing: 16

        Label {
            text: "NanWindow 标题栏模式演示"
            color: root.themeManager.currentPaletteCollection.mainHeadline
            font.pixelSize: 24
            horizontalAlignment: Text.AlignHCenter
            width: 320
        }

        ComboBox {
            id: modeBox
            width: 320
            model: ["DefaultTitleBar", "Frameless", "CustomTitleBar"]
            currentIndex: root.hostWindow.titleBarMode
            onActivated: root.hostWindow.titleBarMode = currentIndex
        }

        ComboBox {
            id: paletteBox
            width: 320
            model: ["Latte", "Frappe", "Macchiato", "Mocha", "Custom"]
            currentIndex: 0
            onActivated: {
                root.themeManager.currentPaletteType = root.paletteValues[currentIndex];
            }
        }

        Row {
            spacing: 16

            CheckBox {
                text: "Always On Top"
                checked: root.hostWindow.alwaysOnTop
                onToggled: root.hostWindow.alwaysOnTop = checked
            }

            CheckBox {
                text: "System Resize"
                checked: root.hostWindow.useSystemResize
                onToggled: root.hostWindow.useSystemResize = checked
            }
        }

        CheckBox {
            text: "Inject system controls in CustomTitleBar"
            checked: root.hostWindow.customTitleBarInjectSystemControls
            onToggled: root.hostWindow.customTitleBarInjectSystemControls = checked
        }

        Row {
            spacing: 8
            width: 320

            Label {
                text: "Radius"
                color: root.themeManager.currentPaletteCollection.bodyCopy
                width: 56
            }

            Slider {
                id: radiusSlider
                from: 0
                to: 24
                stepSize: 1
                value: root.hostWindow.windowRadius
                width: 210
                onMoved: root.hostWindow.windowRadius = value
            }

            Label {
                text: Math.round(radiusSlider.value).toString()
                color: root.themeManager.currentPaletteCollection.bodyCopy
                width: 30
            }
        }

        Rectangle {
            width: 320
            height: 120
            radius: 10
            color: root.themeManager.currentPaletteCollection.secondaryPane

            Text {
                anchors.centerIn: parent
                text: "当前模式: " + modeBox.currentText + "\n当前主题: " + paletteBox.currentText
                color: root.themeManager.currentPaletteCollection.bodyCopy
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    Component.onCompleted: {
        if (!root.themeManager)
            return;
        const idx = root.paletteValues.indexOf(root.themeManager.currentPaletteType);
        if (idx >= 0)
            paletteBox.currentIndex = idx;
    }

    Connections {
        target: root.themeManager

        function onPaletteTypeChanged(type) {
            const idx = root.paletteValues.indexOf(type);
            if (idx >= 0)
                paletteBox.currentIndex = idx;
        }
    }
}
