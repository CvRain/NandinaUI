import QtQuick
import QtQuick.Window

Item {
    id: resizer
    anchors.fill: parent

    required property Window targetWindow

    property int resizeMargin: 6

    property int topIgnoreArea: 40

    // 1. 左上角 (Corner: Top-Left)
    ResizeHandler {
        anchors {
            left: parent.left
            top: parent.top
        }
        width: resizer.resizeMargin
        height: resizer.resizeMargin
        edges: Qt.LeftEdge | Qt.TopEdge
        targetWindow: resizer.targetWindow

        //为适配圆角微调区域
        anchors.leftMargin: 1
        anchors.topMargin: 1
    }

    // 2. 上边缘 (Edge: Top)
    ResizeHandler {
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        height: resizer.resizeMargin
        edges: Qt.TopEdge
        targetWindow: resizer.targetWindow
    }

    // 3. 右上角 (Corner: Top-Right)
    ResizeHandler {
        anchors {
            right: parent.right
            top: parent.top
        }
        width: resizer.resizeMargin
        height: resizer.resizeMargin
        edges: Qt.RightEdge | Qt.TopEdge
        targetWindow: resizer.targetWindow

        //为适配圆角微调区域
        anchors.rightMargin: 1
        anchors.topMargin: 1
    }

    // 4. 左边缘 (Edge: Left)
    ResizeHandler {
        // 从标题栏下方开始
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        width: resizer.resizeMargin
        // 避开顶部区域
        anchors.topMargin: resizer.topIgnoreArea
        edges: Qt.LeftEdge
        targetWindow: resizer.targetWindow
    }

    // 5. 右边缘 (Edge: Right)
    ResizeHandler {
        // 从标题栏下方开始
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        width: resizer.resizeMargin
        // 避开顶部区域
        anchors.topMargin: resizer.topIgnoreArea
        edges: Qt.RightEdge
        targetWindow: resizer.targetWindow
    }

    // 6. 左下角 (Corner: Bottom-Left)
    ResizeHandler {
        anchors {
            left: parent.left
            bottom: parent.bottom
        }
        width: resizer.resizeMargin
        height: resizer.resizeMargin
        edges: Qt.LeftEdge | Qt.BottomEdge
        targetWindow: resizer.targetWindow

        //为适配圆角微调区域
        anchors.leftMargin: 1
        anchors.bottomMargin: 1
    }

    // 7. 下边缘 (Edge: Bottom)
    ResizeHandler {
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: resizer.resizeMargin
        edges: Qt.BottomEdge
        targetWindow: resizer.targetWindow
    }

    // 8. 右下角 (Corner: Bottom-Right)
    ResizeHandler {
        anchors {
            right: parent.right
            bottom: parent.bottom
        }
        width: resizer.resizeMargin
        height: resizer.resizeMargin
        edges: Qt.RightEdge | Qt.BottomEdge
        targetWindow: resizer.targetWindow

        //为适配圆角微调区域
        anchors.rightMargin: 1
        anchors.bottomMargin: 1
    }
}
