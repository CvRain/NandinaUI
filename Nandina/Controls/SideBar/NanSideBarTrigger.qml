pragma ComponentBehavior: Bound

import QtQuick
import Nandina.Theme
import Nandina.Tokens
import Nandina.Primitives
import "../theme_utils.js" as ThemeUtils

Rectangle {
    id: root

    implicitWidth: root.triggerSize
    implicitHeight: root.triggerSize
    radius: root.triggerCornerRadius
    color: interactionArea.pressed ? (themePalette ? themePalette.overlay2 : "#4c4c58") : (interactionArea.hovered ? (themePalette ? themePalette.overlay1 : "#3b3b46") : "transparent")
    border.width: interactionArea.hovered ? 1 : 0
    border.color: themePalette ? themePalette.activeBorder : "#6b6b78"

    property var sidebar: null
    property var themeManager: NanStyle.themeManager
    property int triggerSize: spacingTokens.xxl
    property int triggerCornerRadius: radiusTokens.md
    property font font: ThemeUtils.resolveFont(root, NanStyle.font, Qt.font({
        pixelSize: typographyTokens.caption.pixelSize,
        weight: Font.DemiBold
    }))
    property int transitionDuration: motionTokens.fast
    property string leftExpandedText: "◀"
    property string leftCollapsedText: "▶"
    property string rightExpandedText: "▶"
    property string rightCollapsedText: "◀"

    readonly property var spacingTokens: NanSpacing
    readonly property var radiusTokens: NanRadius
    readonly property var typographyTokens: NanTypography
    readonly property var motionTokens: NanMotion

    readonly property var resolvedSidebar: root.sidebar ? root.sidebar : ThemeUtils.resolveSidebar(root)

    readonly property var resolvedThemeManager: root.themeManager ? root.themeManager : NanTheme.themeManager
    readonly property var themePalette: {
        if (root.resolvedSidebar && root.resolvedSidebar.themePalette)
            return root.resolvedSidebar.themePalette;
        return root.resolvedThemeManager && root.resolvedThemeManager.currentPaletteCollection ? root.resolvedThemeManager.currentPaletteCollection : null;
    }

    signal pressStarted
    signal clicked
    signal released
    signal canceled

    function reportInteraction(type) {
        if (root.resolvedSidebar && root.resolvedSidebar.reportInteraction)
            root.resolvedSidebar.reportInteraction(type, {
                open: root.resolvedSidebar ? root.resolvedSidebar.open : false
            });
    }

    Text {
        id: glyph
        anchors.centerIn: parent
        text: {
            if (!root.resolvedSidebar)
                return root.leftExpandedText;

            const rightSided = root.resolvedSidebar.side === NanSideBar.Side.Right;
            if (rightSided)
                return root.resolvedSidebar.open ? root.rightExpandedText : root.rightCollapsedText;
            return root.resolvedSidebar.open ? root.leftExpandedText : root.leftCollapsedText;
        }
        color: root.themePalette ? root.themePalette.mainHeadline : "#f5f5f5"
        font.family: root.font.family
        font.pixelSize: root.font.pixelSize > 0 ? root.font.pixelSize : root.typographyTokens.caption.pixelSize
        font.weight: root.font.weight > 0 ? root.font.weight : Font.DemiBold
        font.italic: root.font.italic
        scale: root.resolvedSidebar && root.resolvedSidebar.open ? 1.0 : 0.92
        opacity: interactionArea.hovered ? 1 : 0.9

        Behavior on scale {
            NumberAnimation {
                duration: root.transitionDuration
                easing.type: Easing.OutCubic
            }
        }

        Behavior on opacity {
            NumberAnimation {
                duration: root.transitionDuration
                easing.type: Easing.OutCubic
            }
        }
    }

    Pressable {
        id: interactionArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor

        onPressStarted: {
            root.pressStarted();
            root.reportInteraction("sidebar.trigger.pressStarted");
        }

        onClicked: {
            if (root.resolvedSidebar && root.resolvedSidebar.toggle)
                root.resolvedSidebar.toggle();
            root.clicked();
            root.reportInteraction("sidebar.trigger.clicked");
        }

        onReleased: {
            root.released();
            root.reportInteraction("sidebar.trigger.released");
        }
        onCanceled: {
            root.canceled();
            root.reportInteraction("sidebar.trigger.canceled");
        }
    }

    Behavior on color {
        ColorAnimation {
            duration: root.transitionDuration
        }
    }

    Behavior on border.width {
        NumberAnimation {
            duration: root.transitionDuration
            easing.type: Easing.OutCubic
        }
    }
}
