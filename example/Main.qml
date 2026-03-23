pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Nandina.Theme
import Nandina.Controls
import Nandina.Window
import NandinaExample

NanWindow {
    id: root
    readonly property int _modeSystem: 0
    readonly property int _modeFrameless: 1
    readonly property int _modeCustom: 2

    width: 1200
    height: 760
    windowTitle: "Nandina Example — " + ThemeManager.currentThemeName
    titleBarMode: root._modeCustom
    injectControls: titleBarMode === root._modeCustom

    titleBar: Component {
        Item {
            Rectangle {
                anchors.fill: parent
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade50 : ThemeManager.colors.primary.shade50
                topLeftRadius: root._effectiveRadius
                topRightRadius: root._effectiveRadius
            }

            Rectangle {
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
                height: ThemeManager.primitives.borderWidth
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.primary.shade100
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 120
                spacing: 12

                Rectangle {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    radius: 14
                    color: ThemeManager.colors.primary.shade500

                    Text {
                        anchors.centerIn: parent
                        text: "N"
                        font.pixelSize: 13
                        font.bold: true
                        color: "white"
                    }
                }

                ColumnLayout {
                    spacing: 0

                    Text {
                        text: root.windowTitle
                        font.pixelSize: 13
                        font.bold: true
                        color: ThemeManager.colors.surface.shade700
                    }

                    Text {
                        text: root.currentPageSpec ? root.currentPageSpec.navTitle + " / custom title bar" : "custom title bar"
                        font.pixelSize: 11
                        color: ThemeManager.colors.surface.shade500
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                Rectangle {
                    radius: 999
                    color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade200 : ThemeManager.colors.primary.shade100
                    Layout.alignment: Qt.AlignVCenter
                    implicitWidth: _customModeLabel.implicitWidth + 16
                    implicitHeight: 24

                    Text {
                        id: _customModeLabel
                        anchors.centerIn: parent
                        text: "Custom"
                        font.pixelSize: 11
                        font.bold: true
                        color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.primary.shade700
                    }
                }
            }

            DragHandler {
                target: null
                grabPermissions: PointerHandler.CanTakeOverFromAnything
                onActiveChanged: {
                    if (active)
                        root.startSystemMove();
                }
            }

            TapHandler {
                gesturePolicy: TapHandler.DragThreshold
                onDoubleTapped: {
                    if (root.visibility === Window.Maximized)
                        root.showNormal();
                    else
                        root.showMaximized();
                }
            }
        }
    }

    property int navSidebarMode: NanSideBar.Icon

    NanRouter {
        id: router
        stackView: pageStack
        routes: PageRegistration.pageRegistry
        currentRouteId: PageRegistration.firstKey()
    }

    readonly property var foundationPages: router.routesForSection("Foundation")
    readonly property var componentPages: router.routesForSection("Components")
    readonly property var footerPages: router.routesForSection("Footer")
    readonly property var currentPageSpec: router.currentRoute

    RowLayout {
        anchors.fill: parent
        spacing: 0

        NanSideBar {
            id: nav
            Layout.fillHeight: true
            collapsible: root.navSidebarMode
            expandedWidth: 238

            header: Item {
                implicitHeight: 52
                Row {
                    anchors {
                        verticalCenter: parent.verticalCenter
                        left: parent.left
                        leftMargin: 14
                    }
                    spacing: 8

                    Text {
                        text: "❀"
                        font.pixelSize: 16
                        color: ThemeManager.colors.primary.shade600
                    }
                    Text {
                        visible: !nav.collapsed
                        text: "Nandina Example"
                        font.pixelSize: 13
                        font.bold: true
                        color: ThemeManager.colors.surface.shade700
                    }
                }
            }

            NanSideBarGroup {
                title: "Foundation"

                Repeater {
                    model: root.foundationPages
                    delegate: NanSideBarItem {
                        required property var modelData
                        text: modelData.navTitle
                        iconText: modelData.iconText
                        active: router.currentRouteId === modelData.key
                        onClicked: router.replace(modelData.key)
                    }
                }
            }

            NanSideBarGroup {
                title: "Components"

                Repeater {
                    model: root.componentPages
                    delegate: NanSideBarItem {
                        required property var modelData
                        text: modelData.navTitle
                        iconText: modelData.iconText
                        active: router.currentRouteId === modelData.key
                        onClicked: router.replace(modelData.key)
                    }
                }
            }

            footer: Item {
                implicitHeight: _footerColumn.implicitHeight

                Column {
                    id: _footerColumn
                    width: parent.width

                    Repeater {
                        model: root.footerPages
                        delegate: NanSideBarItem {
                            required property var modelData
                            width: parent ? parent.width : implicitWidth
                            text: modelData.navTitle
                            iconText: modelData.iconText
                            active: router.currentRouteId === modelData.key
                            onClicked: router.replace(modelData.key)
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 52
                    color: ThemeManager.colors.surface.shade100
                    border.color: ThemeManager.colors.surface.shade200
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 12

                        ColumnLayout {
                            spacing: 1

                            Text {
                                text: root.currentPageSpec ? root.currentPageSpec.title : ""
                                font.pixelSize: 15
                                font.bold: true
                                color: ThemeManager.colors.surface.shade700
                            }

                            Text {
                                text: root.currentPageSpec ? root.currentPageSpec.summary : ""
                                font.pixelSize: 11
                                color: ThemeManager.colors.surface.shade500
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        NanButton {
                            id: _darkModeBtn
                            text: ThemeManager.darkMode ? "☀ Light" : "🌙 Dark"
                            onClicked: ThemeManager.darkMode = !ThemeManager.darkMode
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    StackView {
                        id: pageStack
                        anchors.fill: parent
                        anchors.margins: 24

                        replaceEnter: Transition {
                            ParallelAnimation {
                                NumberAnimation {
                                    property: "x"
                                    from: 18
                                    to: 0
                                    duration: 140
                                    easing.type: Easing.OutCubic
                                }
                                NumberAnimation {
                                    property: "opacity"
                                    from: 0
                                    to: 1
                                    duration: 120
                                }
                            }
                        }

                        replaceExit: Transition {
                            NumberAnimation {
                                property: "opacity"
                                from: 1
                                to: 0
                                duration: 90
                            }
                        }
                    }
                }
            }
        }
    }
}
