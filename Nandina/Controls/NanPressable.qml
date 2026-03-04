// NanPressable.qml
// Pure interaction primitive — no visual output, no theme dependency.
// Compose this with a visual Item to add hover/press/click behaviour.
//
// Usage:
//   Item {
//       NanPressable {
//           anchors.fill: parent
//           onClicked: doSomething()
//       }
//   }

import QtQuick

Item {
    id: root

    // ── Interaction state (read-only for consumers) ────────────────
    readonly property bool hovered: _hoverHandler.hovered
    readonly property bool pressed: _tapHandler.pressed

    // ── Configuration ──────────────────────────────────────────────
    property bool enabled: true
    property int cursorShape: Qt.PointingHandCursor

    /// Long-press threshold in milliseconds (0 = disabled).
    property int longPressInterval: 0

    // ── Signals ────────────────────────────────────────────────────
    signal clicked
    signal released
    signal pressStarted
    signal longPressed
    signal canceled

    // ── Internal handlers ──────────────────────────────────────────
    HoverHandler {
        id: _hoverHandler
        enabled: root.enabled
        cursorShape: root.enabled ? root.cursorShape : Qt.ArrowCursor
    }

    TapHandler {
        id: _tapHandler
        enabled: root.enabled

        longPressThreshold: root.longPressInterval > 0 ? (root.longPressInterval / 1000.0) : 0.8     // Qt default, signal only emitted if connected

        onPressedChanged: {
            if (pressed)
                root.pressStarted();
        }
        onTapped: {
            root.clicked();
            root.released();
        }
        onLongPressed: {
            if (root.longPressInterval > 0)
                root.longPressed();
        }
        onCanceled: root.canceled()
    }
}
