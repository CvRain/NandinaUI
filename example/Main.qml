import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nandina.Theme
import Nandina.Controls
import NandinaExample

ApplicationWindow {
    id: root
    visible: true
    width: 1200
    height: 760
    title: "Nandina Example — " + ThemeManager.currentThemeName
    color: ThemeManager.colors.surface.shade50

    property string currentPage: "theme"

    RowLayout {
        anchors.fill: parent
        spacing: 0

        NanSideBar {
            id: nav
            Layout.fillHeight: true
            collapsible: "icon"
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

                NanSideBarItem {
                    text: "Theme"
                    iconText: "◐"
                    active: root.currentPage === "theme"
                    onClicked: root.currentPage = "theme"
                }
                NanSideBarItem {
                    text: "Color Palettes"
                    iconText: "◍"
                    active: root.currentPage === "palette"
                    onClicked: root.currentPage = "palette"
                }
            }

            NanSideBarGroup {
                title: "Components"

                NanSideBarItem {
                    text: "Primitives"
                    iconText: "◧"
                    active: root.currentPage === "primitives"
                    onClicked: root.currentPage = "primitives"
                }
                NanSideBarItem {
                    text: "Cards"
                    iconText: "▤"
                    active: root.currentPage === "cards"
                    onClicked: root.currentPage = "cards"
                }
                NanSideBarItem {
                    text: "SideBar"
                    iconText: "☰"
                    active: root.currentPage === "sidebar"
                    onClicked: root.currentPage = "sidebar"
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
                                text: root.pageTitle(root.currentPage)
                                font.pixelSize: 15
                                font.bold: true
                                color: ThemeManager.colors.surface.shade700
                            }

                            Text {
                                text: root.pageSummary(root.currentPage)
                                font.pixelSize: 11
                                color: ThemeManager.colors.surface.shade500
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Button {
                            id: _darkModeBtn
                            text: ThemeManager.darkMode ? "☀ Light" : "🌙 Dark"
                            onClicked: ThemeManager.darkMode = !ThemeManager.darkMode
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    StackLayout {
                        anchors.fill: parent
                        anchors.margins: 24
                        currentIndex: root.pageIndex(root.currentPage)

                        ThemePage {}
                        ColorPalettePage {}
                        SurfacePressablePanelPage {}
                        CardPage {}
                        SideBarPage {}
                    }
                }
            }
        }
    }

    function pageIndex(key) {
        switch (key) {
        case "theme":
            return 0;
        case "palette":
            return 1;
        case "primitives":
            return 2;
        case "cards":
            return 3;
        case "sidebar":
            return 4;
        default:
            return 0;
        }
    }

    function pageTitle(key) {
        switch (key) {
        case "theme":
            return "Theme System";
        case "palette":
            return "Color Palettes";
        case "primitives":
            return "Interaction Primitives";
        case "cards":
            return "Card Presets";
        case "sidebar":
            return "SideBar Component";
        default:
            return "Theme System";
        }
    }

    function pageSummary(key) {
        switch (key) {
        case "theme":
            return "主题切换与暗色模式控制";
        case "palette":
            return "语义色板与完整色阶预览";
        case "primitives":
            return "Surface / Pressable / Panel 组合行为";
        case "cards":
            return "Card 结构与 preset 交互状态";
        case "sidebar":
            return "侧边栏模式与分组导航演示";
        default:
            return "主题切换与暗色模式控制";
        }
    }
}
