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
    property int currentSide: NanSideBar.Left

    Button {
        text: "Switch Theme"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        onClicked: {
            demoWindow.themeManager.setCurrentPaletteType(NandinaColor.Mocha);
            demoWindow.titleBarMode = NanWindow.DefaultTitleBar;
        }
    }

    Button {
        text: demoWindow.currentSide === NanSideBar.Left ? "Dock Right" : "Dock Left"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 60
        onClicked: {
            demoWindow.currentSide = demoWindow.currentSide === NanSideBar.Left ? NanSideBar.Right : NanSideBar.Left;
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
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        x: demoWindow.currentSide === NanSideBar.Left ? 0 : parent.width - width
        side: demoWindow.currentSide
        collapsible: NanSideBar.Icon
        open: true
        showDefaultTrigger: false
        themeManager: demoWindow.themeManager

        header: Component {
            Row {
                width: parent ? parent.width : 220
                height: 38
                spacing: 8

                NanSideBarTrigger {
                    sidebar: appSidebar
                    themeManager: demoWindow.themeManager
                }

                Rectangle {
                    width: appSidebar.collapsed ? 0 : (parent.width - 40)
                    height: 32
                    radius: 8
                    color: demoWindow.themeManager.currentPaletteCollection.surfaceElement0
                    clip: true

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        text: "Nandina UI"
                        color: demoWindow.themeManager.currentPaletteCollection.mainHeadline
                        font.pixelSize: 26
                        font.weight: Font.Medium
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
            NanSideBarItem {
                sidebar: appSidebar
                themeManager: demoWindow.themeManager
                text: "User Profile"
                fallbackGlyph: "U"
            }
        }

        NanSideBarGroup {
            sidebar: appSidebar
            themeManager: demoWindow.themeManager
            title: "Platform"

            NanSideBarItem {
                sidebar: appSidebar
                themeManager: demoWindow.themeManager
                text: "Dashboard"
                fallbackGlyph: "D"
                active: true
            }

            NanSideBarItem {
                sidebar: appSidebar
                themeManager: demoWindow.themeManager
                text: "Inbox"
                fallbackGlyph: "I"
            }

            NanSideBarItem {
                sidebar: appSidebar
                themeManager: demoWindow.themeManager
                text: "Calendar"
                fallbackGlyph: "C"
            }
        }

        NanSideBarGroup {
            sidebar: appSidebar
            themeManager: demoWindow.themeManager
            title: "Projects"
            collapsible: true
            expanded: true

            NanSideBarItem {
                sidebar: appSidebar
                themeManager: demoWindow.themeManager
                text: "Nandina Core"
                fallbackGlyph: "N"
            }

            NanSideBarItem {
                sidebar: appSidebar
                themeManager: demoWindow.themeManager
                text: "Design System"
                fallbackGlyph: "D"
            }
        }
    }

    CustomTheme {
        id: customTheme
    }
}
