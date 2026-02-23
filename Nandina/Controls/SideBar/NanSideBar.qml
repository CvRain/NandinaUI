pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQml
import Nandina.Theme
import "../theme_utils.js" as ThemeUtils

Item {
    id: sideBar

    enum Side {
        Left,
        Right
    }

    enum Collapsible {
        Offcanvas,
        Icon,
        None
    }

    default property alias contentData: contentColumn.data
    property alias headerData: headerSlot.data
    property alias footerData: footerSlot.data

    implicitWidth: width    // 隐式宽度由当前状态决定
    implicitHeight: 520     // 隐式高度由当前状态决定
    clip: false // 禁用裁剪，以允许边缘切换指示器在外部显示

    property bool open: true    // 默认打开
    property int side: NanSideBar.Side.Left // 默认停靠在左侧
    property int collapsible: NanSideBar.Collapsible.Icon // 默认使用图标折叠
    property int collapsedWidth: 68 // 默认折叠宽度
    property int expandedWidth: 270 // 默认展开宽度
    property int railWidth: 8 // 默认轨道宽度
    property int railHitPadding: 8 // 鼠标点击轨道的额外范围
    property int animationDuration: 220 // 默认动画持续时间
    property int borderRadius: 14 // 默认边框圆角
    property int sectionPadding: 12 // 默认部分内边距
    property int contentSpacing: 10 // 默认内容间距
    property bool showDefaultTrigger: true // 默认显示内置触发器
    property bool showRail: true    // 默认显示轨道
    property bool showEdgeToggleIndicator: true // 默认显示边缘切换指示器
    property int edgeToggleSize: 30 // 默认边缘切换指示器大小
    property bool showSectionDivider: true // 默认显示部分分割线
    property var dockingParent: parent // 默认停靠父项
    property Component header // 头部标题组件
    property Component footer // 底部组件
    property var themeManager: null // 主题管理器

    readonly property bool isOffcanvas: sideBar.collapsible === NanSideBar.Collapsible.Offcanvas    // 默认为Offcanvas
    readonly property bool isIconCollapsible: sideBar.collapsible === NanSideBar.Collapsible.Icon // 默认为Icon折叠
    readonly property bool collapsed: sideBar.isIconCollapsible && !sideBar.open // 是否折叠
    readonly property bool hiddenOffcanvas: sideBar.isOffcanvas && !sideBar.open // 是否隐藏Offcanvas
    readonly property real panelWidth: sideBar.calcPanelWidth() // 计算当前面板宽度
    readonly property real hostWidth: sideBar.hiddenOffcanvas ? sideBar.railWidth : sideBar.panelWidth // 主机宽度（考虑Offcanvas隐藏时的轨道宽度）
    readonly property var resolvedDockingParent: sideBar.dockingParent // 父级停靠项
    readonly property bool autoDockEnabled: !!sideBar.resolvedDockingParent // 是否启用自动停靠
    readonly property color panelBaseColor: sideBar.themePalette ? sideBar.themePalette.secondaryPane : '#595979' // 面板基础颜色
    readonly property color panelBorderColor: sideBar.themePalette ? sideBar.themePalette.inactiveBorder : "#434350" // 面板边框颜色
    readonly property color panelOverlayColor: sideBar.themePalette ? sideBar.themePalette.overlay0 : "#3d3d49" // 面板叠加颜色

    signal toggled(bool open)

    ThemeManager {
        id: fallbackThemeManager
    }

    readonly property var resolvedThemeManager: ThemeUtils.resolveThemeManager(sideBar, sideBar.themeManager, fallbackThemeManager)
    readonly property var themePalette: sideBar.resolvedThemeManager && sideBar.resolvedThemeManager.currentPaletteCollection ? sideBar.resolvedThemeManager.currentPaletteCollection : null

    function calcPanelWidth() {
        //sideBar.collapsible === NanSideBar.Collapsible.None ? sideBar.expandedWidth : (sideBar.isIconCollapsible ? (sideBar.open ? sideBar.expandedWidth : sideBar.collapsedWidth) : sideBar.expandedWidth);
        let width;
        if (sideBar.collapsible === NanSideBar.Collapsible.None) {
            width = sideBar.expandedWidth;
        } else {
            if (sideBar.isIconCollapsible) {
                if (sideBar.open) {
                    width = sideBar.expandedWidth;
                } else {
                    width = sideBar.collapsedWidth;
                }
            } else {
                width = sideBar.expandedWidth;
            }
        }
        return width;
    }

    function toggle() {
        if (sideBar.collapsible === NanSideBar.Collapsible.None)
            return;
        sideBar.open = !sideBar.open;
    }

    function expand() {
        sideBar.open = true;
    }

    function collapse() {
        if (sideBar.collapsible === NanSideBar.Collapsible.None)
            return;
        sideBar.open = false;
    }

    onOpenChanged: sideBar.toggled(sideBar.open)

    anchors.top: undefined
    anchors.bottom: undefined
    anchors.left: undefined
    anchors.right: undefined

    Binding {
        target: sideBar
        property: "x"
        when: sideBar.autoDockEnabled && sideBar.resolvedDockingParent
        value: sideBar.side === NanSideBar.Side.Left ? 0 : Math.max(0, sideBar.resolvedDockingParent.width - sideBar.width)
    }

    Binding {
        target: sideBar
        property: "y"
        when: sideBar.autoDockEnabled
        value: 0
    }

    Binding {
        target: sideBar
        property: "height"
        when: sideBar.autoDockEnabled && sideBar.resolvedDockingParent && sideBar.resolvedDockingParent.height !== undefined
        value: sideBar.resolvedDockingParent.height
    }

    width: sideBar.hostWidth

    Behavior on width {
        NumberAnimation {
            duration: sideBar.animationDuration
            easing.type: Easing.OutCubic
        }
    }

    Behavior on x {
        enabled: sideBar.autoDockEnabled

        NumberAnimation {
            duration: sideBar.animationDuration
            easing.type: Easing.OutCubic
        }
    }

    Rectangle {
        id: panel

        width: sideBar.panelWidth
        height: parent.height
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        x: {
            if (!sideBar.hiddenOffcanvas)
                return 0;
            if (sideBar.side === NanSideBar.Side.Left)
                return -sideBar.panelWidth + sideBar.railWidth;
            return sideBar.railWidth;
        }
        radius: sideBar.borderRadius
        clip: true
        color: sideBar.panelBaseColor
        border.width: 1
        border.color: sideBar.panelBorderColor
        opacity: sideBar.hiddenOffcanvas ? 0 : 1

        Rectangle {
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                topMargin: 1
                leftMargin: 1
                rightMargin: 1
            }
            height: 48
            radius: sideBar.borderRadius - 1
            color: sideBar.panelOverlayColor
            opacity: 0.18
            clip: true
        }

        Rectangle {
            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
                bottomMargin: 1
                leftMargin: 1
                rightMargin: 1
            }
            height: 32
            radius: sideBar.borderRadius - 1
            color: sideBar.panelOverlayColor
            opacity: 0.1
            clip: true
        }

        Behavior on width {
            NumberAnimation {
                duration: sideBar.animationDuration
                easing.type: Easing.OutCubic
            }
        }

        Behavior on opacity {
            NumberAnimation {
                duration: 140
                easing.type: Easing.OutCubic
            }
        }

        Behavior on x {
            NumberAnimation {
                duration: sideBar.animationDuration
                easing.type: Easing.OutCubic
            }
        }

        ColumnLayout {
            id: sectionsLayout

            anchors {
                top: parent.top
                topMargin: sideBar.sectionPadding
                bottom: parent.bottom
                bottomMargin: sideBar.sectionPadding
                left: parent.left
                leftMargin: sideBar.sectionPadding
                right: parent.right
                rightMargin: sideBar.sectionPadding
            }
            spacing: 0

            Item {
                id: headerContainer
                Layout.fillWidth: true
                Layout.preferredHeight: headerContent.height
                visible: headerContent.height > 0

                Column {
                    id: headerContent
                    width: parent.width
                    height: childrenRect.height
                    spacing: sideBar.contentSpacing

                    NanSideBarTrigger {
                        visible: sideBar.showDefaultTrigger
                        sidebar: sideBar
                        themeManager: sideBar.themeManager
                    }

                    Loader {
                        id: headerLoader
                        width: parent.width
                        active: sideBar.header !== null
                        sourceComponent: sideBar.header
                        readonly property var loadedItem: item
                        readonly property real loadedImplicitHeight: loadedItem ? Number(loadedItem.implicitHeight || 0) : 0
                        readonly property real loadedExplicitHeight: loadedItem ? Number(loadedItem.height || 0) : 0
                        readonly property real loadedHeight: Math.max(loadedImplicitHeight, loadedExplicitHeight)
                        height: loadedHeight
                        visible: active
                    }

                    Column {
                        id: headerSlot
                        width: parent.width
                        spacing: sideBar.contentSpacing
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: sideBar.showSectionDivider && headerContainer.visible ? 1 : 0
                visible: Layout.preferredHeight > 0
                color: sideBar.themePalette ? sideBar.themePalette.surfaceElement0 : "#434350"
                opacity: 0.8
            }

            Flickable {
                id: contentFlickable
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: width
                contentHeight: contentColumn.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                opacity: sideBar.hiddenOffcanvas ? 0 : 1

                Behavior on opacity {
                    NumberAnimation {
                        duration: 120
                        easing.type: Easing.OutCubic
                    }
                }

                Column {
                    id: contentColumn
                    width: contentFlickable.width
                    spacing: sideBar.contentSpacing
                }

                Rectangle {
                    anchors {
                        top: parent.top
                        left: parent.left
                        right: parent.right
                    }
                    height: 14
                    color: sideBar.panelBaseColor
                    opacity: contentFlickable.contentY > 1 ? 0.9 : 0

                    Behavior on opacity {
                        NumberAnimation {
                            duration: 120
                            easing.type: Easing.OutCubic
                        }
                    }
                }

                Rectangle {
                    anchors {
                        bottom: parent.bottom
                        left: parent.left
                        right: parent.right
                    }
                    height: 14
                    color: sideBar.panelBaseColor
                    opacity: contentFlickable.contentY + contentFlickable.height < contentFlickable.contentHeight - 1 ? 0.9 : 0

                    Behavior on opacity {
                        NumberAnimation {
                            duration: 120
                            easing.type: Easing.OutCubic
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: sideBar.showSectionDivider && footerContainer.visible ? 1 : 0
                visible: Layout.preferredHeight > 0
                color: sideBar.themePalette ? sideBar.themePalette.surfaceElement0 : "#434350"
                opacity: 0.8
            }

            Item {
                id: footerContainer
                Layout.fillWidth: true
                Layout.preferredHeight: footerContent.height
                visible: footerContent.height > 0

                Column {
                    id: footerContent
                    width: parent.width
                    height: childrenRect.height
                    spacing: sideBar.contentSpacing

                    Loader {
                        id: footerLoader
                        width: parent.width
                        active: sideBar.footer !== null
                        sourceComponent: sideBar.footer
                        readonly property var loadedItem: item
                        readonly property real loadedImplicitHeight: loadedItem ? Number(loadedItem.implicitHeight || 0) : 0
                        readonly property real loadedExplicitHeight: loadedItem ? Number(loadedItem.height || 0) : 0
                        readonly property real loadedHeight: Math.max(loadedImplicitHeight, loadedExplicitHeight)
                        height: loadedHeight
                        visible: active
                    }

                    Column {
                        id: footerSlot
                        width: parent.width
                        spacing: sideBar.contentSpacing
                    }
                }
            }
        }
    }

    Rectangle {
        id: rail

        visible: sideBar.showRail && sideBar.collapsible !== NanSideBar.Collapsible.None
        width: sideBar.railWidth
        height: sideBar.height
        x: sideBar.side === NanSideBar.Side.Left ? 0 : Math.max(0, sideBar.width - width)
        y: 0
        radius: sideBar.railWidth / 2
        color: railArea.containsMouse ? (sideBar.themePalette ? sideBar.themePalette.overlay1 : "#3c3c48") : "transparent"

        Behavior on x {
            NumberAnimation {
                duration: sideBar.animationDuration
                easing.type: Easing.OutCubic
            }
        }

        Behavior on color {
            ColorAnimation {
                duration: 120
            }
        }

        MouseArea {
            id: railArea
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: -sideBar.railHitPadding
            anchors.rightMargin: -sideBar.railHitPadding
            hoverEnabled: true
            onClicked: sideBar.toggle()
        }
    }

    Rectangle {
        id: edgeToggle

        visible: sideBar.showEdgeToggleIndicator && sideBar.collapsible !== NanSideBar.Collapsible.None
        width: sideBar.edgeToggleSize
        height: sideBar.edgeToggleSize
        radius: Math.floor(sideBar.edgeToggleSize / 2)
        y: Math.floor((sideBar.height - height) / 2)
        x: {
            if (sideBar.side === NanSideBar.Side.Left)
                return panel.x + panel.width - Math.floor(width / 2);
            return panel.x - Math.floor(width / 2);
        }
        z: 20
        color: edgeArea.pressed ? (sideBar.themePalette ? sideBar.themePalette.overlay2 : "#4c4c58") : (edgeArea.containsMouse ? (sideBar.themePalette ? sideBar.themePalette.overlay1 : "#3b3b46") : sideBar.panelBaseColor)
        border.width: 1
        border.color: edgeArea.containsMouse ? (sideBar.themePalette ? sideBar.themePalette.activeBorder : "#6b6b78") : sideBar.panelBorderColor
        opacity: sideBar.hiddenOffcanvas ? 0.9 : 1.0

        Behavior on x {
            NumberAnimation {
                duration: sideBar.animationDuration
                easing.type: Easing.OutCubic
            }
        }

        Behavior on color {
            ColorAnimation {
                duration: 120
            }
        }

        Text {
            anchors.centerIn: parent
            text: {
                if (sideBar.side === NanSideBar.Side.Right)
                    return sideBar.open ? "▶" : "◀";
                return sideBar.open ? "◀" : "▶";
            }
            color: sideBar.themePalette ? sideBar.themePalette.mainHeadline : "#f5f5f5"
            font.pixelSize: 12
            font.weight: Font.DemiBold
        }

        MouseArea {
            id: edgeArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: sideBar.toggle()
        }
    }
}
