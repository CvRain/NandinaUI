import QtQuick
import QtQuick.Controls as QQC
import Nandina.Theme
import "theme_utils.js" as ThemeUtils

Item {
    id: root

    implicitWidth: 260
    implicitHeight: textField.implicitHeight + (hintText.visible ? hintText.implicitHeight + 6 : 0)

    property alias text: textField.text
    property alias placeholderText: textField.placeholderText
    property int maxLength: textField.maximumLength
    property bool disabled: false
    property bool readOnly: false
    property bool invalid: false
    property bool success: false
    property string errorText: ""
    property string helperText: ""
    property var themeManager: null

    readonly property bool hovered: hoverArea.containsMouse
    readonly property bool pressed: hoverArea.pressed
    readonly property bool focused: textField.activeFocus
    readonly property bool entered: hovered
    readonly property bool exited: !hovered

    ThemeManager {
        id: fallbackThemeManager
    }

    readonly property var resolvedThemeManager: ThemeUtils.resolveThemeManager(root, root.themeManager, fallbackThemeManager)

    readonly property var themePalette: root.resolvedThemeManager && root.resolvedThemeManager.currentPaletteCollection
                                      ? root.resolvedThemeManager.currentPaletteCollection : null

    signal accepted()

    QQC.TextField {
        id: textField
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        activeFocusOnTab: true
        enabled: !root.disabled
        readOnly: root.readOnly
        color: root.themePalette ? root.themePalette.bodyCopy : "#f5f5f5"
        placeholderTextColor: root.themePalette ? root.themePalette.subHeadlines0 : "#a3a3b2"
        selectByMouse: true

        background: Rectangle {
            radius: 8
            border.width: textField.activeFocus ? 2 : 1
            border.color: {
                if (root.invalid)
                    return root.themePalette ? root.themePalette.error : "#d9534f"
                if (root.success)
                    return root.themePalette ? root.themePalette.success : "#55b16c"
                if (textField.activeFocus)
                    return root.themePalette ? root.themePalette.activeBorder : "#4f8cff"
                return root.themePalette ? root.themePalette.inactiveBorder : "#666"
            }
            color: root.themePalette ? root.themePalette.secondaryPane : "#2b2b33"
            opacity: root.disabled ? 0.55 : 1.0

            Behavior on border.color {
                ColorAnimation { duration: 120 }
            }
        }

        onAccepted: root.accepted()
    }

    MouseArea {
        id: hoverArea
        anchors.fill: textField
        enabled: !root.disabled
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }

    Text {
        id: hintText
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: textField.bottom
        anchors.topMargin: 6
        visible: root.errorText.length > 0 || root.helperText.length > 0
        text: root.errorText.length > 0 ? root.errorText : root.helperText
        color: root.errorText.length > 0
             ? (root.themePalette ? root.themePalette.error : "#d9534f")
             : (root.themePalette ? root.themePalette.subHeadlines0 : "#a3a3b2")
        font.pixelSize: 12
        wrapMode: Text.Wrap
    }
}
