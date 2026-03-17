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
import Nandina.Theme
import Nandina.Controls
import Nandina.Types

Item {
    id: root

    readonly property var _colorVariantTypes: ThemeVariant.ColorVariantTypes || ({})
    readonly property int _colorSurface: _colorVariantTypes.Surface ?? 6

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
    readonly property int _backgroundShade: _pressable.pressed ? 300 : _pressable.hovered ? 200 : 100
    readonly property int _borderShade: 300

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
        colorVariant: root._colorSurface
        backgroundShade: root._backgroundShade
        borderShade: root._borderShade
        cornerRadius: ThemeManager.primitives.radiusBase

        Behavior on backgroundShade {
            NumberAnimation {
                duration: 100
            }
        }

        // Glyph label
        Text {
            id: _glyphText
            anchors.centerIn: parent
            text: root._glyph
            font.pixelSize: 11
            color: ThemeManager.colors.surface.shade600

            Behavior on text {
                // smooth glyph swap via quick opacity flicker
                SequentialAnimation {
                    NumberAnimation {
                        target: _glyphText
                        property: "opacity"
                        to: 0
                        duration: 60
                    }
                    NumberAnimation {
                        target: _glyphText
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
