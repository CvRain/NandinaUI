// NanSideBar.qml
// Main sidebar container.  Compose with NanSideBarGroup / NanSideBarItem.
//
// ── Collapsible modes ──────────────────────────────────────────────────────
//   "icon"      — panel stays visible but shrinks to icon-only width
//   "offcanvas" — panel slides fully off-screen; thin rail remains clickable
//   "none"      — always expanded, no toggle
//
// ── Basic usage ───────────────────────────────────────────────────────────
//   NanSideBar {
//       NanSideBarGroup {
//           title: "Navigation"
//           NanSideBarItem { text: "Home";     iconText: "⌂"; active: true }
//           NanSideBarItem { text: "Settings"; iconText: "⚙" }
//       }
//   }
//
// ── With header / footer ──────────────────────────────────────────────────
//   NanSideBar {
//       header: Item { ... }
//       footer: Item { ... }
//       NanSideBarGroup { ... }
//   }

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nandina.Theme
import Nandina.Controls

Item {
    id: root

    // ── Public API ────────────────────────────────────────────────────────
    /// Which side of the parent the sidebar is docked to.
    /// "left" | "right"
    property string side: "left"

    /// Collapse behaviour.  "icon" | "offcanvas" | "none"
    property string collapsible: "icon"

    /// Whether the sidebar is currently open/expanded.
    property bool open: true

    // ── Dimensions ────────────────────────────────────────────────────────
    property real expandedWidth: 240
    property real collapsedWidth: _iconColW   // meaningful only for "icon" mode
    property real railWidth: 6            // visible rail in offcanvas-hidden state

    // ── Slots ─────────────────────────────────────────────────────────────
    /// Default slot: goes into the scrollable content area.
    default property alias content: _contentColumn.data

    /// Sticky header slot (above scrollable area).
    property Item header: null

    /// Sticky footer slot (below scrollable area).
    property Item footer: null

    // ── Appearance ────────────────────────────────────────────────────────
    /// Show the built-in collapse / expand trigger inside the header area.
    property bool showTrigger: true

    /// Show the thin rail in offcanvas mode (clickable to re-open).
    property bool showRail: true

    // ── Animation ─────────────────────────────────────────────────────────
    property int animationDuration: 280
    property int contentAnimationDuration: 220

    // ── Signals ───────────────────────────────────────────────────────────
    signal toggled(bool open)

    // ── Convenience ───────────────────────────────────────────────────────
    readonly property bool collapsed: collapsible === "icon" && !open
    readonly property bool offcanvasHidden: collapsible === "offcanvas" && !open

    // ── Public methods ────────────────────────────────────────────────────
    function toggle() {
        if (collapsible !== "none")
            root.open = !root.open;
    }
    function expand() {
        root.open = true;
    }
    function collapse() {
        if (collapsible !== "none")
            root.open = false;
    }

    onOpenChanged: root.toggled(root.open)

    // ── Private helpers ───────────────────────────────────────────────────
    // ColorFactory already reverses palette shades in dark mode.
    // Keep semantic shade numbers stable here to avoid double inversion.
    readonly property int _panelBackgroundShade: 100
    readonly property int _panelBorderShade: 300
    readonly property color _dividerColor: ThemeManager.colors.surface.shade300

    // Width of the icon-only strip: big enough for the 32 px icon circle + padding
    readonly property real _iconColW: Math.round(ThemeManager.primitives.spacing * 4   // padding each side
    + 32                                   // icon circle
    + ThemeManager.primitives.spacing * 4)

    // Current panel width (the visual rectangle, may be wider than host item)
    readonly property real _panelWidth: {
        if (collapsible === "none")
            return expandedWidth;
        if (collapsible === "icon")
            return open ? expandedWidth : collapsedWidth;
        return expandedWidth;   // offcanvas: panel always full width, slides via x
    }

    // Host item width (what the sidebar "occupies" in the parent layout)
    width: offcanvasHidden ? (showRail ? railWidth : 0) : _panelWidth
    Behavior on width {
        NumberAnimation {
            duration: root.animationDuration
            easing.type: Easing.InOutCubic
        }
    }
    implicitHeight: parent ? parent.height : 600

    // ── Background panel ──────────────────────────────────────────────────
    NanSurface {
        id: _panel
        width: root._panelWidth
        height: parent.height

        // Offcanvas: panel slides off-screen behind the rail
        x: {
            if (!root.offcanvasHidden)
                return 0;
            return root.side === "left" ? -(root._panelWidth - root.railWidth) : (root._panelWidth - root.railWidth);
        }
        Behavior on x {
            NumberAnimation {
                duration: root.animationDuration
                easing.type: Easing.InOutCubic
            }
        }

        Behavior on opacity {
            NumberAnimation {
                duration: root.contentAnimationDuration
                easing.type: Easing.InOutCubic
            }
        }

        colorVariant: "surface"
        backgroundShade: root._panelBackgroundShade
        borderShade: root._panelBorderShade
        bordered: true
        cornerRadius: 0

        clip: true

        // ── Inner layout ─────────────────────────────────────────────────
        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // ── Header ───────────────────────────────────────────────────
            Item {
                id: _headerSlot
                Layout.fillWidth: true
                implicitHeight: _triggerRow.height + _headerChild.implicitHeight + _headerDivider.implicitHeight

                // Built-in trigger row (always present; shows trigger if enabled,
                // or just a small top-margin spacer otherwise)
                RowLayout {
                    id: _triggerRow
                    width: parent.width
                    height: visible ? 44 : 0
                    visible: root.showTrigger || (root.header !== null)
                    opacity: root.offcanvasHidden ? 0 : 1

                    Behavior on opacity {
                        NumberAnimation {
                            duration: root.contentAnimationDuration
                            easing.type: Easing.InOutCubic
                        }
                    }

                    // Trigger button (NanSideBarTrigger)
                    NanSideBarTrigger {
                        id: _builtinTrigger
                        visible: root.showTrigger
                        sidebar: root
                        Layout.leftMargin: root.side === "left" ? 8 : 0
                        Layout.rightMargin: root.side === "right" ? 8 : 0
                    }

                    Item {
                        Layout.fillWidth: true
                    }
                }

                // Consumer header slot
                Item {
                    id: _headerChild
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: _triggerRow.bottom
                    }
                    implicitHeight: root.header ? root.header.implicitHeight : 0
                    opacity: root.collapsed ? 0 : 1

                    Behavior on opacity {
                        NumberAnimation {
                            duration: root.contentAnimationDuration
                            easing.type: Easing.InOutCubic
                        }
                    }

                    Component.onCompleted: _reparentHeader()
                    onVisibleChanged: _reparentHeader()

                    function _reparentHeader() {
                        if (root.header && root.header.parent !== _headerChild) {
                            root.header.parent = _headerChild;
                            root.header.anchors.fill = _headerChild;
                        }
                    }
                }

                // Divider under header
                Rectangle {
                    id: _headerDivider
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }
                    height: ThemeManager.primitives.divideWidth
                    visible: root.header !== null || root.showTrigger
                    color: root._dividerColor
                }
            }

            // ── Scrollable content ────────────────────────────────────────
            Flickable {
                id: _scroller
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: width
                contentHeight: _contentColumn.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    visible: !root.collapsed
                }
                opacity: root.offcanvasHidden ? 0 : 1

                Behavior on opacity {
                    NumberAnimation {
                        duration: root.contentAnimationDuration
                        easing.type: Easing.InOutCubic
                    }
                }

                Column {
                    id: _contentColumn
                    width: parent.width
                    spacing: 4

                    // top padding
                    Item {
                        width: 1
                        height: Math.round(ThemeManager.primitives.spacing * 1.5)
                    }
                }
            }

            // ── Footer ───────────────────────────────────────────────────
            Item {
                id: _footerSlot
                Layout.fillWidth: true
                implicitHeight: root.footer ? (root.footer.implicitHeight + _footerDivider.height) : 0
                visible: root.footer !== null

                Rectangle {
                    id: _footerDivider
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: parent.top
                    }
                    height: ThemeManager.primitives.divideWidth
                    color: root._dividerColor
                }

                Item {
                    id: _footerChild
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: _footerDivider.bottom
                        bottom: parent.bottom
                    }
                    implicitHeight: root.footer ? root.footer.implicitHeight : 0
                    opacity: root.collapsed ? 0 : 1

                    Behavior on opacity {
                        NumberAnimation {
                            duration: root.contentAnimationDuration
                            easing.type: Easing.InOutCubic
                        }
                    }

                    Component.onCompleted: _reparentFooter()
                    onVisibleChanged: _reparentFooter()

                    function _reparentFooter() {
                        if (root.footer && root.footer.parent !== _footerChild) {
                            root.footer.parent = _footerChild;
                            root.footer.anchors.fill = _footerChild;
                        }
                    }
                }
            }
        }
    }

    // ── Offcanvas rail (click to re-open) ─────────────────────────────────
    Rectangle {
        id: _rail
        visible: root.showRail && root.offcanvasHidden
        width: root.railWidth
        height: parent.height
        anchors {
            top: parent.top
        }
        x: root.side === "left" ? 0 : parent.width - width
        color: _railHover.hovered ? ThemeManager.colors.surface.shade400 : "transparent"

        Behavior on color {
            ColorAnimation {
                duration: 180
            }
        }

        HoverHandler {
            id: _railHover
            cursorShape: Qt.PointingHandCursor
        }
        TapHandler {
            onTapped: root.expand()
        }
    }
}
