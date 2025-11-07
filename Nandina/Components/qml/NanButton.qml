import Nandina.Core
import Nandina.Theme
import Nandina.Components
import QtQuick
import QtQuick.Controls.Basic

Button {
    id: control

    property string type: "filledPrimary"
    property real minimumFontSize: 8
    property real maximumFontSize: 72
    property bool autoFitText: true
    // 基础缩放参数
    property real baseScale: 1
    property real hoverScale: 1.04
    property real pressScale: 0.96
    // 当前缩放值（用于绑定到 control.scale）
    property real currentScale: baseScale
    // 是否处于点击“果冻”动画中
    property bool isBouncing: false
    // 根据交互状态计算目标缩放值
    property real targetScale: control.down ? pressScale : (control.hovered ? hoverScale : baseScale)
    // 使用更安全的方式计算字体大小
    property real calculatedFontSize: {
        if (!autoFitText)
            return 18

        // 默认字体大小
        var availableWidth = Math.max(0, control.width - padding * 2)
        var availableHeight = Math.max(0, control.height - padding * 2)
        // 避免除零和负数情况
        if (availableWidth <= 0 || availableHeight <= 0 || !control.text)
            return minimumFontSize

        // 基于按钮高度确定字体大小
        var sizeBasedOnHeight = availableHeight * 0.4
        // 基于按钮宽度和文本长度确定字体大小
        var sizeBasedOnWidth = control.text ? availableWidth / (control.text.length
                                                                * 0.8) : sizeBasedOnHeight
        // 取两者中的较小值，并限制在最小和最大字体大小之间
        return Math.max(minimumFontSize, Math.min(maximumFontSize,
                                                  Math.min(sizeBasedOnHeight,
                                                           sizeBasedOnWidth)))
    }

    // 交互动画相关
    hoverEnabled: true
    transformOrigin: Item.Center
    // 在目标缩放变化时，如果未处于点击动画中，则使用行为动画过渡
    onTargetScaleChanged: {
        if (!isBouncing)
            currentScale = targetScale
    }
    // 将控件整体缩放绑定到 currentScale
    scale: currentScale
    text: "Button"
    implicitWidth: 120
    implicitHeight: 60
    font.pointSize: calculatedFontSize
    // 点击时触发果冻动画
    onClicked: {
        if (!isBouncing) {
            isBouncing = true
            clickBounce.restart()
        }
    }

    // 点击后的“果冻”回弹动画
    SequentialAnimation {
        id: clickBounce

        running: false
        onStopped: {
            control.isBouncing = false
            // 动画结束后与当前交互状态对齐（悬停时停在放大状态）
            control.currentScale = control.targetScale
        }

        // 先轻微压缩
        NumberAnimation {
            target: control
            property: "currentScale"
            to: control.pressScale
            duration: 70
            easing.type: Easing.InQuad
        }

        // 再略微超过目标值（产生果冻感）
        NumberAnimation {
            target: control
            property: "currentScale"
            to: control.hovered ? control.hoverScale + 0.02 : 1.06
            duration: 140
            easing.type: Easing.OutBack
        }

        // 最后回到目标（悬停或基础）
        NumberAnimation {
            target: control
            property: "currentScale"
            to: control.hovered ? control.hoverScale : control.baseScale
            duration: 120
            easing.type: Easing.OutCubic
        }
    }

    // 常规缩放过渡动画（避免与点击动画冲突）
    Behavior on currentScale {
        enabled: !control.isBouncing

        NumberAnimation {
            duration: 120
            easing.type: Easing.OutCubic
        }
    }

    contentItem: Text {
        text: control.text
        font: control.font
        opacity: enabled ? 1 : 0.3
        color: ComponentManager.getButtonStyle(control.type).foreground
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        // 文本在按钮内居中
        anchors.centerIn: parent
        anchors.margins: control.padding
        // 当文本过大时显示省略号
        wrapMode: Text.NoWrap
    }

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 40
        opacity: enabled ? 1 : 0.3
        //border.color: control.down ? "#17a81a" : "#21be2b"
        border.color: ComponentManager.getButtonStyle(control.type).border
        color: ComponentManager.getButtonStyle(control.type).background
        border.width: 1
        radius: 6
    }
}
