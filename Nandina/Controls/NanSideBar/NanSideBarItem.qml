// NanSideBarItem.qml
// Individual navigation item inside NanSideBarGroup.
//
// When the parent sidebar is icon-collapsed, the text label and badge hide
// and the icon centres in the strip.  A tooltip shows the label in that state.
//
// ── Sub-items (expandable) ─────────────────────────────────────────────────────
//   NanSideBarItem {
//       text: "Settings"
//       iconText: "⚙"
//       active: true
//       NanSideBarItem { text: "General"; isSubItem: true }
//       NanSideBarItem { text: "Security"; isSubItem: true }
//   }
//
// ── Badge ─────────────────────────────────────────────────────────────────────
//   NanSideBarItem {
//       text: "Messages"
//       iconText: "✉"
//       badge: "12"
//   }

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nandina.Theme
import Nandina.Controls

Item {
    id: root

    // ── Content ────────────────────────────────────────────────────────────
    property string text: ""
    /// Unicode glyph shown in the icon circle (e.g. "⚙", "⌂").
    property string iconText: ""
    /// Image URL for the icon (takes precedence over iconText when non-empty).
    property url iconSource: ""
    /// Short badge text shown on the right (number or label).
    property string badge: ""

    // ── State ──────────────────────────────────────────────────────────────
    property bool active: false
    property bool enabled: true

    // ── Sub-items ──────────────────────────────────────────────────────────
    /// Whether this is a second-level item (indented, no icon circle).
    property bool isSubItem: false
    /// Sub-items placed as direct children will be collected here.
    default property alias subContent: _subColumn.data
    property bool subExpanded: active   // auto-open when active

    // ── Appearance ────────────────────────────────────────────────────────
    property real itemHeight: 36
    property real subItemHeight: 30
    property real hPadding: ThemeManager.primitives.spacing * 2
    property real iconCircleSize: 28

    // ── Sidebar link ──────────────────────────────────────────────────────
    property var sidebar: null

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
    readonly property bool _rightSided: _resolvedSidebar ? _resolvedSidebar.side === "right" : false
    readonly property bool _isDark: ThemeManager.darkMode
    readonly property bool _hasIcon: root.iconSource.toString().length > 0 || root.iconText.length > 0
    readonly property bool _hasSubItems: _subColumn.children.length > 0
    readonly property bool _hasBadge: root.badge.length > 0
    readonly property color _activeBarColor: root._isDark ? ThemeManager.colors.primary.shade300 : ThemeManager.colors.primary.shade500

    // Signals
    signal clicked
    signal pressStarted

    // ── Geometry ──────────────────────────────────────────────────────────
    implicitWidth: parent ? parent.width : 240
    implicitHeight: _mainRow.height + _subClip.height
    width: parent ? parent.width : implicitWidth

    // ── Colours ───────────────────────────────────────────────────────────
    readonly property color _bgColor: {
        if (!root.enabled)
            return "transparent";
        if (_pressable.pressed)
            return root._isDark ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade200;
        if (root.active)
            return root._isDark ? ThemeManager.colors.surface.shade900 : ThemeManager.colors.primary.shade100;
        if (_pressable.hovered)
            return root._isDark ? ThemeManager.colors.surface.shade800 : ThemeManager.colors.surface.shade100;
        return "transparent";
    }

    readonly property color _textColor: {
        if (!root.enabled)
            return root._isDark ? ThemeManager.colors.surface.shade600 : ThemeManager.colors.surface.shade400;
        if (root.active)
            return root._isDark ? ThemeManager.colors.primary.shade200 : ThemeManager.colors.primary.shade700;
        return root._isDark ? ThemeManager.colors.surface.shade200 : ThemeManager.colors.surface.shade700;
    }

    readonly property color _iconBgColor: {
        if (root.active)
            return root._isDark ? ThemeManager.colors.primary.shade800 : ThemeManager.colors.primary.shade100;
        return root._isDark ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade100;
    }

    readonly property color _badgeBgColor: {
        if (root.active)
            return root._isDark ? ThemeManager.colors.primary.shade600 : ThemeManager.colors.primary.shade500;
        return root._isDark ? ThemeManager.colors.surface.shade600 : ThemeManager.colors.surface.shade200;
    }

    // ── Main row ──────────────────────────────────────────────────────────
    Rectangle {
        id: _mainRow
        width: parent.width
        height: root.isSubItem ? root.subItemHeight : root.itemHeight
        radius: ThemeManager.primitives.radiusBase
        color: root._bgColor

        Behavior on color {
            ColorAnimation {
                duration: 100
            }
        }

        // Active indicator bar  ────────────────────────────────────────────
        Rectangle {
            id: _activeBar
            width: 3
            radius: 2
            color: root._activeBarColor
            anchors.verticalCenter: parent.verticalCenter
            // stick to the inner edge of the sidebar
            x: root._rightSided ? parent.width - width - 2 : 2
            height: root.active ? 18 : 0
            opacity: root.active ? 1.0 : 0.0

            Behavior on height {
                NumberAnimation {
                    duration: 150
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on opacity {
                NumberAnimation {
                    duration: 120
                }
            }
        }

        // Row interior ────────────────────────────────────────────────────
        RowLayout {
            anchors {
                fill: parent
                leftMargin: root.isSubItem ? (root._rightSided ? root.hPadding : root.hPadding + root.iconCircleSize + root.hPadding) : root.hPadding
                rightMargin: root.hPadding
            }
            spacing: root.isSubItem ? 6 : root.hPadding * 0.6

            // ── Icon circle (hidden for sub-items) ────────────────────────
            Rectangle {
                id: _iconCircle
                visible: !root.isSubItem && root._hasIcon
                implicitWidth: root.iconCircleSize
                implicitHeight: root.iconCircleSize
                radius: root.iconCircleSize / 2
                color: root._iconBgColor
                Layout.alignment: Qt.AlignVCenter

                Behavior on color {
                    ColorAnimation {
                        duration: 120
                    }
                }

                // X position animates when collapsing (centres in the strip)
                Behavior on Layout.leftMargin {
                    NumberAnimation {
                        duration: 180
                        easing.type: Easing.InOutSine
                    }
                }

                Image {
                    anchors.centerIn: parent
                    width: 16
                    height: 16
                    source: root.iconSource
                    visible: root.iconSource.toString().length > 0
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    asynchronous: true
                }

                Text {
                    anchors.centerIn: parent
                    text: root.iconText
                    visible: root.iconSource.toString().length === 0
                    font.pixelSize: 13
                    color: root.active ? (root._isDark ? ThemeManager.colors.primary.shade200 : ThemeManager.colors.primary.shade600) : (root._isDark ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade600)
                }
            }

            // Sub-item dot indicator ───────────────────────────────────────
            Rectangle {
                visible: root.isSubItem
                implicitWidth: 6
                implicitHeight: 6
                radius: 3
                color: root.active ? (root._isDark ? ThemeManager.colors.primary.shade400 : ThemeManager.colors.primary.shade500) : (root._isDark ? ThemeManager.colors.surface.shade600 : ThemeManager.colors.surface.shade400)
                Layout.alignment: Qt.AlignVCenter
            }

            // ── Label ─────────────────────────────────────────────────────
            Text {
                id: _label
                Layout.fillWidth: true
                visible: !root._sidebarCollapsed
                text: root.text
                font.family: ThemeManager.primitives.baseFont.fontFamily
                font.pixelSize: root.isSubItem ? Math.round(12 * ThemeManager.primitives.textScaling) : Math.round(13 * ThemeManager.primitives.textScaling)
                font.weight: root.active ? Font.Medium : Font.Normal
                color: root._textColor
                elide: Text.ElideRight

                Behavior on opacity {
                    NumberAnimation {
                        duration: 120
                    }
                }
            }

            // ── Chevron (only if has sub-items) ────────────────────────────
            Text {
                visible: root._hasSubItems && !root._sidebarCollapsed
                text: root.subExpanded ? "▾" : "▸"
                font.pixelSize: 10
                color: root._isDark ? ThemeManager.colors.surface.shade500 : ThemeManager.colors.surface.shade400
                Layout.alignment: Qt.AlignVCenter
            }

            // ── Badge ──────────────────────────────────────────────────────
            Rectangle {
                visible: root._hasBadge && !root._sidebarCollapsed
                implicitWidth: Math.max(_badgeLabel.implicitWidth + 8, 20)
                implicitHeight: 18
                radius: 9
                color: root._badgeBgColor
                Layout.alignment: Qt.AlignVCenter

                Text {
                    id: _badgeLabel
                    anchors.centerIn: parent
                    text: root.badge
                    font.pixelSize: 10
                    font.weight: Font.Medium
                    color: root.active ? "#ffffff" : (root._isDark ? ThemeManager.colors.surface.shade200 : ThemeManager.colors.surface.shade700)
                }
            }
        }

        // ── Tooltip in icon-only mode ──────────────────────────────────────
        ToolTip {
            visible: root._sidebarCollapsed && _pressable.hovered && root.text.length > 0
            text: root.text
            delay: 400
            font.pixelSize: 12
        }

        // ── Interaction ────────────────────────────────────────────────────
        NanPressable {
            id: _pressable
            anchors.fill: parent
            enabled: root.enabled
            onClicked: {
                if (root._hasSubItems)
                    root.subExpanded = !root.subExpanded;
                root.clicked();
            }
            onPressStarted: root.pressStarted()
        }
    }

    // ── Sub-items container ────────────────────────────────────────────────
    Item {
        id: _subClip
        anchors.top: _mainRow.bottom
        width: parent.width
        height: (root._hasSubItems && root.subExpanded && !root._sidebarCollapsed) ? _subColumn.implicitHeight : 0
        clip: true

        Behavior on height {
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutCubic
            }
        }

        Column {
            id: _subColumn
            width: parent.width
            spacing: 2
            topPadding: 2
        }
    }
}
