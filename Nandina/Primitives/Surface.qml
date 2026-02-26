import QtQuick
import Nandina.Theme
import Nandina.Color
import Nandina.Tokens

Rectangle {
    id: root

    property ThemeManager themeManager: NanStyle.themeManager
    readonly property PaletteCollection themePalette: themeManager ? themeManager.currentPaletteCollection : null

    property color backgroundColor: themePalette ? themePalette.base : "#1e1e2e"
    property color borderColor: themePalette ? themePalette.overlay0 : "#45475a"
    property real borderWidth: 1
    property real cornerRadius: NanRadius.Medium

    color: backgroundColor
    border.color: borderColor
    border.width: borderWidth
    radius: root.cornerRadius
}
