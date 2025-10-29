import QtQuick
import Nandina

import QtQuick
import Nandina

Rectangle {
    id: titleBar
    implicitWidth: 640
    implicitHeight: 40

    color: "#2c3e50"
    z: 1 // 确保标题栏在其他元素之上

    property string title: "Nandina"
    property bool isMaximized: false
    property bool isAlwaysOnTop: false
    property int resizeMargin: 6

    signal invokeShowMinimized
    signal invokeToggleMaximize
    signal invokeClose

    // 应用图标和标题
    Row {
        anchors.left: parent.left
        anchors.leftMargin: 15
        anchors.verticalCenter: parent.verticalCenter
        spacing: 10

        Rectangle {
            width: 20
            height: 20
            radius: 4
            color: "#3498db"
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: titleBar.title
            color: "white"
            font.pixelSize: 14
            font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    // 窗口控制按钮
    Row {
        id: controlButtons
        anchors.right: parent.right
        anchors.rightMargin: 5
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        // 置顶按钮
        TitleBarButton {
            id: btnPin
            iconText: "📌"
            tooltip: titleBar.isAlwaysOnTop ? "取消置顶" : "窗口置顶"
            onClicked: titleBar.toggleAlwaysOnTop()
            isChecked: titleBar.isAlwaysOnTop
        }

        // 最小化按钮
        TitleBarButton {
            id: btnMinimize
            iconText: "—"
            tooltip: "最小化"
            onClicked: invokeShowMinimized()
        }

        // 最大化/还原按钮
        TitleBarButton {
            id: btnMaximize
            iconText: titleBar.isMaximized ? "⧉" : "□"
            tooltip: titleBar.isMaximized ? "还原" : "最大化"
            onClicked: invokeToggleMaximize()
        }

        // 关闭按钮
        TitleBarButton {
            id: btnClose
            iconText: "×"
            tooltip: "关闭"
            isCloseButton: true
            onClicked: invokeClose()
        }
    }
}
