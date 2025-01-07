import QtQuick
import Nandina
import Nandina.Color

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    TestRectangle {
        color: "red"
    }
}
