import QtQuick
import QtQuick.Controls

/**
 * NanButtonBase - 基于 Control 的按钮基类
 * 提供基础的按钮交互逻辑，但不包含具体样式
 */
Control {
    // ============================================
    // 公共 API - 属性
    // ============================================
    // ============================================
    // 内部属性
    // ============================================
    // ============================================
    // 默认配置
    // ============================================
    // ============================================
    // 键盘支持
    // ============================================
    // ============================================
    // 鼠标交互区域
    // ============================================
    // ============================================
    // 自动重复计时器
    // ============================================
    // ============================================
    // 状态管理
    // ============================================

    id: control

    // 基础状态属性
    property alias down: mouseArea.pressed
    property alias containsPress: mouseArea.containsPress
    // 文本属性（兼容 Button API）
    property string text: ""
    property font font
    // 交互行为配置
    property bool autoRepeat: false
    property int autoRepeatDelay: 300
    property int autoRepeatInterval: 100
    // 视觉反馈
    property bool highlighted: false
    property bool flat: false
    // 用于自动重复点击
    property bool __autoRepeatActive: false

    // ============================================
    // 公共 API - 信号
    // ============================================
    signal clicked()
    signal pressed()
    signal released()
    signal pressAndHold()
    signal doubleClicked()
    signal canceled()

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
    hoverEnabled: true
    activeFocusOnTab: true
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            event.accepted = true;
            mouseArea.pressed = true;
            control.pressed();
        }
    }
    Keys.onReleased: (event) => {
        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            event.accepted = true;
            if (mouseArea.pressed) {
                mouseArea.pressed = false;
                control.released();
                control.clicked();
            }
        }
    }
    states: [
        State {
            name: "disabled"
            when: !control.enabled
        },
        State {
            name: "pressed"
            when: control.enabled && control.down
        },
        State {
            name: "hovered"
            when: control.enabled && control.hovered && !control.down
        },
        State {
            name: "normal"
            when: control.enabled && !control.hovered && !control.down
        }
    ]

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        acceptedButtons: Qt.LeftButton
        onClicked: {
            control.forceActiveFocus(Qt.MouseFocusReason);
            control.clicked();
        }
        onPressed: {
            control.forceActiveFocus(Qt.MouseFocusReason);
            control.pressed();
            if (control.autoRepeat)
                autoRepeatTimer.start();

        }
        onReleased: {
            control.released();
            if (control.autoRepeat) {
                autoRepeatTimer.stop();
                control.__autoRepeatActive = false;
            }
        }
        onPressAndHold: {
            control.pressAndHold();
        }
        onDoubleClicked: {
            control.doubleClicked();
        }
        onCanceled: {
            control.canceled();
            if (control.autoRepeat) {
                autoRepeatTimer.stop();
                control.__autoRepeatActive = false;
            }
        }
        onContainsMouseChanged: {
            control.hovered = containsMouse;
        }
    }

    Timer {
        id: autoRepeatTimer

        interval: control.__autoRepeatActive ? control.autoRepeatInterval : control.autoRepeatDelay
        repeat: true
        running: false
        onTriggered: {
            control.__autoRepeatActive = true;
            control.clicked();
        }
    }

}
