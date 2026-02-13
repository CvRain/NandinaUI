import QtQuick
import QtQuick.Controls as QQC
import Nandina.Theme
import "theme_utils.js" as ThemeUtils

Item {
    id: root

    implicitWidth: control.implicitWidth
    implicitHeight: control.implicitHeight

    property alias checked: control.checked
    property bool partiallyChecked: false
    property bool disabled: false
    property string text: ""
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

    QQC.CheckBox {
        id: control
        anchors.fill: parent
        text: root.text
        activeFocusOnTab: true
        enabled: !root.disabled
        tristate: root.partiallyChecked

        indicator: Rectangle {
            implicitWidth: 18
            implicitHeight: 18
            radius: 4
            border.width: control.visualFocus ? 2 : 1
            border.color: {
                if (control.checked || control.checkState === Qt.PartiallyChecked)
                    return root.themePalette ? root.themePalette.activeBorder : "#4f8cff"
                return root.themePalette ? root.themePalette.inactiveBorder : "#666"
            }
            color: {
                if (control.checked || control.checkState === Qt.PartiallyChecked)
                    return root.themePalette ? root.themePalette.activeBorder : "#4f8cff"
                return root.themePalette ? root.themePalette.secondaryPane : "#2b2b33"
            }
            opacity: root.disabled ? 0.5 : 1.0

            Text {
                anchors.centerIn: parent
                visible: control.checked || control.checkState === Qt.PartiallyChecked
                text: control.checkState === Qt.PartiallyChecked ? "—" : "✓"
                color: root.themePalette ? root.themePalette.onAccent : "white"
                font.pixelSize: 12
                font.weight: Font.Bold
            }
        }

        contentItem: Text {
            text: control.text
            color: root.themePalette ? root.themePalette.bodyCopy : "#efefef"
            verticalAlignment: Text.AlignVCenter
            leftPadding: control.indicator.width + control.spacing
            font.pixelSize: 13
            opacity: root.disabled ? 0.6 : 1.0
        }

        onToggled: root.toggled(checked)
    }
}
