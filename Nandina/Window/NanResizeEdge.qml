// NanResizeEdge.qml
// Single-edge/corner resize zone.  Attach a DragHandler for system resize
// and a HoverHandler to show the correct resize cursor.
// Used internally by NanWindowResizer.

import QtQuick
import QtQuick.Window

Item {
    id: root

    // ── Configuration ──────────────────────────────────────────────
    /// Qt edge flags (Qt.LeftEdge, Qt.TopEdge, …) — may be OR-combined.
    property int edges: 0

    /// The window to resize.
    required property Window targetWindow

    // ── Cursor shape derived from edges ────────────────────────────
    readonly property int _cursor: {
        const L = Qt.LeftEdge, R = Qt.RightEdge, T = Qt.TopEdge, B = Qt.BottomEdge;
        if (edges === (L | T) || edges === (R | B))
            return Qt.SizeFDiagCursor;
        if (edges === (R | T) || edges === (L | B))
            return Qt.SizeBDiagCursor;
        if (edges & L || edges & R)
            return Qt.SizeHorCursor;
        if (edges & T || edges & B)
            return Qt.SizeVerCursor;
        return Qt.ArrowCursor;
    }

    HoverHandler {
        cursorShape: root._cursor
    }

    DragHandler {
        target: null
        grabPermissions: PointerHandler.CanTakeOverFromAnything
        onActiveChanged: {
            if (active && root.targetWindow)
                root.targetWindow.startSystemResize(root.edges);
        }
    }
}
