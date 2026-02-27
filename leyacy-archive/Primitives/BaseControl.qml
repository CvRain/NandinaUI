import QtQuick
import Nandina.Theme
import Nandina.Color

FocusScope {
    id: root

    property ThemeManager themeManager: NanStyle.themeManager
    readonly property PaletteCollection themePalette: themeManager ? themeManager.currentPaletteCollection : null

    readonly property bool disabled: !enabled
    readonly property bool interactive: enabled && visible
    property bool hovered: false
    property bool pressed: false
    readonly property bool focused: activeFocus
}
