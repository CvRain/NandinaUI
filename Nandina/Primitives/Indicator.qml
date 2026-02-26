import QtQuick
import Nandina.Theme
import Nandina.Color
import Nandina.Tokens

Item {
    id: root

    property ThemeManager themeManager: NanStyle.themeManager
    readonly property PaletteCollection themePalette: themeManager ? themeManager.currentPaletteCollection : null

    property bool checked: false
    property bool mixed: false
    property color activeColor: themePalette ? themePalette.accent : "#89b4fa"
    property color inactiveColor: themePalette ? themePalette.overlay0 : "#45475a"

    implicitWidth: NanSpacing.Spacing5
    implicitHeight: NanSpacing.Spacing5

    Rectangle {
        anchors.fill: parent
        radius: NanRadius.Small
        color: root.checked ? root.activeColor : "transparent"
        border.color: root.checked ? root.activeColor : root.inactiveColor
        border.width: 1
    }

    Rectangle {
        visible: root.mixed
        width: parent.width * 0.5
        height: 2
        anchors.centerIn: parent
        radius: 1
        color: root.checked ? (root.themePalette ? root.themePalette.onAccent : "#11111b") : root.inactiveColor
    }
}
