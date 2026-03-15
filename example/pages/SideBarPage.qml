pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nandina.Theme
import Nandina.Controls

NanPage {
    id: root
    property int sidebarMode: NanSideBar.Icon

    readonly property var sidebarModes: [
        {
            text: "NanSideBar.Icon",
            value: NanSideBar.Icon
        },
        {
            text: "NanSideBar.Offcanvas",
            value: NanSideBar.Offcanvas
        },
        {
            text: "NanSideBar.None",
            value: NanSideBar.None
        }
    ]

    function sidebarModeLabel(mode) {
        switch (mode) {
        case NanSideBar.Icon:
            return "NanSideBar.Icon";
        case NanSideBar.Offcanvas:
            return "NanSideBar.Offcanvas";
        case NanSideBar.None:
            return "NanSideBar.None";
        default:
            return "unknown";
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 12

            Text {
                text: root.routeSpec?.navTitle ?? ""
                font.pixelSize: 28
                font.bold: true
                color: ThemeManager.colors.primary.shade700
            }

            Text {
                text: root.routeSpec?.summary ?? ""
                font.pixelSize: 13
                color: ThemeManager.colors.surface.shade600
            }

            Row {
                spacing: 8

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Collapsible:"
                    font.pixelSize: 13
                    color: ThemeManager.colors.surface.shade700
                }

                Repeater {
                    model: root.sidebarModes
                    delegate: NanButton {
                        required property var modelData
                        text: modelData.text
                        onClicked: root.sidebarMode = modelData.value
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 420
                radius: ThemeManager.primitives.radiusContainer
                clip: true
                color: ThemeManager.colors.surface.shade100
                border.color: ThemeManager.colors.surface.shade300
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    NanSideBar {
                        id: _sidebarDemo
                        Layout.fillHeight: true
                        collapsible: root.sidebarMode
                        expandedWidth: 210

                        NanSideBarGroup {
                            title: "Main"

                            NanSideBarItem {
                                text: "Dashboard"
                                iconText: "⌂"
                                active: true
                            }
                            NanSideBarItem {
                                text: "Inbox"
                                iconText: "✉"
                                badge: "9"
                            }
                            NanSideBarItem {
                                text: "Calendar"
                                iconText: "◻"
                                badge: "3"
                            }
                        }

                        NanSideBarGroup {
                            title: "Projects"
                            collapsible: true

                            NanSideBarItem {
                                text: "Alpha"
                                iconText: "α"
                                NanSideBarItem {
                                    isSubItem: true
                                    text: "Overview"
                                }
                                NanSideBarItem {
                                    isSubItem: true
                                    text: "Issues"
                                }
                            }
                            NanSideBarItem {
                                text: "Beta"
                                iconText: "β"
                            }
                        }

                        NanSideBarGroup {
                            title: "Settings"

                            NanSideBarItem {
                                text: "Preferences"
                                iconText: "⚙"
                            }
                            NanSideBarItem {
                                text: "Members"
                                iconText: "◉"
                                badge: "12"
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "transparent"

                        Column {
                            anchors.centerIn: parent
                            spacing: 12

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "Main content"
                                font.pixelSize: 18
                                font.bold: true
                                color: ThemeManager.colors.surface.shade600
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "Sidebar is " + (_sidebarDemo.open ? "open" : "closed") + " · mode: " + root.sidebarModeLabel(_sidebarDemo.collapsible)
                                font.pixelSize: 13
                                color: ThemeManager.colors.surface.shade500
                            }
                        }
                    }
                }
            }

            Item {
                Layout.preferredHeight: 24
            }
        }
    }
}
