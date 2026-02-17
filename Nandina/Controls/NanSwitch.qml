import QtQuick
import QtQuick.Controls as QQC
import Nandina.Theme
import "theme_utils.js" as ThemeUtils

Item {
    id: root

    implicitWidth: control.implicitWidth
    implicitHeight: control.implicitHeight

    property alias checked: control.checked
    property bool disabled: false
    property var themeManager: null

    readonly property bool hovered: control.hovered
    readonly property bool pressed: control.pressed
    readonly property bool focused: control.visualFocus
    readonly property bool entered: hovered
    readonly property bool exited: !hovered

    ThemeManager {
        id: fallbackThemeManager
    }

    readonly property var resolvedThemeManager: ThemeUtils.resolveThemeManager(root, root.themeManager, fallbackThemeManager)

    readonly property var themePalette: root.resolvedThemeManager && root.resolvedThemeManager.currentPaletteCollection
                                      ? root.resolvedThemeManager.currentPaletteCollection : null

    signal toggled(bool checked)

    QQC.Switch {
        id: control
        anchors.fill: parent
        activeFocusOnTab: true
        enabled: !root.disabled

        indicator: Rectangle {
            implicitWidth: 42
            implicitHeight: 24
            radius: height / 2
            color: control.checked
                     ? (root.themePalette ? root.themePalette.activeBorder : "#4f8cff")
                     : (root.themePalette ? root.themePalette.surfaceElement1 : "#3f3f48")
            border.width: control.visualFocus ? 2 : 1
            border.color: control.visualFocus
                        ? (root.themePalette ? root.themePalette.activeBorder : "#4f8cff")
                        : (root.themePalette ? root.themePalette.inactiveBorder : "#666")
            opacity: root.disabled ? 0.5 : 1.0

            Rectangle {
                width: 18
                height: 18
                radius: 9
                anchors.verticalCenter: parent.verticalCenter
                x: control.checked ? parent.width - width - 3 : 3
                color: root.themePalette ? root.themePalette.onAccent : "white"

                Behavior on x {
                    NumberAnimation { duration: 120 }
                }
            }
        }

        onToggled: root.toggled(checked)
    }
}
