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
    property var paletteValues: [NandinaColor.Latte, NandinaColor.Frappe, NandinaColor.Macchiato, NandinaColor.Mocha, NandinaColor.Custom]
    property var demoPages: [
        {
            title: "Window Demo",
            source: "WindowDemoPage.qml"
        },
        {
            title: "Button",
            source: "ButtonDemoPage.qml"
        },
        {
            title: "Input",
            source: "InputDemoPage.qml"
        },
        {
            title: "Label",
            source: "LabelDemoPage.qml"
        },
        {
            title: "Switch",
            source: "SwitchDemoPage.qml"
        },
        {
            title: "Checkbox",
            source: "CheckboxDemoPage.qml"
        }
    ]

    function pageProperties(index) {
        const props = {
            themeManager: demoWindow.themeManager
        };
        if (index === 0)
            props.hostWindow = demoWindow;
        return props;
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
                    color: index === demoWindow.currentDemoPage ? demoWindow.themeManager.currentPaletteCollection.overlay1 : "transparent"

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
                model: ["Latte", "Frappe", "Macchiato", "Mocha", "Custom"]
                currentIndex: 0
                onActivated: {
                    demoWindow.themeManager.currentPaletteType = demoWindow.paletteValues[currentIndex];
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
        demoWindow.themeManager.setCustomColorCollection(legacyColorCollection);
        demoWindow.themeManager.setCustomPaletteCollection(legacyPaletteCollection);
        pageStack.push(Qt.resolvedUrl(demoWindow.demoPages[demoWindow.currentDemoPage].source), demoWindow.pageProperties(demoWindow.currentDemoPage));
    }

    onCurrentDemoPageChanged: {
        if (pageStack.depth === 0)
            return;
        pageStack.replace(Qt.resolvedUrl(demoWindow.demoPages[demoWindow.currentDemoPage].source), demoWindow.pageProperties(demoWindow.currentDemoPage));
    }

    Connections {
        target: demoWindow.themeManager

        function onPaletteTypeChanged(type) {
            const idx = demoWindow.paletteValues.indexOf(type);
            if (idx >= 0)
                globalPaletteBox.currentIndex = idx;
        }
    }

    ColorCollection {
        id: legacyColorCollection
        rosewater: "#c9f6e8"
        flamingo: "#a9e8d3"
        pink: "#80ddbf"
        mauve: "#5ed1aa"
        red: "#d31b76"
        maroon: "#a80a5c"
        peach: "#eab312"
        yellow: "#f0ca55"
        green: "#85cc21"
        teal: "#4f46e5"
        sky: "#54c1f1"
        sapphire: "#01a5ea"
        blue: "#00a470"
        lavender: "#8a84ec"
        text: "#1f2741"
        subtext1: "#303b62"
        subtext0: "#39466e"
        overlay2: "#41507f"
        overlay1: "#495a90"
        overlay0: "#6876a2"
        surface2: "#8892b3"
        surface1: "#a6adc8"
        surface0: "#c6c9d7"
        base: "#e4e5ec"
        mantle: "#c6c9d7"
        crust: "#a6adc8"
    }

    PaletteCollection {
        id: legacyPaletteCollection
        backgroundPane: "#e4e5ec"
        secondaryPane: "#c6c9d7"
        surfaceElement0: "#a6adc8"
        surfaceElement1: "#8892b3"
        surfaceElement2: "#6876a2"
        overlay0: "#495a90"
        overlay1: "#41507f"
        overlay2: "#39466e"
        bodyCopy: "#1f2741"
        mainHeadline: "#1f2741"
        subHeadlines0: "#303b62"
        subHeadlines1: "#39466e"
        subtle: "#41507f"
        onAccent: "#e4e5ec"
        links: "#11ba81"
        success: "#85cc21"
        warning: "#eab312"
        error: "#d31b76"
        tags: "#01a5ea"
        selectionBackground: "#a6adc8"
        cursor: "#11ba81"
        cursorText: "#e4e5ec"
        activeBorder: "#11ba81"
        inactiveBorder: "#6876a2"
        bellBorder: "#eab312"
        color0: "#39466e"
        color1: "#d31b76"
        color2: "#85cc21"
        color3: "#eab312"
        color4: "#11ba81"
        color5: "#4f46e5"
        color6: "#01a5ea"
        color7: "#a6adc8"
        color8: "#495a90"
        color9: "#a80a5c"
        color10: "#85cc21"
        color11: "#f0ca55"
        color12: "#5ed1aa"
        color13: "#8a84ec"
        color14: "#54c1f1"
        color15: "#8892b3"
        color16: "#a9e8d3"
        color17: "#c9f6e8"
        mark1: "#11ba81"
        mark2: "#4f46e5"
        mark3: "#01a5ea"
        mark1Text: "#e4e5ec"
        mark2Text: "#e4e5ec"
        mark3Text: "#e4e5ec"
    }
}
