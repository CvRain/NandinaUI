import QtQuick
import NandinaUI.Color

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    Component.onCompleted: function () {
        NandinanTheme.printTheme()
    }
}
