import Nandina.Components
import Nandina.Core
import Nandina.Theme
import Qt5Compat.GraphicalEffects
import QtQuick
import QtQuick.Controls.Basic

Button {
    // 图标与文本间距

    id: control

    // 图标位置枚举
    enum IconPosition {
        Left,
        Right,
        IconOnly
    }

    readonly property string componentName: "NanButton"
    property string type: "filledPrimary"
    // 字体自动适应属性
    property bool autoFitText: true
    property real minimumFontSize: 8
    property real maximumFontSize: 72
    property real manualFontSize: 18 // 当 autoFitText 为 false 时使用
    // 图标/图片属性
    property url iconSource: ""
    // 图标路径
    property int iconPosition: NanButton.IconPosition.Left
    // 图标位置: Left, Right, IconOnly
    property real iconSize: 24
    // 图标大小
    property int iconSpacing: 8
    // 缩放动画参数
    property real baseScale: 1
    property real hoverScale: 1.04
    property real pressScale: 0.96
    property real currentScale: baseScale
    property bool isBouncing: false
    property real targetScale: control.down ? pressScale : (control.hovered ? hoverScale : baseScale)
    // 颜色动画相关属性
    property color currentBackgroundColor: buttonStyle().background
    property color currentBorderColor: buttonStyle().border
    property color currentForegroundColor: buttonStyle().foreground
    property real hoverBrightness: 1.15 // 悬浮时颜色变亮 15%
    property real pressBrightness: 0.85 // 按下时颜色变暗 15%
    // 计算字体大小
    property real calculatedFontSize: {
        if (!autoFitText)
            return manualFontSize;

        var availableWidth = Math.max(0, control.width - padding * 2);
        var availableHeight = Math.max(0, control.height - padding * 2);
        // 如果有图标,减去图标占用的空间
        if (iconSource != "" && iconPosition !== NanButton.IconPosition.IconOnly)
            availableWidth -= (iconSize + iconSpacing);

        if (availableWidth <= 0 || availableHeight <= 0 || !control.text)
            return minimumFontSize;

        var sizeBasedOnHeight = availableHeight * 0.4;
        var sizeBasedOnWidth = control.text ? availableWidth / (control.text.length * 0.8) : sizeBasedOnHeight;
        return Math.max(minimumFontSize, Math.min(maximumFontSize, Math.min(sizeBasedOnHeight, sizeBasedOnWidth)));
    }

    // 辅助函数:调整颜色亮度
    function adjustColorBrightness(color, factor) {
        var r = Math.min(255, Math.max(0, color.r * 255 * factor));
        var g = Math.min(255, Math.max(0, color.g * 255 * factor));
        var b = Math.min(255, Math.max(0, color.b * 255 * factor));
        return Qt.rgba(r / 255, g / 255, b / 255, color.a);
    }

    // 获取按钮样式
    function buttonStyle() {
        return ComponentManager.getStyle(control.componentName, control.type);
    }

    // 获取内容颜色(根据交互状态)
    function getInteractiveColor(baseColor) {
        if (control.down)
            return adjustColorBrightness(baseColor, pressBrightness);
        else if (control.hovered)
            return adjustColorBrightness(baseColor, hoverBrightness);
        return baseColor;
    }

    // 交互动画相关
    hoverEnabled: true
    transformOrigin: Item.Center
    scale: currentScale
    text: "Button"
    implicitWidth: 120
    implicitHeight: 60
    font.pointSize: calculatedFontSize
    // 在目标缩放变化时,如果未处于点击动画中,则使用行为动画过渡
    onTargetScaleChanged: {
        if (!isBouncing)
            currentScale = targetScale;

    }
    // 点击时触发果冻动画
    onClicked: {
        if (!isBouncing) {
            isBouncing = true;
            clickBounce.restart();
        }
    }

    // 监听主题变化,更新当前颜色
    Connections {
        function onPaletteChanged() {
            control.currentBackgroundColor = control.buttonStyle().background;
            control.currentBorderColor = control.buttonStyle().border;
            control.currentForegroundColor = control.buttonStyle().foreground;
        }

        target: ThemeManager
    }

    // 点击后的"果冻"回弹动画
    SequentialAnimation {
        id: clickBounce

        running: false
        onStopped: {
            control.isBouncing = false;
            control.currentScale = control.targetScale;
        }

        NumberAnimation {
            target: control
            property: "currentScale"
            to: control.pressScale
            duration: 70
            easing.type: Easing.InQuad
        }

        NumberAnimation {
            target: control
            property: "currentScale"
            to: control.hovered ? control.hoverScale + 0.02 : 1.06
            duration: 140
            easing.type: Easing.OutBack
        }

        NumberAnimation {
            target: control
            property: "currentScale"
            to: control.hovered ? control.hoverScale : control.baseScale
            duration: 120
            easing.type: Easing.OutCubic
        }

    }

    // 常规缩放过渡动画(避免与点击动画冲突)
    Behavior on currentScale {
        enabled: !control.isBouncing

        NumberAnimation {
            duration: 120
            easing.type: Easing.OutCubic
        }

    }

    // 内容项:包含图标和文本的布局
    contentItem: Row {
        spacing: control.iconSpacing
        anchors.centerIn: parent

        // 左侧图标
        Image {
            id: leftIcon

            visible: control.iconSource != "" && control.iconPosition === NanButton.IconPosition.Left
            source: control.iconSource
            width: control.iconSize
            height: control.iconSize
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
            opacity: control.enabled ? 1 : 0.3

            ColorOverlay {
                anchors.fill: parent
                source: parent
                color: control.getInteractiveColor(control.currentForegroundColor)

                Behavior on color {
                    ColorAnimation {
                        duration: 200
                        easing.type: Easing.OutCubic
                    }

                }

            }

        }

        // 居中图标(仅图标模式)
        Image {
            id: centerIcon

            visible: control.iconSource != "" && control.iconPosition === NanButton.IconPosition.IconOnly
            source: control.iconSource
            width: control.iconSize
            height: control.iconSize
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
            opacity: control.enabled ? 1 : 0.3

            ColorOverlay {
                anchors.fill: parent
                source: parent
                color: control.getInteractiveColor(control.currentForegroundColor)

                Behavior on color {
                    ColorAnimation {
                        duration: 200
                        easing.type: Easing.OutCubic
                    }

                }

            }

        }

        // 文本
        Text {
            id: buttonText

            visible: control.iconPosition !== NanButton.IconPosition.IconOnly
            text: control.text
            font: control.font
            opacity: control.enabled ? 1 : 0.3
            color: control.getInteractiveColor(control.currentForegroundColor)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            wrapMode: Text.NoWrap
            anchors.verticalCenter: parent.verticalCenter

            Behavior on color {
                ColorAnimation {
                    duration: 200
                    easing.type: Easing.OutCubic
                }

            }

        }

        // 右侧图标
        Image {
            id: rightIcon

            visible: control.iconSource != "" && control.iconPosition === NanButton.IconPosition.Right
            source: control.iconSource
            width: control.iconSize
            height: control.iconSize
            fillMode: Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
            opacity: control.enabled ? 1 : 0.3

            ColorOverlay {
                anchors.fill: parent
                source: parent
                color: control.getInteractiveColor(control.currentForegroundColor)

                Behavior on color {
                    ColorAnimation {
                        duration: 200
                        easing.type: Easing.OutCubic
                    }

                }

            }

        }

    }

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 40
        opacity: control.enabled ? 1 : 0.3
        radius: 6
        border.width: 1
        border.color: control.getInteractiveColor(control.currentBorderColor)
        color: control.getInteractiveColor(control.currentBackgroundColor)

        Behavior on color {
            ColorAnimation {
                duration: 200
                easing.type: Easing.OutCubic
            }

        }

        Behavior on border.color {
            ColorAnimation {
                duration: 200
                easing.type: Easing.OutCubic
            }

        }

    }

}
