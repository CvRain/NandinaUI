import Nandina.Components
import Nandina.Core
import Nandina.Icon
import Nandina.Theme
import Qt5Compat.GraphicalEffects
import QtQuick
import QtQuick.Controls

NanButtonBase {
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
    // 自绘图标枚举，使用 int 以避免自定义类型未注册时的类型错误
    property int vectorIcon: IconManager.ICON_NONE
    // 图标路径
    property int iconPosition: NanButton.IconPosition.Left
    // 图标位置: Left, Right, IconOnly
    property real iconSize: control.height > 0 ? control.height * 0.5 : 20
    // 图标大小（调整为高度的 50%，更舒适的比例）
    property int iconSpacing: 6
    // 图标与文字间距（减小间距使其更紧凑）
    // 文字悬浮提示
    property bool showTooltip: true
    // 是否在文字被截断时显示悬浮提示
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

        var availableWidth = Math.max(0, control.width - control.leftPadding - control.rightPadding);
        var availableHeight = Math.max(0, control.height - control.topPadding - control.bottomPadding);
        // 如果有图标,减去图标占用的空间
        var hasIcon = (iconSource !== "" || vectorIcon !== IconManager.ICON_NONE);
        if (hasIcon && iconPosition !== NanButton.IconPosition.IconOnly)
            availableWidth -= (iconSize + iconSpacing);

        if (availableWidth <= 0 || availableHeight <= 0 || !control.text)
            return minimumFontSize;

        var sizeBasedOnHeight = availableHeight * 0.5;
        // 调整为 50% 以获得更好的视觉效果
        var sizeBasedOnWidth = control.text ? availableWidth / (control.text.length * 0.6) : sizeBasedOnHeight;
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
    implicitWidth: 100
    implicitHeight: 40
    // 调整默认尺寸，参考 shadcn-ui 的 default 按钮尺寸
    padding: 12
    // 增加内边距，让内容与边缘有更舒适的距离
    leftPadding: padding
    rightPadding: padding
    topPadding: padding * 0.6
    // 上下 padding 略小，因为文字和图标通常不需要太多垂直空间
    bottomPadding: padding * 0.6
    font.pointSize: calculatedFontSize
    clip: true // 裁剪超出边界的内容
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

    // 悬浮提示 - 当文字被截断时显示完整内容
    ToolTip {
        // 3秒后自动隐藏

        id: tooltip

        // 只有当文字被截断且启用了 tooltip 时才显示
        visible: control.showTooltip && control.hovered && control.text !== "" && buttonText.truncated && control.iconPosition !== NanButton.IconPosition.IconOnly
        text: control.text
        delay: 500
        // 延迟 500ms 显示，避免鼠标快速划过时频繁弹出
        timeout: 3000
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
    contentItem: Item {
        clip: true // 确保内容不会超出按钮边界
        // 让 Control 能正确测量内容尺寸
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight

        Row {
            id: row

            spacing: control.iconSpacing
            // IconOnly 模式下居中显示，否则根据内容宽度调整
            anchors.centerIn: control.iconPosition === NanButton.IconPosition.IconOnly ? parent : undefined
            anchors.verticalCenter: control.iconPosition !== NanButton.IconPosition.IconOnly ? parent.verticalCenter : undefined
            anchors.left: control.iconPosition !== NanButton.IconPosition.IconOnly ? parent.left : undefined
            anchors.right: control.iconPosition !== NanButton.IconPosition.IconOnly ? parent.right : undefined
            anchors.leftMargin: control.iconPosition !== NanButton.IconPosition.IconOnly ? 0 : undefined
            anchors.rightMargin: control.iconPosition !== NanButton.IconPosition.IconOnly ? 0 : undefined
            // ensure row accounts for all children (icons + text) to avoid clipping
            width: control.iconPosition !== NanButton.IconPosition.IconOnly ? Math.min(childrenRect.width, parent.width) : undefined

            // 左侧图标 (图片)
            Image {
                id: leftIconImage

                visible: control.iconSource.toString() !== "" && control.vectorIcon === IconManager.ICON_NONE && control.iconPosition === NanButton.IconPosition.Left
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

            // 左侧图标 (自绘)
            NanIconItem {
                id: leftIconItem

                // 设置为空字符串以禁用自动主题颜色，改用按钮的 foreground 颜色
                colorRole: ""
                visible: control.vectorIcon !== IconManager.ICON_NONE && control.iconSource.toString() === "" && control.iconPosition === NanButton.IconPosition.Left
                icon: control.vectorIcon
                width: control.iconSize
                height: control.iconSize
                color: control.getInteractiveColor(control.currentForegroundColor)
                anchors.verticalCenter: parent.verticalCenter
                opacity: control.enabled ? 1 : 0.3

                Behavior on color {
                    ColorAnimation {
                        duration: 200
                        easing.type: Easing.OutCubic
                    }

                }

            }

            // 居中图标 (图片)
            Image {
                id: centerIconImage

                visible: control.iconSource.toString() !== "" && control.vectorIcon === IconManager.ICON_NONE && control.iconPosition === NanButton.IconPosition.IconOnly
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

            // 居中图标 (自绘)
            NanIconItem {
                id: centerIconItem

                // 设置为空字符串以禁用自动主题颜色，改用按钮的 foreground 颜色
                colorRole: ""
                visible: control.vectorIcon !== IconManager.ICON_NONE && control.iconSource.toString() === "" && control.iconPosition === NanButton.IconPosition.IconOnly
                icon: control.vectorIcon
                width: control.iconSize
                height: control.iconSize
                color: control.getInteractiveColor(control.currentForegroundColor)
                anchors.verticalCenter: parent.verticalCenter
                opacity: control.enabled ? 1 : 0.3

                Behavior on color {
                    ColorAnimation {
                        duration: 200
                        easing.type: Easing.OutCubic
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
                // 限制文本宽度,避免超出按钮
                width: {
                    var availableWidth = control.width - control.leftPadding - control.rightPadding;
                    var hasIcon = (control.iconSource !== "" || control.vectorIcon !== IconManager.ICON_NONE);
                    if (hasIcon && control.iconPosition !== NanButton.IconPosition.IconOnly)
                        availableWidth -= (control.iconSize + control.iconSpacing);

                    return Math.max(0, availableWidth);
                }

                Behavior on color {
                    ColorAnimation {
                        duration: 200
                        easing.type: Easing.OutCubic
                    }

                }

            }

            // 右侧图标 (图片)
            Image {
                id: rightIconImage

                visible: control.iconSource.toString() !== "" && control.vectorIcon === IconManager.ICON_NONE && control.iconPosition === NanButton.IconPosition.Right
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

            // 右侧图标 (自绘)
            NanIconItem {
                id: rightIconItem

                // 设置为空字符串以禁用自动主题颜色，改用按钮的 foreground 颜色
                colorRole: ""
                visible: control.vectorIcon !== IconManager.ICON_NONE && control.iconSource.toString() === "" && control.iconPosition === NanButton.IconPosition.Right
                icon: control.vectorIcon
                width: control.iconSize
                height: control.iconSize
                color: control.getInteractiveColor(control.currentForegroundColor)
                anchors.verticalCenter: parent.verticalCenter
                opacity: control.enabled ? 1 : 0.3

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
