import QtQuick
import QtQuick.Window

Item {
    id: root

    required property Window targetWindow
    property int resizeMargin: 6
    property int topIgnoreArea: 40

    ResizeHandler {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 1
        anchors.topMargin: 1
        width: root.resizeMargin
        height: root.resizeMargin
        edges: Qt.LeftEdge | Qt.TopEdge
        targetWindow: root.targetWindow
    }

    ResizeHandler {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: root.resizeMargin
        edges: Qt.TopEdge
        targetWindow: root.targetWindow
    }

    ResizeHandler {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 1
        anchors.topMargin: 1
        width: root.resizeMargin
        height: root.resizeMargin
        edges: Qt.RightEdge | Qt.TopEdge
        targetWindow: root.targetWindow
    }

    ResizeHandler {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: root.topIgnoreArea
        width: root.resizeMargin
        edges: Qt.LeftEdge
        targetWindow: root.targetWindow
    }

    ResizeHandler {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: root.topIgnoreArea
        width: root.resizeMargin
        edges: Qt.RightEdge
        targetWindow: root.targetWindow
    }

    ResizeHandler {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 1
        anchors.bottomMargin: 1
        width: root.resizeMargin
        height: root.resizeMargin
        edges: Qt.LeftEdge | Qt.BottomEdge
        targetWindow: root.targetWindow
    }

    ResizeHandler {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: root.resizeMargin
        edges: Qt.BottomEdge
        targetWindow: root.targetWindow
    }

    ResizeHandler {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 1
        anchors.bottomMargin: 1
        width: root.resizeMargin
        height: root.resizeMargin
        edges: Qt.RightEdge | Qt.BottomEdge
        targetWindow: root.targetWindow
    }
}
