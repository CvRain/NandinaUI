import QtQuick
import QtQuick.Controls as QQC
import Nandina.Theme
import Nandina.Tokens
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
    property int indicatorSize: NanSpacing.lg + 2
    property int indicatorRadius: NanSpacing.xs
    property font textFont: NanTypography.body
    property font markFont: Qt.font({
        pixelSize: NanTypography.caption.pixelSize,
        weight: Font.Bold
    })

    readonly property bool hovered: control.hovered
    readonly property bool pressed: control.pressed
    readonly property bool focused: control.visualFocus
    readonly property bool entered: hovered
    readonly property bool exited: !hovered

    ThemeManager {
        id: fallbackThemeManager
    }

    readonly property var resolvedThemeManager: ThemeUtils.resolveThemeManager(root, root.themeManager, fallbackThemeManager)

    readonly property var themePalette: root.resolvedThemeManager && root.resolvedThemeManager.currentPaletteCollection ? root.resolvedThemeManager.currentPaletteCollection : null

    signal toggled(bool checked)

    QQC.CheckBox {
        id: control
        anchors.fill: parent
        text: root.text
        activeFocusOnTab: true
        enabled: !root.disabled
        tristate: root.partiallyChecked
        spacing: NanSpacing.sm

        indicator: Rectangle {
            implicitWidth: root.indicatorSize
            implicitHeight: root.indicatorSize
            radius: root.indicatorRadius
            border.width: control.visualFocus ? 2 : 1
            border.color: {
                if (control.checked || control.checkState === Qt.PartiallyChecked)
                    return root.themePalette ? root.themePalette.activeBorder : "#4f8cff";
                return root.themePalette ? root.themePalette.inactiveBorder : "#666";
            }
            color: {
                if (control.checked || control.checkState === Qt.PartiallyChecked)
                    return root.themePalette ? root.themePalette.activeBorder : "#4f8cff";
                return root.themePalette ? root.themePalette.secondaryPane : "#2b2b33";
            }
            opacity: root.disabled ? 0.5 : 1.0

            Text {
                anchors.centerIn: parent
                visible: control.checked || control.checkState === Qt.PartiallyChecked
                text: control.checkState === Qt.PartiallyChecked ? "—" : "✓"
                color: root.themePalette ? root.themePalette.onAccent : "white"
                font: root.markFont
            }
        }

        contentItem: Text {
            text: control.text
            color: root.themePalette ? root.themePalette.bodyCopy : "#efefef"
            verticalAlignment: Text.AlignVCenter
            leftPadding: control.indicator.width + control.spacing
            font: root.textFont
            opacity: root.disabled ? 0.6 : 1.0
        }

        onToggled: root.toggled(checked)
    }
}
