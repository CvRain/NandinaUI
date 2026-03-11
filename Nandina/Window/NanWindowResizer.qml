// NanWindowResizer.qml
// Overlays 8 NanResizeEdge zones (4 edges + 4 corners) over a frameless
// window to enable system-native drag-to-resize on all sides.
//
// Usage inside NanWindow:
//   NanWindowResizer {
//       anchors.fill: parent
//       targetWindow: root  // the ApplicationWindow
//       resizeMargin: 6
//       topIgnoreHeight: titleBarHeight  // skip title bar drag region
//   }

import QtQuick
import QtQuick.Window

Item {
    id: root

    // ── Configuration ──────────────────────────────────────────────
    required property Window targetWindow

    /// Pixel thickness of each resize zone.
    property int resizeMargin: 6

    /// Vertical area at the top that belongs to the title bar drag region
    /// and should NOT trigger resize on the left/right edges.
    property int topIgnoreHeight: 40

    // ── Corners ────────────────────────────────────────────────────
    NanResizeEdge {
        anchors {
            left: parent.left
            top: parent.top
            leftMargin: 1
            topMargin: 1
        }
        width: root.resizeMargin
        height: root.resizeMargin
        edges: Qt.LeftEdge | Qt.TopEdge
        targetWindow: root.targetWindow
    }
    NanResizeEdge {
        anchors {
            right: parent.right
            top: parent.top
            rightMargin: 1
            topMargin: 1
        }
        width: root.resizeMargin
        height: root.resizeMargin
        edges: Qt.RightEdge | Qt.TopEdge
        targetWindow: root.targetWindow
    }
    NanResizeEdge {
        anchors {
            left: parent.left
            bottom: parent.bottom
            leftMargin: 1
            bottomMargin: 1
        }
        width: root.resizeMargin
        height: root.resizeMargin
        edges: Qt.LeftEdge | Qt.BottomEdge
        targetWindow: root.targetWindow
    }
    NanResizeEdge {
        anchors {
            right: parent.right
            bottom: parent.bottom
            rightMargin: 1
            bottomMargin: 1
        }
        width: root.resizeMargin
        height: root.resizeMargin
        edges: Qt.RightEdge | Qt.BottomEdge
        targetWindow: root.targetWindow
    }

    // ── Edges ──────────────────────────────────────────────────────
    NanResizeEdge {
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        height: root.resizeMargin
        edges: Qt.TopEdge
        targetWindow: root.targetWindow
    }
    NanResizeEdge {
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: root.resizeMargin
        edges: Qt.BottomEdge
        targetWindow: root.targetWindow
    }
    NanResizeEdge {
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
            topMargin: root.topIgnoreHeight
        }
        width: root.resizeMargin
        edges: Qt.LeftEdge
        targetWindow: root.targetWindow
    }
    NanResizeEdge {
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
            topMargin: root.topIgnoreHeight
        }
        width: root.resizeMargin
        edges: Qt.RightEdge
        targetWindow: root.targetWindow
    }
}
