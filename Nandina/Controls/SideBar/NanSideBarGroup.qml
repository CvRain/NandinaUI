pragma ComponentBehavior: Bound

import QtQuick
import Nandina.Theme
import "../theme_utils.js" as ThemeUtils

Item {
    id: root

    default property alias contentData: contentColumn.data

    implicitWidth: 200
    implicitHeight: groupColumn.implicitHeight

    property var sidebar: null
    property var themeManager: null
    property string title: ""
    property bool collapsible: false
    property bool expanded: true
    property int spacing: 6

    readonly property var resolvedSidebar: root.sidebar

    readonly property bool collapsed: root.resolvedSidebar ? root.resolvedSidebar.collapsed : false
    readonly property var resolvedThemeManager: ThemeUtils.resolveThemeManager(root, root.themeManager, fallbackThemeManager)
    readonly property var themePalette: {
        if (root.resolvedSidebar && root.resolvedSidebar.themePalette)
            return root.resolvedSidebar.themePalette;
        return root.resolvedThemeManager && root.resolvedThemeManager.currentPaletteCollection ? root.resolvedThemeManager.currentPaletteCollection : null;
    }

    ThemeManager {
        id: fallbackThemeManager
    }

    Column {
        id: groupColumn
        width: parent.width
        spacing: root.spacing

        Item {
            id: headerRow
            width: parent.width
            height: root.collapsed || root.title.length === 0 ? 0 : 22
            visible: height > 0

            Text {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                text: root.title
                color: root.themePalette ? root.themePalette.subHeadlines0 : "#b6b6c4"
                font.pixelSize: 11
                font.weight: Font.Medium
                elide: Text.ElideRight
            }

            Text {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                visible: root.collapsible
                text: root.expanded ? "▾" : "▸"
                color: root.themePalette ? root.themePalette.subHeadlines1 : "#9a9aaa"
                font.pixelSize: 11
            }

            MouseArea {
                anchors.fill: parent
                enabled: root.collapsible
                onClicked: root.expanded = !root.expanded
            }

            Behavior on height {
                NumberAnimation {
                    duration: 140
                    easing.type: Easing.OutCubic
                }
            }
        }

        Item {
            id: contentClip
            width: parent.width
            height: (root.collapsible && !root.expanded) ? 0 : contentColumn.implicitHeight
            clip: true

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.spacing
            }

            Behavior on height {
                NumberAnimation {
                    duration: 180
                    easing.type: Easing.OutCubic
                }
            }
        }
    }
}
