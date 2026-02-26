pragma ComponentBehavior: Bound

import QtQuick
import Nandina.Theme
import Nandina.Tokens
import "../theme_utils.js" as ThemeUtils

Rectangle {
    id: root

    implicitWidth: root.triggerSize
    implicitHeight: root.triggerSize
    radius: root.triggerCornerRadius
    color: triggerArea.pressed ? (themePalette ? themePalette.overlay2 : "#4c4c58") : (triggerArea.containsMouse ? (themePalette ? themePalette.overlay1 : "#3b3b46") : "transparent")
    border.width: triggerArea.containsMouse ? 1 : 0
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

    signal clicked

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
        opacity: triggerArea.containsMouse ? 1 : 0.9

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

    MouseArea {
        id: triggerArea
        anchors.fill: parent
        hoverEnabled: true

        onClicked: {
            if (root.resolvedSidebar && root.resolvedSidebar.toggle)
                root.resolvedSidebar.toggle();
            root.clicked();
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
