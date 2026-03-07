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
                title: "Pages"

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
                    text: "SideBar Demo"
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

                        Text {
                            text: root.pageTitle(root.currentPage)
                            font.pixelSize: 15
                            font.bold: true
                            color: ThemeManager.colors.surface.shade700
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
            return "Theme";
        case "palette":
            return "Color Palettes";
        case "primitives":
            return "Surface / Pressable / Panel";
        case "cards":
            return "Card";
        case "sidebar":
            return "SideBar Demo";
        default:
            return "Theme";
        }
    }
}
/*
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nandina.Theme
import Nandina.Controls

ApplicationWindow {
    id: root
    visible: true
    width: 800
    height: 600
    title: "Nandina Theme Example — " + ThemeManager.currentThemeName
    color: ThemeManager.colors.surface.shade50

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            anchors {
                left: parent.left
                right: parent.right
                margins: 24
            }
            spacing: 20

            // ── Header ─────────────────────────────────────────────────
            Text {
                Layout.topMargin: 24
                text: "Nandina Theme Preview"
                font.pixelSize: 28
                font.bold: true
                color: ThemeManager.colors.primary.shade700
            }

            Text {
                text: "Current theme: " + ThemeManager.currentThemeName + (ThemeManager.darkMode ? " (dark)" : " (light)")
                font.pixelSize: 14
                color: ThemeManager.colors.surface.shade700
            }

            // ── Theme switch buttons ────────────────────────────────────
            Text {
                Layout.topMargin: 12
                text: "Switch Theme"
                font.pixelSize: 18
                font.bold: true
                color: ThemeManager.colors.primary.shade600
            }

            Flow {
                Layout.fillWidth: true
                spacing: 8

                Repeater {
                    model: ThemeManager.availableThemes
                    delegate: Button {
                        id: _themeBtn
                        required property string modelData
                        text: modelData
                        highlighted: ThemeManager.currentThemeName === modelData
                        onClicked: ThemeManager.setThemeByName(modelData)

                        background: Rectangle {
                            radius: 6
                            color: _themeBtn.highlighted ? ThemeManager.colors.primary.shade500 : (_themeBtn.hovered ? ThemeManager.colors.surface.shade200 : ThemeManager.colors.surface.shade100)
                            border.color: ThemeManager.colors.primary.shade300
                            border.width: _themeBtn.highlighted ? 0 : 1
                        }
                        contentItem: Text {
                            text: _themeBtn.text
                            font.pixelSize: 13
                            font.capitalization: Font.Capitalize
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: _themeBtn.highlighted ? "#ffffff" : ThemeManager.colors.surface.shade800
                        }
                    }
                }

                Button {
                    id: _darkModeBtn
                    text: ThemeManager.darkMode ? "☀ Light" : "🌙 Dark"
                    onClicked: ThemeManager.darkMode = !ThemeManager.darkMode

                    background: Rectangle {
                        radius: 6
                        color: _darkModeBtn.hovered ? ThemeManager.colors.tertiary.shade200 : ThemeManager.colors.tertiary.shade100
                        border.color: ThemeManager.colors.tertiary.shade400
                        border.width: 1
                    }
                    contentItem: Text {
                        text: _darkModeBtn.text
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: ThemeManager.colors.tertiary.shade800
                    }
                }
            }

            // ── Color palettes ─────────────────────────────────────────
            Text {
                Layout.topMargin: 16
                text: "Color Palettes"
                font.pixelSize: 18
                font.bold: true
                color: ThemeManager.colors.primary.shade600
            }

            ColorRow {
                label: "Primary"
                colorPalette: ThemeManager.colors.primary
            }
            ColorRow {
                label: "Secondary"
                colorPalette: ThemeManager.colors.secondary
            }
            ColorRow {
                label: "Tertiary"
                colorPalette: ThemeManager.colors.tertiary
            }
            ColorRow {
                label: "Success"
                colorPalette: ThemeManager.colors.success
            }
            ColorRow {
                label: "Warning"
                colorPalette: ThemeManager.colors.warning
            }
            ColorRow {
                label: "Error"
                colorPalette: ThemeManager.colors.error
            }
            ColorRow {
                label: "Surface"
                colorPalette: ThemeManager.colors.surface
            }

            // ── NanSurface 演示 ────────────────────────────────────────
            Text {
                Layout.topMargin: 16
                text: "NanSurface — 主题感知容器"
                font.pixelSize: 18
                font.bold: true
                color: ThemeManager.colors.primary.shade600
            }
            Text {
                text: "colorVariant 与 shade 自动跟随主题，无需手动切换"
                font.pixelSize: 13
                color: ThemeManager.colors.surface.shade600
                Layout.bottomMargin: 4
            }

            Flow {
                Layout.fillWidth: true
                spacing: 12

                Repeater {
                    model: ["surface", "primary", "secondary", "tertiary", "success", "warning", "error"]
                    delegate: NanSurface {
                        required property string modelData
                        width: 110
                        height: 72
                        colorVariant: modelData

                        Text {
                            anchors.centerIn: parent
                            text: parent.modelData
                            font.pixelSize: 12
                            font.capitalization: Font.Capitalize
                            color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade200 : ThemeManager.colors.surface.shade800
                        }
                    }
                }
            }

            // ── NanPressable 演示 ──────────────────────────────────────
            Text {
                Layout.topMargin: 16
                text: "NanPressable — 纯交互原语"
                font.pixelSize: 18
                font.bold: true
                color: ThemeManager.colors.primary.shade600
            }
            Text {
                text: "NanSurface + NanPressable 组合：hover / press / click 状态可观察"
                font.pixelSize: 13
                color: ThemeManager.colors.surface.shade600
                Layout.bottomMargin: 4
            }

            Row {
                spacing: 16

                // 普通点击卡片
                NanSurface {
                    id: _pressDemo
                    width: 160
                    height: 80
                    colorVariant: "primary"
                    backgroundShade: _pressable.pressed ? 400 : _pressable.hovered ? 100 : -1
                    borderShade: _pressable.hovered ? 400 : -1

                    Behavior on backgroundShade {
                        NumberAnimation {
                            duration: 80
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        text: _pressable.pressed ? "▼ Pressed" : _pressable.hovered ? "▲ Hovered" : "👆 Click me"
                        font.pixelSize: 13
                        color: ThemeManager.darkMode ? ThemeManager.colors.primary.shade200 : ThemeManager.colors.primary.shade700
                    }

                    NanPressable {
                        id: _pressable
                        anchors.fill: parent
                        onClicked: _clickCount.count++
                    }
                }

                // 禁用状态卡片
                NanSurface {
                    width: 160
                    height: 80
                    colorVariant: "surface"
                    opacity: 0.45

                    Text {
                        anchors.centerIn: parent
                        text: "🚫 Disabled"
                        font.pixelSize: 13
                        color: ThemeManager.colors.surface.shade500
                    }

                    NanPressable {
                        anchors.fill: parent
                        enabled: false
                    }
                }

                // 长按卡片
                NanSurface {
                    id: _longPressCard
                    width: 160
                    height: 80
                    colorVariant: "tertiary"
                    property bool _triggered: false

                    Text {
                        anchors.centerIn: parent
                        text: _longPressCard._triggered ? "✔ Long pressed!" : "⏳ Hold me"
                        font.pixelSize: 13
                        color: ThemeManager.darkMode ? ThemeManager.colors.tertiary.shade200 : ThemeManager.colors.tertiary.shade700
                    }

                    NanPressable {
                        anchors.fill: parent
                        longPressInterval: 800
                        onLongPressed: _longPressCard._triggered = !_longPressCard._triggered
                    }
                }
            }

            Text {
                id: _clickCount
                property int count: 0
                text: "Click count: " + count
                font.pixelSize: 14
                color: ThemeManager.colors.primary.shade500
            }

            // ── NanPanel 演示 ──────────────────────────────────────────
            Text {
                Layout.topMargin: 16
                text: "NanPanel — 主题容器 + 可选标题"
                font.pixelSize: 18
                font.bold: true
                color: ThemeManager.colors.primary.shade600
            }
            Text {
                text: "内边距、分割线、标题字体均自动追踪主题 token"
                font.pixelSize: 13
                color: ThemeManager.colors.surface.shade600
                Layout.bottomMargin: 8
            }

            // 无标题 panel
            NanPanel {
                Layout.fillWidth: true
                Text {
                    text: "无标题 Panel — 只有内容区，无分割线"
                    font.pixelSize: 13
                    color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade200 : ThemeManager.colors.surface.shade700
                }
            }

            // 带标题 panel
            NanPanel {
                Layout.fillWidth: true
                title: "Panel Title"
                Text {
                    text: "带标题 Panel — 自动显示分割线，手动指定标题"
                    font.pixelSize: 13
                    color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade200 : ThemeManager.colors.surface.shade700
                }
            }

            // variant 演示
            Flow {
                Layout.fillWidth: true
                spacing: 12
                Layout.bottomMargin: 12

                Repeater {
                    model: ["primary", "success", "warning", "error"]
                    delegate: NanPanel {
                        required property string modelData
                        width: 220
                        colorVariant: modelData
                        backgroundShade: ThemeManager.darkMode ? 900 : 50
                        borderShade: ThemeManager.darkMode ? 700 : 200
                        title: modelData.charAt(0).toUpperCase() + modelData.slice(1) + " Panel"

                        Text {
                            text: "colorVariant: \"" + parent.colorVariant + "\""
                            font.pixelSize: 12
                            color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade600
                        }
                    }
                }
            }

            // 嵌套 panels
            NanPanel {
                Layout.fillWidth: true
                title: "Outer Panel"

                ColumnLayout {
                    width: parent.width
                    spacing: 8

                    Text {
                        text: "Panels 可以嵌套，外层用 surface，内层可主动指定 variant"
                        font.pixelSize: 13
                        color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade200 : ThemeManager.colors.surface.shade700
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }

                    Row {
                        spacing: 8

                        NanPanel {
                            width: 160
                            title: "Inner A"
                            colorVariant: "primary"
                            backgroundShade: ThemeManager.darkMode ? 900 : 50
                            Text {
                                text: "Primary tint"
                                font.pixelSize: 12
                                color: ThemeManager.darkMode ? ThemeManager.colors.primary.shade200 : ThemeManager.colors.primary.shade700
                            }
                        }

                        NanPanel {
                            width: 160
                            title: "Inner B"
                            colorVariant: "tertiary"
                            backgroundShade: ThemeManager.darkMode ? 900 : 50
                            Text {
                                text: "Tertiary tint"
                                font.pixelSize: 12
                                color: ThemeManager.darkMode ? ThemeManager.colors.tertiary.shade200 : ThemeManager.colors.tertiary.shade700
                            }
                        }
                    }
                }
            }

            // ── NanCard 演示 ───────────────────────────────────────────
            Text {
                Layout.topMargin: 16
                text: "NanCard — 结构化卡片"
                font.pixelSize: 18
                font.bold: true
                color: ThemeManager.colors.primary.shade600
            }
            Text {
                text: "支持 outlined / tonal / filled 三种预设，header / content / footer 分区，可选图片横幅与交互状态"
                font.pixelSize: 13
                color: ThemeManager.colors.surface.shade600
                Layout.bottomMargin: 8
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            // 三种 Preset 横向对比
            Text {
                text: "Preset 对比（surface variant）"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade700
            }
            Flow {
                Layout.fillWidth: true
                spacing: 12

                // outlined
                NanCard {
                    width: 240
                    preset: "outlined"
                    title: "Outlined Card"
                    description: "Near-transparent bg with subtle border — default preset"
                    Text {
                        text: "Body content area.\nAdd any items here."
                        font.pixelSize: 13
                        color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade600
                        lineHeight: 1.5
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                }

                // tonal
                NanCard {
                    width: 240
                    preset: "tonal"
                    colorVariant: "primary"
                    title: "Tonal Card"
                    description: "Lightly tinted background that adapts to dark mode"
                    Text {
                        text: "Tonal preset with primary variant."
                        font.pixelSize: 13
                        color: ThemeManager.darkMode ? ThemeManager.colors.primary.shade300 : ThemeManager.colors.primary.shade700
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                }

                // filled
                NanCard {
                    width: 240
                    preset: "filled"
                    colorVariant: "primary"
                    title: "Filled Card"
                    description: "Solid colour fill — white text for strong visual weight"
                    Text {
                        text: "Filled preset with high contrast white text."
                        font.pixelSize: 13
                        color: "#ffffff"
                        opacity: 0.85
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                }
            }

            // All colorVariants × tonal
            Text {
                Layout.topMargin: 12
                text: "Tonal preset — 全色板"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade700
            }
            Flow {
                Layout.fillWidth: true
                spacing: 10

                Repeater {
                    model: ["primary", "secondary", "tertiary", "success", "warning", "error"]
                    delegate: NanCard {
                        required property string modelData
                        width: 170
                        preset: "tonal"
                        colorVariant: modelData
                        title: modelData.charAt(0).toUpperCase() + modelData.slice(1)
                        description: "Tonal · " + modelData
                    }
                }
            }

            // Header action slot demo
            Text {
                Layout.topMargin: 12
                text: "Header Action 插槽 + showDividers"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade700
            }
            NanCard {
                Layout.fillWidth: true
                preset: "outlined"
                title: "Card with Header Action"
                description: "The action slot anchors any item to the header top-right corner"
                showDividers: true
                headerAction: Rectangle {
                    implicitWidth: 52
                    implicitHeight: 22
                    radius: 11
                    color: ThemeManager.colors.success.shade500
                    Text {
                        anchors.centerIn: parent
                        text: "Badge"
                        font.pixelSize: 10
                        font.bold: true
                        color: "#ffffff"
                    }
                }

                Text {
                    text: "Content area below the divider.\nshowDividers: true draws hairlines between sections."
                    font.pixelSize: 13
                    color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade600
                    lineHeight: 1.5
                    wrapMode: Text.WordWrap
                    width: parent.width
                }

                footer: Row {
                    spacing: 8
                    // Minimal inline pill buttons for demo (NanButton not yet available)
                    Repeater {
                        model: ["Cancel", "Confirm"]
                        delegate: Rectangle {
                            required property string modelData
                            required property int index
                            implicitWidth: _label.implicitWidth + 24
                            implicitHeight: 32
                            radius: ThemeManager.primitives.radiusBase
                            color: index === 1 ? ThemeManager.colors.primary.shade500 : (ThemeManager.darkMode ? ThemeManager.colors.surface.shade800 : ThemeManager.colors.surface.shade100)
                            border.color: ThemeManager.colors.surface.shade300
                            border.width: index === 0 ? 1 : 0
                            Text {
                                id: _label
                                anchors.centerIn: parent
                                text: parent.modelData
                                font.pixelSize: 13
                                color: parent.index === 1 ? "#ffffff" : (ThemeManager.darkMode ? ThemeManager.colors.surface.shade200 : ThemeManager.colors.surface.shade700)
                            }
                        }
                    }
                }
            }

            // Interactive card demo
            Text {
                Layout.topMargin: 12
                text: "interactive: true — 悬停 / 按压 / 点击"
                font.pixelSize: 14
                font.bold: true
                color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade700
            }
            Flow {
                Layout.fillWidth: true
                spacing: 12

                Repeater {
                    model: [
                        {
                            preset: "outlined",
                            variant: "surface"
                        },
                        {
                            preset: "tonal",
                            variant: "primary"
                        },
                        {
                            preset: "filled",
                            variant: "secondary"
                        }
                    ]
                    delegate: NanCard {
                        required property var modelData
                        width: 220
                        interactive: true
                        preset: modelData.preset
                        colorVariant: modelData.variant
                        title: modelData.preset.charAt(0).toUpperCase() + modelData.preset.slice(1)
                        description: "variant: " + modelData.variant

                        Text {
                            text: parent.pressed ? "▼ Pressed" : parent.hovered ? "▲ Hovered" : "👆 Click me"
                            font.pixelSize: 13
                            color: parent.preset === "filled" ? "#ffffff" : (ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade600)
                            width: parent.width
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }

            Item {
                Layout.preferredHeight: 32
            }
        }
    }

    // ── Inline helper component ────────────────────────────────────
    component ColorRow: ColumnLayout {
        property string label
        property var colorPalette
        spacing: 4

        Text {
            text: label
            font.pixelSize: 13
            font.bold: true
            color: ThemeManager.colors.surface.shade700
        }

        Row {
            spacing: 2
            Repeater {
                model: [
                    {
                        idx: 0,
                        name: "50"
                    },
                    {
                        idx: 1,
                        name: "100"
                    },
                    {
                        idx: 2,
                        name: "200"
                    },
                    {
                        idx: 3,
                        name: "300"
                    },
                    {
                        idx: 4,
                        name: "400"
                    },
                    {
                        idx: 5,
                        name: "500"
                    },
                    {
                        idx: 6,
                        name: "600"
                    },
                    {
                        idx: 7,
                        name: "700"
                    },
                    {
                        idx: 8,
                        name: "800"
                    },
                    {
                        idx: 9,
                        name: "900"
                    },
                    {
                        idx: 10,
                        name: "950"
                    }
                ]
                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    readonly property color shadeColor: colorPalette ? colorPalette.shade(modelData.idx) : "transparent"

                    width: 56
                    height: 40
                    radius: 4
                    color: shadeColor
                    border.color: Qt.darker(shadeColor, 1.15)
                    border.width: 0.5

                    Text {
                        anchors.centerIn: parent
                        text: parent.modelData.name
                        font.pixelSize: 9
                        color: parent.index < 5 ? "#333333" : "#ffffff"
                    }
                }
            }

            // ── NanSideBar ────────────────────────────────────────────────
            Text {
                Layout.topMargin: 8
                text: "SideBar"
                font.pixelSize: 18
                font.bold: true
                color: ThemeManager.colors.primary.shade600
            }

            // Mode selector
            Row {
                spacing: 8
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Collapsible:"
                    font.pixelSize: 13
                    color: ThemeManager.colors.surface.shade700
                }
                Repeater {
                    model: ["icon", "offcanvas", "none"]
                    delegate: Button {
                        id: _modeBtn
                        required property string modelData
                        required property int index
                        text: modelData
                        highlighted: _sidebarDemo.collapsible === modelData
                        onClicked: _sidebarDemo.collapsible = modelData
                    }
                }
            }

            // Demo frame  (mini-window)
            Rectangle {
                Layout.fillWidth: true
                height: 380
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
                        collapsible: "icon"
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

                    // Main content pane
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
                                text: "Sidebar is " + (_sidebarDemo.open ? "open" : "closed") + " · mode: " + _sidebarDemo.collapsible
                                font.pixelSize: 13
                                color: ThemeManager.colors.surface.shade500
                            }
                        }
                    }
                }
            }
        }
    }
}
*/
