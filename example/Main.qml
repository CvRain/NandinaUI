pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Nandina.Color
import Nandina.Window
import Nandina.Controls

NanWindow {
    id: demoWindow

    width: 900
    height: 600
    visible: true
    windowTitle: "NanWindow Demo"
    titleBarMode: NanWindow.CustomTitleBar
    themeManager.customColorCollection: customTheme.colorCollection
    themeManager.customPaletteCollection: customTheme.paletteCollection
    property int currentSide: NanSideBar.Side.Left

    Button {
        text: "Switch Theme"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20

        property var colors: [NandinaColor.Mocha, NandinaColor.Macchiato, NandinaColor.Frappe, NandinaColor.Latte, NandinaColor.Custom]
        property int colorIndex: 0

        onClicked: {
            colorIndex = (colorIndex + 1) % colors.length;
            demoWindow.themeManager.setCurrentPaletteType(colors[colorIndex]);
        }
    }

    Button {
        text: demoWindow.currentSide === NanSideBar.Side.Left ? "Dock Right" : "Dock Left"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 60
        onClicked: {
            demoWindow.currentSide = demoWindow.currentSide === NanSideBar.Side.Left ? NanSideBar.Side.Right : NanSideBar.Side.Left;
        }
    }

    Text {
        anchors.centerIn: parent
        text: "NanWindow Demo"
        font.pointSize: 20
        font.bold: true
        color: demoWindow.themeManager.currentColorCollection.rosewater
    }

    NanSideBar {
        id: appSidebar
        dockingParent: parent
        side: demoWindow.currentSide
        collapsible: NanSideBar.Icon
        open: true
        expandedWidth: 200
        collapsedWidth: 72
        borderRadius: 16
        sectionPadding: 10
        contentSpacing: 8
        showDefaultTrigger: false
        themeManager: demoWindow.themeManager

        header: Component {
            Row {
                width: parent ? parent.width : 220
                height: 34
                spacing: 8

                NanSideBarTrigger {}

                Rectangle {
                    width: appSidebar.collapsed ? 0 : (parent.width - 40)
                    height: 32
                    radius: 9
                    color: demoWindow.themeManager.currentPaletteCollection.surfaceElement0
                    clip: true

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                        text: "Nandina UI"
                        color: demoWindow.themeManager.currentPaletteCollection.mainHeadline
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                    }

                    Behavior on width {
                        NumberAnimation {
                            duration: 180
                            easing.type: Easing.OutCubic
                        }
                    }
                }
            }
        }

        footer: Component {
            Rectangle {
                width: parent ? parent.width : 160
                height: 50
                radius: 8
                color: "red"
            }
        }

        NanSideBarGroup {
            title: "Controls"

            NanSideBarItem {
                text: "Dashboard"
                fallbackGlyph: "D"
                active: true
            }

            NanSideBarItem {
                text: "Inbox"
                fallbackGlyph: "I"
            }

            NanSideBarItem {
                text: "Calendar"
                fallbackGlyph: "C"
            }
        }

        NanSideBarGroup {
            title: "Projects"
            collapsible: true
            expanded: true

            NanSideBarItem {
                text: "Nandina Core"
                fallbackGlyph: "N"
            }

            NanSideBarItem {
                text: "Design System"
                fallbackGlyph: "D"
            }
        }
    }

    CustomTheme {
        id: customTheme
    }
}
