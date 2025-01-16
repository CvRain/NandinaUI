import QtQuick
import Nandina.Color
import Nandina.Utils

Rectangle {
    id: root

    property int level: NandinaType.AlertType.Info
    property string message: "default alert messages"
    property Image icon: null
    property real displayDuration: 3000
    property bool autoClose: true

    readonly property color successColor: "#f6ffed"
    readonly property color successBorderColor: "#b7eb8f"
    readonly property color errorColor: "#fff1f0"
    readonly property color errorBorderColor: "#ffccc7"
    readonly property color warningColor: "#fffbe6"
    readonly property color warningBorderColor: "#ffe58f"
    readonly property color infoColor: "#cbe9ff"
    readonly property color infoBorderColor: "#91caff"
    readonly property color fontColor: "#4c4f69"

    signal closed
    signal showed
    implicitWidth: 550
    implicitHeight: 35
    radius: 5
    color: root.infoColor
    border.color: root.infoBorderColor
    visible: false
    opacity: visible ? 1 : 0
    y: visible ? 0 : -implicitHeight

    NumberAnimation {
        id: opacityAnimation
        target: root
        properties: "opacity"
        duration: 200
        easing.type: Easing.InOutQuad
    }

    NumberAnimation {
        id: fadeOutAnimation
        target: root
        properties: "opacity"
        from: 1
        to: 0
        duration: 200
        easing.type: Easing.InOutQuad
        onRunningChanged: {
            if (!running && root.opacity === 0){
                root.visible = false
            }
        }
    }

    NumberAnimation {
        id: slideInAnimation
        target: root
        properties: "y"
        from: -implicitHeight
        to: 0
        duration: 200
        easing.type: Easing.InOutQuad
    }

    NumberAnimation {
        id: slideOutAnimation
        target: root
        properties: "y"
        from: 0
        to: -implicitHeight
        duration: 200
        easing.type: Easing.InOutQuad
    }

    Timer {
        id: hideTimer
        interval: root.displayDuration
        repeat: false
        running: false
        onTriggered: function(){
            if(root.autoClose === true){
                opacityAnimation.stop()
                fadeOutAnimation.start()
                slideOutAnimation.start()
            }
        }
    }

    Row {
        id: row
        spacing: 10
        width: parent.width * 0.9
        height: parent.height * 0.9
        anchors.centerIn: parent

        Image {
            id: icon
            width: row.width
            height: row.width
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            width: row.width - icon.width
            text: root.message
            font.family: NandinaFont.getFontFamily(NandinaFont.Font_Light)
            font.pixelSize: Math.min(root.width, root.height) * 0.35
            color: root.fontColor
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Component.onCompleted: {
        upateColor();
    }

    function show(duration, autoClose = true) {
        if (duration !== undefined) {
            root.displayDuration = duration;
        }
        upateColor()
        root.visible = true;
        hideTimer.restart()
        opacityAnimation.from = 0;
        opacityAnimation.to = 1;
        opacityAnimation.start()
        slideInAnimation.start()
    }

    function upateColor() {
        let backgroundColor = root.infoColor;
        let borderColor = root.infoBorderColor;

        switch (root.level) {
        case NandinaType.AlertType.Success:
            backgroundColor = root.successColor;
            borderColor = root.successBorderColor;
            break;
        case NandinaType.AlertType.Error:
            backgroundColor = root.errorColor;
            borderColor = root.errorBorderColor;
            break;
        case NandinaType.AlertType.Warning:
            backgroundColor = root.warningColor;
            borderColor = root.warningBorderColor;
            break;
        case NandinaType.AlertType.Info:
            backgroundColor = root.infoColor;
            borderColor = root.infoBorderColor;
            break;
        default:
            break;
        }

        root.color = backgroundColor;
        root.border.color = borderColor;
    }
}
