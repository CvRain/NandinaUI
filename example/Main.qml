import QtQuick
import Nandina.Color

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello World")

    Text {
        id: someText
        text: "Hello world!"
        width: 120
        height: 45

        anchors.centerIn: parent
    }
}
