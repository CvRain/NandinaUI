import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Shapes

ApplicationWindow {
    id: rootWindow
    width: 800
    height: 600
    visible: true
    title: "Nandina Window"

    flags: Qt.Window | Qt.FramelessWindowHint

    // 窗口属性
    property bool isMaximized: false
    property bool isAlwaysOnTop: false
    property int titleBarHeight: 40
    property bool isFrameVisible: true
    property int resizeMargin: 8 // 增加边缘区域宽度以获得更好的体验

    default property alias content: contentArea.data

    color: "#f0f0f0"

    // 切换全屏/最大化
    function toggleMaximize() {
        if (isMaximized) {
            showNormal()
            isMaximized = false
        } else {
            showMaximized()
            isMaximized = true
        }
    }

    // 切换置顶
    function toggleAlwaysOnTop() {
        isAlwaysOnTop = !isAlwaysOnTop
        flags = isAlwaysOnTop ? (Qt.Window | Qt.FramelessWindowHint
                                 | Qt.WindowStaysOnTopHint) : (Qt.Window | Qt.FramelessWindowHint)
    }

    // 主内容区域
    Rectangle {
        anchors.fill: parent
        color: "transparent"

        // 自定义标题栏
        Rectangle {
            id: titleBar
            width: parent.width
            height: titleBarHeight
            color: "#2c3e50"
            z: 1 // 确保标题栏在其他元素之上

            // 标题栏拖动区域
            DragHandler {
                target: null
                grabPermissions: PointerHandler.CanTakeOverFromAnything
                onActiveChanged: if (active) rootWindow.startSystemMove()
            }

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
                    text: rootWindow.title
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
                    tooltip: isAlwaysOnTop ? "取消置顶" : "窗口置顶"
                    onClicked: toggleAlwaysOnTop()
                    isChecked: isAlwaysOnTop
                }

                // 最小化按钮
                TitleBarButton {
                    id: btnMinimize
                    iconText: "—"
                    tooltip: "最小化"
                    onClicked: rootWindow.showMinimized()
                }

                // 最大化/还原按钮
                TitleBarButton {
                    id: btnMaximize
                    iconText: isMaximized ? "⧉" : "□"
                    tooltip: isMaximized ? "还原" : "最大化"
                    onClicked: toggleMaximize()
                }

                // 关闭按钮
                TitleBarButton {
                    id: btnClose
                    iconText: "×"
                    tooltip: "关闭"
                    isCloseButton: true
                    onClicked: rootWindow.close()
                }
            }
        }

        // 内容区域
        Item {
            id: contentArea
            anchors.top: titleBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }

        // 窗口边框（可选，用于调整窗口大小）
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.width: 1
            border.color: "#bdc3c7"
            visible: isFrameVisible
        }

        // 使用 DragHandler 进行系统级调整大小
        // 左上角
        DragHandler {
            id: topLeftResize
            target: null
            acceptedDevices: PointerDevice.Mouse
            grabPermissions: PointerHandler.CanTakeOverFromAnything
            onActiveChanged: if (active) rootWindow.startSystemResize(Qt.LeftEdge | Qt.TopEdge)
        }

        Rectangle {
            anchors {
                left: parent.left
                top: parent.top
            }
            width: resizeMargin
            height: resizeMargin
            color: "transparent"
            MouseArea{
                anchors.fill: parent
                cursorShape: Qt.SizeFDiagCursor
            }
            HoverHandler {
                cursorShape: parent.cursorShape
            }
            DragHandler {
                target: null
                grabPermissions: PointerHandler.CanTakeOverFromAnything
                onActiveChanged: if (active) rootWindow.startSystemResize(Qt.LeftEdge | Qt.TopEdge)
            }
        }

        // 上边缘
        DragHandler {
            id: topResize
            target: null
            acceptedDevices: PointerDevice.Mouse
            grabPermissions: PointerHandler.CanTakeOverFromAnything
            onActiveChanged: if (active) rootWindow.startSystemResize(Qt.TopEdge)
        }

        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }
            height: resizeMargin
            color: "transparent"
            MouseArea{
                anchors.fill: parent
                cursorShape: Qt.SizeVerCursor
            }
            HoverHandler {
                cursorShape: parent.cursorShape
            }
            DragHandler {
                target: null
                grabPermissions: PointerHandler.CanTakeOverFromAnything
                onActiveChanged: if (active) rootWindow.startSystemResize(Qt.TopEdge)
            }
        }

        // 右上角
        DragHandler {
            id: topRightResize
            target: null
            acceptedDevices: PointerDevice.Mouse
            grabPermissions: PointerHandler.CanTakeOverFromAnything
            onActiveChanged: if (active) rootWindow.startSystemResize(Qt.RightEdge | Qt.TopEdge)
        }

        Rectangle {
            anchors {
                right: parent.right
                top: parent.top
            }
            width: resizeMargin
            height: resizeMargin
            color: "transparent"
            MouseArea{
                anchors.fill: parent
                cursorShape: Qt.SizeBDiagCursor
            }
            HoverHandler {
                cursorShape: parent.cursorShape
            }
            DragHandler {
                target: null
                grabPermissions: PointerHandler.CanTakeOverFromAnything
                onActiveChanged: if (active) rootWindow.startSystemResize(Qt.RightEdge | Qt.TopEdge)
            }
        }

        // 右边缘
        DragHandler {
            id: rightResize
            target: null
            acceptedDevices: PointerDevice.Mouse
            grabPermissions: PointerHandler.CanTakeOverFromAnything
            onActiveChanged: if (active) rootWindow.startSystemResize(Qt.RightEdge)
        }

        Rectangle {
            anchors {
                right: parent.right
                top: parent.top
                bottom: parent.bottom
            }
            width: resizeMargin
            color: "transparent"
            MouseArea{
                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor
            }
            HoverHandler {
                cursorShape: parent.cursorShape
            }
            DragHandler {
                target: null
                grabPermissions: PointerHandler.CanTakeOverFromAnything
                onActiveChanged: if (active) rootWindow.startSystemResize(Qt.RightEdge)
            }
        }

        // 右下角
        DragHandler {
            id: bottomRightResize
            target: null
            acceptedDevices: PointerDevice.Mouse
            grabPermissions: PointerHandler.CanTakeOverFromAnything
            onActiveChanged: if (active) rootWindow.startSystemResize(Qt.RightEdge | Qt.BottomEdge)
        }

        Rectangle {
            anchors {
                right: parent.right
                bottom: parent.bottom
            }
            width: resizeMargin
            height: resizeMargin
            color: "transparent"
            MouseArea{
                anchors.fill: parent
                cursorShape: Qt.SizeFDiagCursor
            }
            HoverHandler {
                cursorShape: parent.cursorShape
            }
            DragHandler {
                target: null
                grabPermissions: PointerHandler.CanTakeOverFromAnything
                onActiveChanged: if (active) rootWindow.startSystemResize(Qt.RightEdge | Qt.BottomEdge)
            }
        }

        // 下边缘
        DragHandler {
            id: bottomResize
            target: null
            acceptedDevices: PointerDevice.Mouse
            grabPermissions: PointerHandler.CanTakeOverFromAnything
            onActiveChanged: if (active) rootWindow.startSystemResize(Qt.BottomEdge)
        }

        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            height: resizeMargin
            color: "transparent"
            MouseArea{
                anchors.fill: parent
                cursorShape: Qt.SizeVerCursor
            }
            HoverHandler {
                cursorShape: parent.cursorShape
            }
            DragHandler {
                target: null
                grabPermissions: PointerHandler.CanTakeOverFromAnything
                onActiveChanged: if (active) rootWindow.startSystemResize(Qt.BottomEdge)
            }
        }

        // 左下角
        DragHandler {
            id: bottomLeftResize
            target: null
            acceptedDevices: PointerDevice.Mouse
            grabPermissions: PointerHandler.CanTakeOverFromAnything
            onActiveChanged: if (active) rootWindow.startSystemResize(Qt.LeftEdge | Qt.BottomEdge)
        }

        Rectangle {
            anchors {
                left: parent.left
                bottom: parent.bottom
            }
            width: resizeMargin
            height: resizeMargin
            color: "transparent"
            MouseArea{
                anchors.fill: parent
                cursorShape: Qt.SizeBDiagCursor
            }
            HoverHandler {
                cursorShape: parent.cursorShape
            }
            DragHandler {
                target: null
                grabPermissions: PointerHandler.CanTakeOverFromAnything
                onActiveChanged: if (active) rootWindow.startSystemResize(Qt.LeftEdge | Qt.BottomEdge)
            }
        }

        // 左边缘
        DragHandler {
            id: leftResize
            target: null
            acceptedDevices: PointerDevice.Mouse
            grabPermissions: PointerHandler.CanTakeOverFromAnything
            onActiveChanged: if (active) rootWindow.startSystemResize(Qt.LeftEdge)
        }

        Rectangle {
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
            }
            width: resizeMargin
            color: "transparent"
            MouseArea{
                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor
            }

            HoverHandler {
                cursorShape: parent.cursorShape
            }
            DragHandler {
                target: null
                grabPermissions: PointerHandler.CanTakeOverFromAnything
                onActiveChanged: if (active) rootWindow.startSystemResize(Qt.LeftEdge)
            }
        }
    }

    // 窗口状态变化处理
    onVisibilityChanged: {
        if (visibility === Window.Maximized) {
            isMaximized = true
        } else if (visibility === Window.Windowed) {
            isMaximized = false
        }
    }
}
