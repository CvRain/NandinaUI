// NanSideBarGroup.qml
// A labelled section within NanSideBar.  Groups can be made collapsible.
//
// ── Basic usage ───────────────────────────────────────────────────────────
//   NanSideBarGroup {
//       title: "Platform"
//       NanSideBarItem { text: "Dashboard"; iconText: "⊞" }
//       NanSideBarItem { text: "Analytics";  iconText: "📈" }
//   }
//
// ── Collapsible group ─────────────────────────────────────────────────────
//   NanSideBarGroup {
//       title: "Projects"
//       collapsible: true
//       expanded: false          // start collapsed
//       NanSideBarItem { ... }
//   }

pragma ComponentBehavior: Bound

import QtQuick
import Nandina.Theme
import Nandina.Controls

Item {
    id: root

    // ── Slots ─────────────────────────────────────────────────────────────
    /// Menu items placed here; reparented into the content column.
    default property alias content: _contentColumn.data

    // ── Header ─────────────────────────────────────────────────────────────
    /// Section label shown above the items.  Hidden when empty or collapsed.
    property string title: ""

    /// When true the group header is clickable and the content can be hidden.
    property bool collapsible: false
    property bool expanded: true

    // ── Appearance ────────────────────────────────────────────────────────
    property real labelPaddingH: ThemeManager.primitives.spacing * 2
    property real groupSpacing: 4   // space between items

    // ── Link to parent sidebar (resolved automatically) ───────────────────
    /// Set this explicitly when the auto-resolution fails.
    property var sidebar: null

    // Resolved reference: walk up the tree to find NanSideBar
    readonly property var _resolvedSidebar: {
        if (root.sidebar)
            return root.sidebar;
        var p = root.parent;
        while (p) {
            if (p.hasOwnProperty("collapsed") && p.hasOwnProperty("side") && p.hasOwnProperty("toggle"))
                return p;
            p = p.parent;
        }
        return null;
    }

    readonly property bool _sidebarCollapsed: _resolvedSidebar ? _resolvedSidebar.collapsed : false
    readonly property bool _isDark: ThemeManager.darkMode
    readonly property color _groupDividerColor: _isDark ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade200
    readonly property color _groupLabelColor: _isDark ? ThemeManager.colors.surface.shade400 : ThemeManager.colors.surface.shade600

    // ── Geometry ──────────────────────────────────────────────────────────
    implicitWidth: parent ? parent.width : 240
    implicitHeight: _outerColumn.implicitHeight
    width: parent ? parent.width : implicitWidth

    Column {
        id: _outerColumn
        width: parent.width
        spacing: 0

        // ── Label row ────────────────────────────────────────────────────
        Item {
            id: _labelRow
            width: parent.width
            // Label is hidden when the sidebar is icon-collapsed or the title is empty
            height: (root.title.length > 0 && !root._sidebarCollapsed) ? 28 : 0
            clip: true
            visible: height > 0

            Behavior on height {
                NumberAnimation {
                    duration: 150
                    easing.type: Easing.OutCubic
                }
            }

            // Underline for collapsible header
            Rectangle {
                visible: root.collapsible
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
                height: 1
                opacity: 0.38
                color: root._groupDividerColor
            }

            // Label text
            Text {
                id: _labelText
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                    right: _chevron.visible ? _chevron.left : parent.right
                    leftMargin: root.labelPaddingH
                    rightMargin: root.labelPaddingH
                }
                text: root.title
                font.family: ThemeManager.primitives.baseFont.fontFamily
                font.pixelSize: Math.round(11 * ThemeManager.primitives.textScaling)
                font.weight: Font.Medium
                color: root._groupLabelColor
                opacity: root._sidebarCollapsed ? 0 : 1
                elide: Text.ElideRight
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.6

                Behavior on opacity {
                    NumberAnimation {
                        duration: 180
                        easing.type: Easing.InOutCubic
                    }
                }
            }

            // Chevron for collapsible
            Text {
                id: _chevron
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: parent.right
                    rightMargin: root.labelPaddingH
                }
                visible: root.collapsible
                text: root.expanded ? "▾" : "▸"
                font.pixelSize: 10
                color: root._isDark ? ThemeManager.colors.surface.shade500 : ThemeManager.colors.surface.shade400
                opacity: root._sidebarCollapsed ? 0 : 1

                Behavior on opacity {
                    NumberAnimation {
                        duration: 180
                        easing.type: Easing.InOutCubic
                    }
                }

                Behavior on rotation {
                    NumberAnimation {
                        duration: 180
                        easing.type: Easing.OutCubic
                    }
                }
            }

            // Invisible expand/collapse tap area
            NanPressable {
                anchors.fill: parent
                enabled: root.collapsible
                onClicked: root.expanded = !root.expanded
            }
        }

        // ── Items container ───────────────────────────────────────────────
        Item {
            id: _contentClip
            width: parent.width
            // When collapsed: 0 height (animated); otherwise natural
            height: (root.collapsible && !root.expanded) ? 0 : _contentColumn.implicitHeight
            clip: true

            Behavior on height {
                NumberAnimation {
                    duration: 200
                    easing.type: Easing.OutCubic
                }
            }

            Column {
                id: _contentColumn
                width: parent.width
                spacing: root.groupSpacing
            }
        }

        // Bottom spacing between groups
        Item {
            width: 1
            height: 8
        }
    }
}
