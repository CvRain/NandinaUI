import QtQuick
import QtQuick.Window

Rectangle {
    id: root

    property int edges: 0
    property Window targetWindow: null

    color: "transparent"

    MouseArea {
        anchors.fill: parent
        cursorShape: {
            if (root.edges === (Qt.LeftEdge | Qt.TopEdge)
                    || root.edges === (Qt.RightEdge | Qt.BottomEdge)) {
                return Qt.SizeFDiagCursor
            }
            if (root.edges === (Qt.RightEdge | Qt.TopEdge)
                    || root.edges === (Qt.LeftEdge | Qt.BottomEdge)) {
                return Qt.SizeBDiagCursor
            }
            if (root.edges & Qt.LeftEdge || root.edges & Qt.RightEdge) {
                return Qt.SizeHorCursor
            }
            if (root.edges & Qt.TopEdge || root.edges & Qt.BottomEdge) {
                return Qt.SizeVerCursor
            }
            return Qt.ArrowCursor
        }
    }

    DragHandler {
        target: null
        grabPermissions: PointerHandler.CanTakeOverFromAnything
        onActiveChanged: {
            if (active && root.targetWindow) {
                root.targetWindow.startSystemResize(root.edges)
            }
        }
    }
}
