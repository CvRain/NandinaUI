import QtQuick

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
        if (size === Button.Size.Sm)
            return 30
        if (size === Button.Size.Lg)
            return 42
        return 36
    }

    activeFocusOnTab: true

    property string text: ""
    property bool disabled: false
    property int variant: Button.Variant.Default
    property int size: Button.Size.Md
    property Component leftIcon: null
    property Component rightIcon: null
    property var themeManager: null

    readonly property bool hovered: interactionArea.containsMouse
    readonly property bool pressed: interactionArea.pressed
    readonly property bool focused: root.activeFocus
    readonly property bool entered: hovered
    readonly property bool exited: !hovered

    readonly property var themePalette: themeManager && themeManager.currentPaletteCollection
                                      ? themeManager.currentPaletteCollection : null

    readonly property color foregroundColor: {
        if (variant === Button.Variant.Link)
            return themePalette ? themePalette.links : "#6c8cff"
        if (variant === Button.Variant.Outline || variant === Button.Variant.Ghost)
            return themePalette ? themePalette.bodyCopy : "#f5f5f5"
        if (variant === Button.Variant.Destructive)
            return themePalette ? themePalette.onAccent : "white"
        return themePalette ? themePalette.onAccent : "white"
    }

    readonly property color backgroundColor: {
        if (variant === Button.Variant.Outline || variant === Button.Variant.Ghost || variant === Button.Variant.Link)
            return "transparent"
        if (variant === Button.Variant.Destructive)
            return themePalette ? themePalette.error : "#d9534f"
        if (variant === Button.Variant.Secondary)
            return themePalette ? themePalette.secondaryPane : "#3b3b46"
        return themePalette ? themePalette.activeBorder : "#4f8cff"
    }

    readonly property color hoverColor: {
        if (variant === Button.Variant.Outline || variant === Button.Variant.Ghost || variant === Button.Variant.Link)
            return themePalette ? themePalette.overlay0 : "#2f2f37"
        return themePalette ? themePalette.overlay1 : "#4066bf"
    }

    readonly property color pressedColor: {
        if (variant === Button.Variant.Outline || variant === Button.Variant.Ghost || variant === Button.Variant.Link)
            return themePalette ? themePalette.overlay1 : "#383844"
        return themePalette ? themePalette.overlay2 : "#3557a8"
    }

    readonly property color borderColor: {
        if (variant === Button.Variant.Outline)
            return themePalette ? themePalette.inactiveBorder : "#666"
        if (focused)
            return themePalette ? themePalette.activeBorder : "#4f8cff"
        return "transparent"
    }

    property int horizontalPadding: size === Button.Size.Sm ? 10 : (size === Button.Size.Lg ? 16 : 12)

    signal clicked()
    signal released()

    Rectangle {
        id: backgroundRect
        anchors.fill: parent
        radius: 8
        border.width: root.variant === Button.Variant.Outline || root.focused ? 1 : 0
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
            font.pixelSize: root.size === Button.Size.Sm ? 12 : 13
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
