import QtQuick
import QtQuick.Controls

Rectangle {
    id: button
    width: 35
    height: 30
    color: "transparent"

    property string iconText: ""
    property bool isCloseButton: false
    property bool isChecked: false
    signal clicked

    Rectangle {
        anchors.centerIn: parent
        width: 20
        height: 20
        radius: 3
        color: {
            if (button.isCloseButton && mouseArea.containsPress)
                return "#e74c3c"
            else if (mouseArea.containsPress)
                return "rgba(255,255,255,0.3)"
            else if (button.isChecked)
                return "rgba(52, 152, 219, 0.5)"
            else
                return "transparent"
        }

        Behavior on color {
            ColorAnimation {
                duration: 150
            }
        }

        Text {
            anchors.centerIn: parent
            text: button.iconText
            color: button.isCloseButton ? "white" : "white"
            font.pixelSize: 12
            font.bold: button.isCloseButton
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor

        onClicked: button.clicked()
    }
}
