import QtQuick

Rectangle {
    property int edges: 0
    property var targetWindow: null

    color: "transparent"

    MouseArea {
        anchors.fill: parent
        cursorShape: {
            // 优先级最高：角落光标 (必须首先检查)
            if (edges === (Qt.LeftEdge | Qt.TopEdge)
                    || edges === (Qt.RightEdge | Qt.BottomEdge)) {
                // 左上 (SizeFDiagCursor) 和 右下 (SizeFDiagCursor)
                return Qt.SizeFDiagCursor
            }
            if (edges === (Qt.RightEdge | Qt.TopEdge)
                    || edges === (Qt.LeftEdge | Qt.BottomEdge)) {
                // 右上 (SizeBDiagCursor) 和 左下 (SizeBDiagCursor)
                return Qt.SizeBDiagCursor
            }

            // 优先级其次：边光标 (只包含单边时检查)
            if (edges & Qt.LeftEdge || edges & Qt.RightEdge) {
                return Qt.SizeHorCursor
            }
            if (edges & Qt.TopEdge || edges & Qt.BottomEdge) {
                return Qt.SizeVerCursor
            }

            return Qt.ArrowCursor
        }
    }

    DragHandler {
        target: null
        grabPermissions: PointerHandler.CanTakeOverFromAnything
        onActiveChanged: {
            if (active && parent.targetWindow) {
                parent.targetWindow.startSystemResize(parent.edges)
            }
        }
    }
}
