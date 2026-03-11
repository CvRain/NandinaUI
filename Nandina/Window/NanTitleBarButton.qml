// NanTitleBarButton.qml
// A single title-bar window-control button (minimize / maximize / close).
// Used internally by NanTitleBar.  Can also be embedded in a custom title bar.
//
// Usage:
//   NanTitleBarButton {
//       iconText: "−"; onClicked: window.showMinimized()
//   }
//   NanTitleBarButton {
//       isClose: true; onClicked: window.close()
//   }

import QtQuick
import Nandina.Theme
import Nandina.Controls

Item {
    id: root

    // ── Geometry ───────────────────────────────────────────────────
    implicitWidth: 46
    implicitHeight: 32

    // ── Configuration ──────────────────────────────────────────────
    /// The glyph shown in the button.
    property string iconText: ""

    /// Mark as the close button — gets a red hover background.
    property bool isClose: false

    // ── Signals ────────────────────────────────────────────────────
    signal clicked

    // ── Private colours ────────────────────────────────────────────
    readonly property bool _isDark: ThemeManager.darkMode

    readonly property color _hoverBg: root.isClose ? "#C0392B" : (_isDark ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(0, 0, 0, 0.07))

    readonly property color _pressBg: root.isClose ? "#A93226" : (_isDark ? Qt.rgba(1, 1, 1, 0.18) : Qt.rgba(0, 0, 0, 0.13))

    readonly property color _iconColor: {
        if (root.isClose && (_pressable.hovered || _pressable.pressed))
            return "#ffffff";
        return _isDark ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade600;
    }

    // ── Background ─────────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        radius: ThemeManager.primitives.radiusBase * 0.5
        color: _pressable.pressed ? root._pressBg : (_pressable.hovered ? root._hoverBg : "transparent")
        Behavior on color {
            ColorAnimation {
                duration: 80
            }
        }
    }

    // ── Icon ───────────────────────────────────────────────────────
    Text {
        anchors.centerIn: parent
        text: root.iconText
        font.pixelSize: 12
        color: root._iconColor
        Behavior on color {
            ColorAnimation {
                duration: 80
            }
        }
    }

    // ── Interaction ────────────────────────────────────────────────
    NanPressable {
        id: _pressable
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
