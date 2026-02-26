pragma ComponentBehavior: Bound

import QtQuick
import Nandina.Color
import Nandina.Window
import Nandina.Controls
import Nandina.Theme
import Nandina.Tokens
import Nandina.Experimental

NanWindow {
    id: demoWindow

    width: 900
    height: 600
    visible: true
    windowTitle: "NanWindow Demo"
    titleBarMode: NanWindow.CustomTitleBar
    themeManager.customColorCollection: customTheme.colorCollection
    themeManager.customPaletteCollection: customTheme.paletteCollection
    font: Qt.font({
        family: "Sans Serif",
        pixelSize: NanTypography.body.pixelSize,
        weight: Font.Normal
    })
    NanStyle.themeManager: demoWindow.themeManager
    NanStyle.font: demoWindow.font
    property int currentSide: NanSideBar.Side.Left
    property var sidebarInteractionLogs: []

    function appendSidebarInteractionLog(type, eventPayload) {
        var next = sidebarInteractionLogs.slice(0);
        var sideLabel = eventPayload && eventPayload.side === NanSideBar.Side.Right ? "Right" : "Left";
        var openLabel = eventPayload && eventPayload.open ? "open" : "closed";
        next.unshift(type + " [" + sideLabel + ", " + openLabel + "]");
        if (next.length > 8)
            next = next.slice(0, 8);
        sidebarInteractionLogs = next;
    }

    NanButton {
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

    NanButton {
        text: demoWindow.currentSide === NanSideBar.Side.Left ? "Dock Right" : "Dock Left"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 60
        onClicked: {
            demoWindow.currentSide = demoWindow.currentSide === NanSideBar.Side.Left ? NanSideBar.Side.Right : NanSideBar.Side.Left;
        }
    }

    Text {
        id: titleText
        anchors.centerIn: parent
        text: "NanWindow Demo"
        font.pointSize: 20
        font.bold: true
        color: demoWindow.themeManager.currentColorCollection.rosewater
    }

    NanButton {
        id: testButton1
        text: "Test Button 1"
        font.bold: true

        variant: NanButton.Primary
        accent: NanButton.Tonal

        anchors.top: titleText.bottom
        anchors.topMargin: 60
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Rectangle {
        id: sidebarEventLogPanel
        width: 280
        height: 210
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 18
        anchors.rightMargin: 18
        radius: 10
        color: demoWindow.themeManager.currentPaletteCollection.secondaryPane
        border.width: 1
        border.color: demoWindow.themeManager.currentPaletteCollection.surfaceElement0

        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 6

            Text {
                text: "Sidebar Events"
                color: demoWindow.themeManager.currentPaletteCollection.mainHeadline
                font.bold: true
            }

            Repeater {
                model: demoWindow.sidebarInteractionLogs

                delegate: Text {
                    required property var modelData
                    text: modelData
                    color: demoWindow.themeManager.currentPaletteCollection.bodyCopy
                    font.pixelSize: NanTypography.caption.pixelSize
                    elide: Text.ElideRight
                    width: parent ? parent.width : 240
                }
            }
        }
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
                        font: Qt.font({
                            family: demoWindow.font.family,
                            pixelSize: NanTypography.subtitle.pixelSize,
                            weight: Font.DemiBold
                        })
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
                text: "Settings"
                fallbackGlyph: "S"
            }
        }

        NanSideBarGroup {
            title: "Controls"
            collapsible: true
            expanded: true
            font: Qt.font({
                family: demoWindow.font.family,
                pixelSize: NanTypography.caption.pixelSize,
                weight: Font.DemiBold
            })

            NanSideBarItem {
                text: "NanButton"
                fallbackGlyph: "B"
                active: true
                font: Qt.font({
                    family: demoWindow.font.family,
                    pixelSize: NanTypography.bodyLarge.pixelSize,
                    weight: Font.Medium
                })
            }

            NanSideBarItem {
                text: "NanInput"
                fallbackGlyph: "I"
            }
        }

        NanStyleScope {
            ThemeManager {
                id: componentScopeTheme
                Component.onCompleted: setCurrentPaletteType(NandinaColor.Frappe)
            }

            NanStyle.themeManager: componentScopeTheme
            NanStyle.font: Qt.font({
                family: demoWindow.font.family,
                pixelSize: NanTypography.caption.pixelSize,
                weight: Font.Medium
            })

            NanSideBarGroup {
                title: "Components"
                collapsible: true
                expanded: true

                NanSideBarItem {
                    text: "Scoped Theme"
                    fallbackGlyph: "T"
                }
            }
        }
    }

    Connections {
        target: appSidebar

        function onSideBarInteraction(type, payload) {
            demoWindow.appendSidebarInteractionLog(type, payload);
        }
    }

    CustomTheme {
        id: customTheme
    }
}
