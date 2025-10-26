import QtQuick
import QtQuick.Controls.Basic
import NandinaUI

ApplicationWindow {
    id: rootWindow
    width: 640
    height: 480
    visible: true
    title: "Nandina Window"

    Text{
        anchors.centerIn: parent
        text: ThemeManager.getCurrentPaletteType()
    }
}
