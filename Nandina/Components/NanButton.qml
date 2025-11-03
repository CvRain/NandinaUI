import Nandina.Core
import Nandina.Theme
import QtQuick
import QtQuick.Controls.Basic

Button {
    id: control

    property string type: "FilledPrimary"
    property real minimumFontSize: 8
    property real maximumFontSize: 72
    property bool autoFitText: true

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

    text: "Button"
    implicitWidth: 120
    implicitHeight: 60
    font.pointSize: calculatedFontSize

    contentItem: Text {
        text: control.text
        font: control.font
        opacity: enabled ? 1 : 0.3
        color: ThemeManager.getButtonStyle(control.type).foreground
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
        border.color: ThemeManager.getButtonStyle(control.type).border
        color: ThemeManager.getButtonStyle(control.type).background
        border.width: 1
        radius: 6
    }
}
