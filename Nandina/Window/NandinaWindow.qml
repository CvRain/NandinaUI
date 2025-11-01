import QtQuick
import QtQuick.Controls.Basic
import Nandina.Theme

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
    property int resizeMargin: 8
    property int windowRadius: 10 // 新增：圆角半径

    default property alias content: contentArea.data

    color: "transparent" // 改为透明，让圆角效果可见

    // 主内容区域 - 添加圆角
    Rectangle {
        id: contentRoot
        anchors.fill: parent
        color: ThemeManager.color.base
        radius: rootWindow.windowRadius // 设置圆角半径
        clip: true // 确保内容不会超出圆角边界

        Behavior on color {

            ColorAnimation {
                duration: 200
                easing.type: Easing.InOutQuad
            }
        }

        // 自定义标题栏
        TitleBar {
            id: titleBar
            width: parent.width
            height: rootWindow.titleBarHeight

            title: rootWindow.title
            radius: rootWindow.windowRadius

            isMaximized: rootWindow.isMaximized
            isAlwaysOnTop: rootWindow.isAlwaysOnTop
            resizeMargin: rootWindow.resizeMargin

            onInvokeClose: rootWindow.close()
            onInvokeShowMinimized: rootWindow.showMinimized()
            onInvokeToggleMaximize: rootWindow.showMaximized()

            // 标题栏拖动区域
            DragHandler {
                id: dragHandler
                target: null
                grabPermissions: PointerHandler.CanTakeOverFromAnything
            }

            Connections {
                target: dragHandler
                function onActiveChanged() {
                    if (dragHandler.active) {
                        rootWindow.startSystemMove()
                    }
                }
            }
        }

        // 内容区域（不再响应缩放拖拽）
        Item {
            id: contentArea
            anchors.top: titleBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }

        // 窗口边框（可选，用于调整窗口大小）- 也需要圆角
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.width: 1
            border.color: ThemeManager.color.surface0
            visible: rootWindow.isFrameVisible
            radius: rootWindow.windowRadius // 边框也要圆角
        }
    }

    // 窗口缩放区域 - 需要调整以适应圆角
    WindowResizer {
        anchors.fill: parent
        targetWindow: rootWindow
        resizeMargin: rootWindow.resizeMargin
        topIgnoreArea: rootWindow.titleBarHeight
        visible: !rootWindow.isMaximized

        // 在最大化时隐藏，避免圆角区域的问题
    }

    // 窗口状态变化处理
    onVisibilityChanged: {
        if (visibility === Window.Maximized) {
            isMaximized = true
            // 最大化时移除圆角
            contentRoot.radius = 0
        } else if (visibility === Window.Windowed) {
            isMaximized = false
            // 恢复窗口时重新启用圆角
            contentRoot.radius = rootWindow.windowRadius
        }
    }

    // 切换全屏/最大化
    function toggleMaximize() {
        if (isMaximized) {
            showNormal()
            isMaximized = false
            contentRoot.radius = rootWindow.windowRadius // 恢复圆角
        } else {
            showMaximized()
            isMaximized = true
            contentRoot.radius = 0 // 最大化时移除圆角
        }
    }

    // 切换置顶
    function toggleAlwaysOnTop() {
        isAlwaysOnTop = !isAlwaysOnTop
        flags = isAlwaysOnTop ? (Qt.Window | Qt.FramelessWindowHint
                                 | Qt.WindowStaysOnTopHint) : (Qt.Window | Qt.FramelessWindowHint)
    }
}
