pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Nandina.Color
import Nandina.Window

NanWindow {
    id: demoWindow

    width: 900
    height: 600
    visible: true
    windowTitle: "NanWindow Demo"
    titleBarMode: NanWindow.CustomTitleBar
    windowRadius: 12
    customTitleBarInjectSystemControls: false

    customTitleBar: Component {
        CornerRectangle {
            fillColor: demoWindow.themeManager.currentPaletteCollection.secondaryPane
            topLeftRadius: demoWindow.effectiveWindowRadius
            topRightRadius: demoWindow.effectiveWindowRadius
            bottomLeftRadius: 0
            bottomRightRadius: 0

            Row {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 12
                spacing: 10

                Rectangle {
                    width: 10
                    height: 10
                    radius: 5
                    color: demoWindow.themeManager.currentPaletteCollection.success
                }

                Text {
                    text: demoWindow.windowTitle
                    color: demoWindow.themeManager.currentPaletteCollection.bodyCopy
                    font.pixelSize: 14
                }
            }

            Row {
                visible: !demoWindow.customTitleBarInjectSystemControls
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 10
                spacing: 2

                TitleBarButton {
                    text: "—"
                    textColor: demoWindow.themeManager.currentPaletteCollection.bodyCopy
                    hoverColor: demoWindow.themeManager.currentPaletteCollection.overlay0
                    pressedColor: demoWindow.themeManager.currentPaletteCollection.overlay1
                    useAccentForHover: true
                    accentColor: demoWindow.themeManager.currentPaletteCollection.activeBorder
                    onClicked: demoWindow.showMinimized()
                }

                TitleBarButton {
                    text: demoWindow.visibility === Window.Maximized ? "❐" : "□"
                    textColor: demoWindow.themeManager.currentPaletteCollection.bodyCopy
                    hoverColor: demoWindow.themeManager.currentPaletteCollection.overlay0
                    pressedColor: demoWindow.themeManager.currentPaletteCollection.overlay1
                    useAccentForHover: true
                    accentColor: demoWindow.themeManager.currentPaletteCollection.activeBorder
                    onClicked: {
                        if (demoWindow.visibility === Window.Maximized)
                            demoWindow.showNormal();
                        else
                            demoWindow.showMaximized();
                    }
                }

                TitleBarButton {
                    text: "✕"
                    isCloseButton: true
                    onClicked: demoWindow.close()
                }
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                z: -1
                onPressed: {
                    if (demoWindow.visibility !== Window.FullScreen && demoWindow.visibility !== Window.Maximized)
                        demoWindow.startSystemMove();
                }
                onDoubleClicked: {
                    if (demoWindow.visibility === Window.Maximized)
                        demoWindow.showNormal();
                    else
                        demoWindow.showMaximized();
                }
            }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 16

        Label {
            text: "NanWindow 标题栏模式演示"
            color: demoWindow.themeManager.currentPaletteCollection.mainHeadline
            font.pixelSize: 24
            horizontalAlignment: Text.AlignHCenter
            width: 320
        }

        ComboBox {
            id: modeBox
            width: 320
            model: ["DefaultTitleBar", "Frameless", "CustomTitleBar"]
            currentIndex: demoWindow.titleBarMode
            onActivated: demoWindow.titleBarMode = currentIndex
        }

        ComboBox {
            id: paletteBox
            width: 320
            model: ["Latte", "Frappe", "Macchiato", "Mocha"]
            currentIndex: 0
            onActivated: {
                const values = [NandinaColor.Latte, NandinaColor.Frappe, NandinaColor.Macchiato, NandinaColor.Mocha];
                demoWindow.themeManager.currentPaletteType = values[currentIndex];
            }
        }

        Row {
            spacing: 16

            CheckBox {
                text: "Always On Top"
                checked: demoWindow.alwaysOnTop
                onToggled: demoWindow.alwaysOnTop = checked
            }

            CheckBox {
                text: "System Resize"
                checked: demoWindow.useSystemResize
                onToggled: demoWindow.useSystemResize = checked
            }
        }

        CheckBox {
            text: "Inject system controls in CustomTitleBar"
            checked: demoWindow.customTitleBarInjectSystemControls
            onToggled: demoWindow.customTitleBarInjectSystemControls = checked
        }

        Row {
            spacing: 8
            width: 320

            Label {
                text: "Radius"
                color: demoWindow.themeManager.currentPaletteCollection.bodyCopy
                width: 56
            }

            Slider {
                id: radiusSlider
                from: 0
                to: 24
                stepSize: 1
                value: demoWindow.windowRadius
                width: 210
                onMoved: demoWindow.windowRadius = value
            }

            Label {
                text: Math.round(radiusSlider.value).toString()
                color: demoWindow.themeManager.currentPaletteCollection.bodyCopy
                width: 30
            }
        }

        Rectangle {
            width: 320
            height: 120
            radius: 10
            color: demoWindow.themeManager.currentPaletteCollection.secondaryPane

            Text {
                anchors.centerIn: parent
                text: "当前模式: " + modeBox.currentText + "\n当前主题: " + paletteBox.currentText
                color: demoWindow.themeManager.currentPaletteCollection.bodyCopy
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
