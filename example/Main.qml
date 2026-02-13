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

    property int currentDemoPage: 0
    property var demoPages: [
        { title: "Window Demo", source: "WindowDemoPage.qml" },
        { title: "Button", source: "ButtonDemoPage.qml" },
        { title: "Input", source: "InputDemoPage.qml" },
        { title: "Label", source: "LabelDemoPage.qml" },
        { title: "Switch", source: "SwitchDemoPage.qml" },
        { title: "Checkbox", source: "CheckboxDemoPage.qml" }
    ]

    function pageProperties(index) {
        const props = {
            themeManager: demoWindow.themeManager
        }
        if (index === 0)
            props.hostWindow = demoWindow
        return props
    }

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

    Item {
        anchors.fill: parent
        anchors.margins: 18

        Rectangle {
            id: sidebar
            width: 180
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            radius: 10
            color: demoWindow.themeManager.currentPaletteCollection.secondaryPane
            border.width: 1
            border.color: demoWindow.themeManager.currentPaletteCollection.surfaceElement0

            ListView {
                id: menuList
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                spacing: 6
                model: demoWindow.demoPages

                delegate: Rectangle {
                    id: menuItem
                    required property int index
                    required property var modelData

                    width: menuList.width
                    height: 36
                    radius: 8
                    color: index === demoWindow.currentDemoPage
                           ? demoWindow.themeManager.currentPaletteCollection.overlay1
                           : "transparent"

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        text: menuItem.modelData.title
                        color: demoWindow.themeManager.currentPaletteCollection.bodyCopy
                        font.pixelSize: 13
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: demoWindow.currentDemoPage = menuItem.index
                    }
                }
            }
        }

        Item {
            anchors.left: sidebar.right
            anchors.leftMargin: 12
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            ComboBox {
                id: globalPaletteBox
                width: 220
                model: ["Latte", "Frappe", "Macchiato", "Mocha"]
                currentIndex: 0
                onActivated: {
                    const values = [NandinaColor.Latte, NandinaColor.Frappe, NandinaColor.Macchiato, NandinaColor.Mocha]
                    demoWindow.themeManager.currentPaletteType = values[currentIndex]
                }
            }

            StackView {
                id: pageStack
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: globalPaletteBox.bottom
                anchors.bottom: parent.bottom
                anchors.topMargin: 10
            }
        }
    }

    Component.onCompleted: {
        pageStack.push(Qt.resolvedUrl(demoWindow.demoPages[demoWindow.currentDemoPage].source),
                       demoWindow.pageProperties(demoWindow.currentDemoPage))
    }

    onCurrentDemoPageChanged: {
        if (pageStack.depth === 0)
            return
        pageStack.replace(Qt.resolvedUrl(demoWindow.demoPages[demoWindow.currentDemoPage].source),
                          demoWindow.pageProperties(demoWindow.currentDemoPage))
    }
}
