pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
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

    implicitWidth: width
    implicitHeight: 520
    clip: false

    property bool open: true
    property int side: NanSideBar.Side.Left
    property int collapsible: NanSideBar.Collapsible.Icon
    property int collapsedWidth: 68
    property int expandedWidth: 270
    property int railWidth: 8
    property int railHitPadding: 8
    property int animationDuration: 220
    property int borderRadius: 12
    property int sectionPadding: 12
    property int contentSpacing: 8
    property bool showDefaultTrigger: true
    property bool showRail: true
    property bool showEdgeToggleIndicator: true
    property int edgeToggleSize: 30
    property bool showSectionDivider: true
    property bool autoDockToParent: false
    property Component header
    property Component footer
    property var themeManager: null

    readonly property bool isOffcanvas: sideBar.collapsible === NanSideBar.Collapsible.Offcanvas
    readonly property bool isIconCollapsible: sideBar.collapsible === NanSideBar.Collapsible.Icon
    readonly property bool collapsed: sideBar.isIconCollapsible && !sideBar.open
    readonly property bool hiddenOffcanvas: sideBar.isOffcanvas && !sideBar.open
    readonly property real panelWidth: sideBar.collapsible === NanSideBar.Collapsible.None ? sideBar.expandedWidth : (sideBar.isIconCollapsible ? (sideBar.open ? sideBar.expandedWidth : sideBar.collapsedWidth) : sideBar.expandedWidth)
    readonly property real hostWidth: sideBar.hiddenOffcanvas ? sideBar.railWidth : sideBar.panelWidth

    signal toggled(bool open)

    ThemeManager {
        id: fallbackThemeManager
    }

    readonly property var resolvedThemeManager: ThemeUtils.resolveThemeManager(sideBar, sideBar.themeManager, fallbackThemeManager)
    readonly property var themePalette: sideBar.resolvedThemeManager && sideBar.resolvedThemeManager.currentPaletteCollection ? sideBar.resolvedThemeManager.currentPaletteCollection : null

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

    anchors.left: sideBar.autoDockToParent && parent && sideBar.side === NanSideBar.Side.Left ? parent.left : undefined
    anchors.right: sideBar.autoDockToParent && parent && sideBar.side === NanSideBar.Side.Right ? parent.right : undefined

    width: sideBar.hostWidth

    Behavior on width {
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
        color: sideBar.themePalette ? sideBar.themePalette.secondaryPane : "#2b2b33"
        border.width: 1
        border.color: sideBar.themePalette ? sideBar.themePalette.surfaceElement0 : "#434350"
        opacity: sideBar.hiddenOffcanvas ? 0 : 1

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
                Layout.preferredHeight: headerContent.implicitHeight
                visible: headerContent.implicitHeight > 0

                Column {
                    id: headerContent
                    width: parent.width
                    spacing: sideBar.contentSpacing

                    NanSideBarTrigger {
                        visible: sideBar.showDefaultTrigger
                        sidebar: sideBar
                        themeManager: sideBar.themeManager
                    }

                    Loader {
                        width: parent.width
                        active: sideBar.header !== null
                        sourceComponent: sideBar.header
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
                Layout.preferredHeight: footerContent.implicitHeight
                visible: footerContent.implicitHeight > 0

                Column {
                    id: footerContent
                    width: parent.width
                    spacing: sideBar.contentSpacing

                    Loader {
                        width: parent.width
                        active: sideBar.footer !== null
                        sourceComponent: sideBar.footer
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
        radius: sideBar.railWidth / 2
        color: railArea.containsMouse ? (sideBar.themePalette ? sideBar.themePalette.surfaceElement0 : "#3c3c48") : "transparent"
        anchors {
            top: parent.top
            bottom: parent.bottom
            left: sideBar.side === NanSideBar.Side.Left ? parent.left : undefined
            right: sideBar.side === NanSideBar.Side.Right ? parent.right : undefined
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
        color: edgeArea.pressed ? (sideBar.themePalette ? sideBar.themePalette.overlay1 : "#4c4c58") : (edgeArea.containsMouse ? (sideBar.themePalette ? sideBar.themePalette.surfaceElement0 : "#3b3b46") : (sideBar.themePalette ? sideBar.themePalette.secondaryPane : "#2b2b33"))
        border.width: 1
        border.color: sideBar.themePalette ? sideBar.themePalette.activeBorder : "#6b6b78"
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
        }

        MouseArea {
            id: edgeArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: sideBar.toggle()
        }
    }
}
