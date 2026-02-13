import QtQuick
import Nandina.Theme
import "theme_utils.js" as ThemeUtils

FocusScope {
    id: root

    enum Variant {
        Default,
        Outline,
        Ghost,
        Destructive,
        Secondary,
        Link
    }

    enum Size {
        Sm,
        Md,
        Lg
    }

    implicitWidth: Math.max(96, contentRow.implicitWidth + horizontalPadding * 2)
    implicitHeight: {
        if (size === NanButton.Size.Sm)
            return 30
        if (size === NanButton.Size.Lg)
            return 42
        return 36
    }

    activeFocusOnTab: true

    property string text: ""
    property bool disabled: false
    property int variant: NanButton.Variant.Default
    property int size: NanButton.Size.Md
    property Component leftIcon: null
    property Component rightIcon: null
    property var themeManager: null

    readonly property bool hovered: interactionArea.containsMouse
    readonly property bool pressed: interactionArea.pressed
    readonly property bool focused: root.activeFocus
    readonly property bool entered: hovered
    readonly property bool exited: !hovered

    ThemeManager {
        id: fallbackThemeManager
    }

    readonly property var resolvedThemeManager: ThemeUtils.resolveThemeManager(root, root.themeManager, fallbackThemeManager)

    readonly property var themePalette: root.resolvedThemeManager && root.resolvedThemeManager.currentPaletteCollection
                                      ? root.resolvedThemeManager.currentPaletteCollection : null

    readonly property color foregroundColor: {
        if (root.variant === NanButton.Variant.Link)
            return root.themePalette ? root.themePalette.links : "#6c8cff"
        if (root.variant === NanButton.Variant.Outline || root.variant === NanButton.Variant.Ghost)
            return root.themePalette ? root.themePalette.bodyCopy : "#f5f5f5"
        if (root.variant === NanButton.Variant.Destructive)
            return root.themePalette ? root.themePalette.onAccent : "white"
        return root.themePalette ? root.themePalette.onAccent : "white"
    }

    readonly property color backgroundColor: {
        if (root.variant === NanButton.Variant.Outline || root.variant === NanButton.Variant.Ghost || root.variant === NanButton.Variant.Link)
            return "transparent"
        if (root.variant === NanButton.Variant.Destructive)
            return root.themePalette ? root.themePalette.error : "#d9534f"
        if (root.variant === NanButton.Variant.Secondary)
            return root.themePalette ? root.themePalette.secondaryPane : "#3b3b46"
        return root.themePalette ? root.themePalette.activeBorder : "#4f8cff"
    }

    readonly property color hoverColor: {
        if (root.variant === NanButton.Variant.Outline || root.variant === NanButton.Variant.Ghost || root.variant === NanButton.Variant.Link)
            return root.themePalette ? root.themePalette.overlay0 : "#2f2f37"
        return root.themePalette ? root.themePalette.overlay1 : "#4066bf"
    }

    readonly property color pressedColor: {
        if (root.variant === NanButton.Variant.Outline || root.variant === NanButton.Variant.Ghost || root.variant === NanButton.Variant.Link)
            return root.themePalette ? root.themePalette.overlay1 : "#383844"
        return root.themePalette ? root.themePalette.overlay2 : "#3557a8"
    }

    readonly property color borderColor: {
        if (root.variant === NanButton.Variant.Outline)
            return root.themePalette ? root.themePalette.inactiveBorder : "#666"
        if (root.focused)
            return root.themePalette ? root.themePalette.activeBorder : "#4f8cff"
        return "transparent"
    }

    property int horizontalPadding: root.size === NanButton.Size.Sm ? 10 : (root.size === NanButton.Size.Lg ? 16 : 12)

    signal clicked()
    signal released()

    Rectangle {
        anchors.fill: parent
        radius: 8
        border.width: root.variant === NanButton.Variant.Outline || root.focused ? 1 : 0
        border.color: root.borderColor
        color: {
            if (root.disabled)
                return root.themePalette ? root.themePalette.surfaceElement1 : "#3a3a42"
            if (interactionArea.pressed)
                return root.pressedColor
            if (interactionArea.containsMouse)
                return root.hoverColor
            return root.backgroundColor
        }
        opacity: root.disabled ? 0.6 : 1.0

        Behavior on color {
            ColorAnimation { duration: 120 }
        }
    }

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: 8

        Loader {
            active: root.leftIcon !== null
            sourceComponent: root.leftIcon
        }

        Text {
            text: root.text
            color: root.disabled && root.themePalette ? root.themePalette.subHeadlines0 : root.foregroundColor
            font.pixelSize: root.size === NanButton.Size.Sm ? 12 : 13
            font.weight: Font.Medium
        }

        Loader {
            active: root.rightIcon !== null
            sourceComponent: root.rightIcon
        }
    }

    Keys.onPressed: function(event) {
        if (root.disabled)
            return

        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
            root.clicked()
            root.released()
            event.accepted = true
        }
    }

    MouseArea {
        id: interactionArea
        anchors.fill: parent
        enabled: !root.disabled
        hoverEnabled: true
        cursorShape: root.disabled ? Qt.ArrowCursor : Qt.PointingHandCursor
        onClicked: root.clicked()
        onReleased: root.released()
    }
}
