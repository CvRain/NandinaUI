import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nandina.Theme
import Nandina.Controls
import Nandina.Window
import NandinaExample

NanWindow {
    id: root
    width: 1200
    height: 760
    windowTitle: "Nandina Example — " + ThemeManager.currentThemeName
    titleBarMode: "frameless"

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
                    onClicked: root.openPage("theme")
                }
                NanSideBarItem {
                    text: "Color Palettes"
                    iconText: "◍"
                    active: root.currentPage === "palette"
                    onClicked: root.openPage("palette")
                }
            }

            NanSideBarGroup {
                title: "Components"

                NanSideBarItem {
                    text: "Primitives"
                    iconText: "◧"
                    active: root.currentPage === "primitives"
                    onClicked: root.openPage("primitives")
                }
                NanSideBarItem {
                    text: "Cards"
                    iconText: "▤"
                    active: root.currentPage === "cards"
                    onClicked: root.openPage("cards")
                }
                NanSideBarItem {
                    text: "Button"
                    iconText: "⬡"
                    active: root.currentPage === "button"
                    onClicked: root.openPage("button")
                }
                NanSideBarItem {
                    text: "SideBar"
                    iconText: "☰"
                    active: root.currentPage === "sidebar"
                    onClicked: root.openPage("sidebar")
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

                    StackView {
                        id: pageStack
                        anchors.fill: parent
                        anchors.margins: 24
                        initialItem: root.pageComponent(root.currentPage)

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

    function openPage(key) {
        if (root.currentPage === key)
            return;

        var targetComponent = root.pageComponent(key);
        if (!targetComponent)
            return;

        root.currentPage = key;

        if (pageStack.depth > 1)
            pageStack.clear(StackView.Immediate);

        if (pageStack.depth === 0) {
            pageStack.push(targetComponent, {}, StackView.Immediate);
            return;
        }

        pageStack.replace(targetComponent, {}, StackView.ReplaceTransition);
    }

    function pageComponent(key) {
        switch (key) {
        case "theme":
            return themePageComponent;
        case "palette":
            return palettePageComponent;
        case "primitives":
            return primitivesPageComponent;
        case "cards":
            return cardsPageComponent;
        case "button":
            return buttonPageComponent;
        case "sidebar":
            return sidebarPageComponent;
        default:
            return themePageComponent;
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
        case "button":
            return "NanButton";
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
        case "button":
            return "preset / variant / size / icon / keyboard";
        case "sidebar":
            return "侧边栏模式与分组导航演示";
        default:
            return "主题切换与暗色模式控制";
        }
    }

    Component {
        id: themePageComponent
        ThemePage {}
    }

    Component {
        id: palettePageComponent
        ColorPalettePage {}
    }

    Component {
        id: primitivesPageComponent
        SurfacePressablePanelPage {}
    }

    Component {
        id: cardsPageComponent
        CardPage {}
    }

    Component {
        id: buttonPageComponent
        ButtonPage {}
    }

    Component {
        id: sidebarPageComponent
        SideBarPage {}
    }
}
