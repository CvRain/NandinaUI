import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Nandina.Controls
import Nandina.Theme

Page {
    id: root

    required property ThemeManager themeManager

    background: Rectangle {
        color: root.themeManager.currentPaletteCollection.backgroundPane
    }

    ColumnLayout {
        anchors.fill: parent
        width: parent.width
        height: parent.height
        spacing: 10

        NanInput {}
        NanInput {}
    }
}
