import QtQuick
import Nandina.Theme
import Nandina.Tokens
import "theme_utils.js" as ThemeUtils

Item {
    id: root

    implicitWidth: labelText.implicitWidth
    implicitHeight: labelText.implicitHeight

    property string text: ""
    property var forControl: null
    property bool disabled: false
    property bool required: false
    property var themeManager: NanStyle.themeManager
    property font font: ThemeUtils.resolveFont(root, NanStyle.font, NanTypography.body)

    readonly property var resolvedThemeManager: root.themeManager ? root.themeManager : NanTheme.themeManager

    readonly property var themePalette: root.resolvedThemeManager && root.resolvedThemeManager.currentPaletteCollection ? root.resolvedThemeManager.currentPaletteCollection : null

    Text {
        id: labelText
        anchors.fill: parent
        text: root.required ? (root.text + " *") : root.text
        color: root.disabled ? (root.themePalette ? root.themePalette.subHeadlines0 : "#9d9dac") : (root.themePalette ? root.themePalette.bodyCopy : "#efefef")
        font: root.font
        verticalAlignment: Text.AlignVCenter
    }

    MouseArea {
        anchors.fill: parent
        enabled: !!root.forControl
        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: {
            if (root.forControl && root.forControl.forceActiveFocus)
                root.forControl.forceActiveFocus();
        }
    }
}
