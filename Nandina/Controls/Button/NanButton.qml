import QtQuick
import Nandina.Theme
import "../theme_utils.js" as ThemeUtils
import "button_style_utils.js" as ButtonStyleUtils

FocusScope {
    id: root

    enum Variant {
        Primary,
        Ghost,
        Secondary,
        Tertiary,
        Destructive,
        Link,
        Custom
    }

    enum Accent {
        Filled,
        Tonal,
        Outlined
    }

    enum LinkTone {
        Neutral,
        Success,
        Warn,
        Error
    }

    enum Size {
        Sm,
        Md,
        Lg
    }

    implicitWidth: Math.max(96, contentRow.implicitWidth + horizontalPadding * 2)
    implicitHeight: {
        if (size === NanButton.Size.Sm)
            return 30;
        if (size === NanButton.Size.Lg)
            return 42;
        return 36;
    }

    activeFocusOnTab: !root.disabled

    property string text: ""
    property bool disabled: false
    property int variant: NanButton.Variant.Primary
    property int accent: NanButton.Accent.Filled
    property int linkTone: NanButton.LinkTone.Neutral
    property int size: NanButton.Size.Md
    property Component leftIcon: null
    property Component rightIcon: null
    property var themeManager: null

    readonly property int primary: NanButton.Variant.Primary
    readonly property int secondary: NanButton.Variant.Secondary
    readonly property int tertiary: NanButton.Variant.Tertiary
    readonly property int ghost: NanButton.Variant.Ghost
    readonly property int destructive: NanButton.Variant.Destructive
    readonly property int link: NanButton.Variant.Link
    readonly property int custom: NanButton.Variant.Custom

    readonly property int filled: NanButton.Accent.Filled
    readonly property int tonal: NanButton.Accent.Tonal
    readonly property int outlined: NanButton.Accent.Outlined

    readonly property int neutral: NanButton.LinkTone.Neutral
    readonly property int success: NanButton.LinkTone.Success
    readonly property int warn: NanButton.LinkTone.Warn
    readonly property int error: NanButton.LinkTone.Error

    property color customBackgroundColor: "transparent"
    property color customForegroundColor: "transparent"
    property color customBorderColor: "transparent"
    property color customHoverColor: "transparent"
    property color customPressedColor: "transparent"

    property bool useCustomBackgroundColor: false
    property bool useCustomForegroundColor: false
    property bool useCustomBorderColor: false
    property bool useCustomHoverColor: false
    property bool useCustomPressedColor: false

    property bool enableHoverAnimation: true
    property bool enableClickBounce: true
    property bool enableHoverHighlight: true
    property real baseScale: 1.0
    property real hoverScale: 1.03
    property real pressScale: 0.97
    property int hoverTransitionDuration: 120
    property int clickBounceInDuration: 70
    property int clickBounceOutDuration: 140
    property real hoverHighlightOpacity: 0.06

    property real currentScale: baseScale
    property bool isBouncing: false
    property bool wasPressedInside: false
    property bool keyboardPressActive: false
    readonly property real targetScale: {
        if (root.disabled)
            return root.baseScale;
        if (root.pressed)
            return root.pressScale;
        if (root.hovered)
            return root.hoverScale;
        return root.baseScale;
    }

    readonly property bool hovered: interactionArea.containsMouse
    readonly property bool pressed: interactionArea.pressed || root.keyboardPressActive
    readonly property bool focused: root.activeFocus
    readonly property bool entered: hovered
    readonly property bool exited: !hovered

    ThemeManager {
        id: fallbackThemeManager
    }

    readonly property var resolvedThemeManager: ThemeUtils.resolveThemeManager(root, root.themeManager, fallbackThemeManager)

    readonly property var themePalette: root.resolvedThemeManager && root.resolvedThemeManager.currentPaletteCollection ? root.resolvedThemeManager.currentPaletteCollection : null

    readonly property var styleConstants: ({
            variantPrimary: NanButton.Variant.Primary,
            variantSecondary: NanButton.Variant.Secondary,
            variantTertiary: NanButton.Variant.Tertiary,
            variantGhost: NanButton.Variant.Ghost,
            variantDestructive: NanButton.Variant.Destructive,
            variantLink: NanButton.Variant.Link,
            variantCustom: NanButton.Variant.Custom,
            accentFilled: NanButton.Accent.Filled,
            accentTonal: NanButton.Accent.Tonal,
            accentOutlined: NanButton.Accent.Outlined,
            toneSuccess: NanButton.LinkTone.Success,
            toneWarn: NanButton.LinkTone.Warn,
            toneError: NanButton.LinkTone.Error
        })

    readonly property var resolvedColors: ButtonStyleUtils.resolveColors({
        palette: root.themePalette,
        variant: root.variant,
        accent: root.accent,
        linkTone: root.linkTone,
        focused: root.focused,
        customBackgroundColor: root.customBackgroundColor,
        customForegroundColor: root.customForegroundColor,
        customBorderColor: root.customBorderColor,
        customHoverColor: root.customHoverColor,
        customPressedColor: root.customPressedColor,
        useCustomBackgroundColor: root.useCustomBackgroundColor,
        useCustomForegroundColor: root.useCustomForegroundColor,
        useCustomBorderColor: root.useCustomBorderColor,
        useCustomHoverColor: root.useCustomHoverColor,
        useCustomPressedColor: root.useCustomPressedColor,
        constants: root.styleConstants
    })

    readonly property color variantBaseColor: resolvedColors.variantBaseColor
    readonly property color foregroundColor: resolvedColors.foregroundColor
    readonly property color backgroundColor: resolvedColors.backgroundColor
    readonly property color hoverColor: resolvedColors.hoverColor
    readonly property color pressedColor: resolvedColors.pressedColor
    readonly property color borderColor: resolvedColors.borderColor

    property int horizontalPadding: root.size === NanButton.Size.Sm ? 10 : (root.size === NanButton.Size.Lg ? 16 : 12)

    signal clicked
    signal released
    signal pressStarted
    signal canceled

    Accessible.role: Accessible.Button
    Accessible.name: root.text
    Accessible.focusable: !root.disabled

    function emitClickSequence() {
        root.triggerClickFeedback();
        root.clicked();
        root.released();
    }

    transformOrigin: Item.Center
    scale: currentScale

    function triggerClickFeedback() {
        if (!root.enableClickBounce || root.disabled)
            return;
        clickBounce.restart();
    }

    onTargetScaleChanged: {
        if (!root.enableHoverAnimation || root.isBouncing)
            return;
        root.currentScale = root.targetScale;
    }

    Behavior on currentScale {
        enabled: root.enableHoverAnimation && !root.isBouncing

        NumberAnimation {
            duration: root.hoverTransitionDuration
            easing.type: Easing.OutCubic
        }
    }

    SequentialAnimation {
        id: clickBounce
        running: false

        onStarted: root.isBouncing = true
        onStopped: {
            root.isBouncing = false;
            root.currentScale = root.targetScale;
        }

        NumberAnimation {
            target: root
            property: "currentScale"
            to: root.pressScale
            duration: root.clickBounceInDuration
            easing.type: Easing.InQuad
        }

        NumberAnimation {
            target: root
            property: "currentScale"
            to: root.hovered ? root.hoverScale : root.baseScale
            duration: root.clickBounceOutDuration
            easing.type: Easing.OutBack
        }
    }

    Rectangle {
        id: backgroundRect
        anchors.fill: parent
        radius: 8
        border.width: root.accent === NanButton.Accent.Outlined || root.focused ? 1 : 0
        border.color: root.borderColor
        color: {
            if (root.disabled)
                return root.themePalette ? root.themePalette.surfaceElement1 : "#3a3a42";
            if (root.pressed)
                return root.pressedColor;
            if (interactionArea.containsMouse)
                return root.hoverColor;
            return root.backgroundColor;
        }
        opacity: root.disabled ? 0.6 : 1.0

        Behavior on color {
            ColorAnimation {
                duration: 120
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: root.foregroundColor
            opacity: {
                if (!root.enableHoverHighlight || root.disabled)
                    return 0;
                if (root.pressed)
                    return root.hoverHighlightOpacity * 1.8;
                if (root.hovered)
                    return root.hoverHighlightOpacity;
                return 0;
            }

            Behavior on opacity {
                NumberAnimation {
                    duration: root.hoverTransitionDuration
                    easing.type: Easing.OutCubic
                }
            }
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

    Keys.onPressed: function (event) {
        if (root.disabled)
            return;
        if (event.isAutoRepeat)
            return;
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            root.pressStarted();
            root.emitClickSequence();
            event.accepted = true;
            return;
        }
        if (event.key === Qt.Key_Space) {
            if (!root.keyboardPressActive) {
                root.keyboardPressActive = true;
                root.pressStarted();
            }
            event.accepted = true;
        }
    }

    Keys.onReleased: function (event) {
        if (root.disabled)
            return;
        if (event.isAutoRepeat)
            return;
        if (event.key === Qt.Key_Space && root.keyboardPressActive) {
            root.keyboardPressActive = false;
            root.emitClickSequence();
            event.accepted = true;
        }
    }

    onActiveFocusChanged: {
        if (!root.activeFocus && root.keyboardPressActive) {
            root.keyboardPressActive = false;
            root.canceled();
        }
    }

    onDisabledChanged: {
        if (root.disabled && root.keyboardPressActive) {
            root.keyboardPressActive = false;
            root.canceled();
        }
    }

    MouseArea {
        id: interactionArea
        anchors.fill: parent
        enabled: !root.disabled
        hoverEnabled: true
        cursorShape: root.disabled ? Qt.ArrowCursor : Qt.PointingHandCursor
        onPressed: {
            root.wasPressedInside = true;
            root.pressStarted();
        }
        onClicked: {
            root.triggerClickFeedback();
            root.clicked();
        }
        onCanceled: {
            root.wasPressedInside = false;
            root.canceled();
        }
        onReleased: {
            if (root.wasPressedInside && containsMouse)
                root.released();
            else if (root.wasPressedInside)
                root.canceled();
            root.wasPressedInside = false;
        }
    }
}
