// NanSideBarTrigger.qml
// A compact button that toggles the sidebar open/closed.
//
// Usage (standalone, explicit sidebar reference):
//   NanSideBarTrigger { sidebar: _mySidebar }
//
// Usage (inside NanSideBar header slot):
//   The built-in trigger inside NanSideBar already uses this component.
//   You can also embed one anywhere by providing a sidebar reference.

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nandina.Theme
import Nandina.Controls

Item {
    id: root

    // Reference to the NanSideBar this trigger controls.
    // If null, the component walks the parent chain automatically.
    property var sidebar: null

    // Optional size override.
    property real buttonSize: 28

    // ── Resolved sidebar ──────────────────────────────────────────────────
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

    readonly property bool _open: _resolvedSidebar ? _resolvedSidebar.open : true
    readonly property bool _leftSided: _resolvedSidebar ? _resolvedSidebar.side !== "right" : true
    readonly property bool _isDark: ThemeManager.darkMode

    // Glyph: points inward (towards sidebar content) when open, outward when closed.
    //   left sidebar open  → ◀   (collapse leftward)
    //   left sidebar closed → ▶  (expand rightward)
    //   right sidebar open  → ▶
    //   right sidebar closed → ◀
    readonly property string _glyph: {
        if (root._leftSided)
            return root._open ? "◀" : "▶";
        return root._open ? "▶" : "◀";
    }

    implicitWidth: root.buttonSize
    implicitHeight: root.buttonSize

    // ── Background ────────────────────────────────────────────────────────
    NanSurface {
        id: _bg
        anchors.fill: parent
        colorVariant: "surface"
        backgroundShade: _pressable.pressed ? (root._isDark ? 600 : 300) : _pressable.hovered ? (root._isDark ? 700 : 200) : (root._isDark ? 800 : 100)
        borderShade: root._isDark ? 600 : 300
        cornerRadius: ThemeManager.primitives.radiusBase

        Behavior on backgroundShade {
            NumberAnimation {
                duration: 100
            }
        }

        // Glyph label
        Text {
            anchors.centerIn: parent
            text: root._glyph
            font.pixelSize: 11
            color: root._isDark ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade600

            Behavior on text {
                // smooth glyph swap via quick opacity flicker
                SequentialAnimation {
                    NumberAnimation {
                        target: parent
                        property: "opacity"
                        to: 0
                        duration: 60
                    }
                    NumberAnimation {
                        target: parent
                        property: "opacity"
                        to: 1.0
                        duration: 60
                    }
                }
            }
        }
    }

    // ── Interaction ────────────────────────────────────────────────────────
    NanPressable {
        id: _pressable
        anchors.fill: parent
        onClicked: {
            if (root._resolvedSidebar)
                root._resolvedSidebar.toggle();
        }
    }

    // ── Tooltip ────────────────────────────────────────────────────────────
    ToolTip {
        visible: _pressable.hovered
        text: root._open ? qsTr("Collapse sidebar") : qsTr("Expand sidebar")
        delay: 500
        font.pixelSize: 12
    }
}
