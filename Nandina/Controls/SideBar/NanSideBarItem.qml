pragma ComponentBehavior: Bound

import QtQuick
import Nandina.Theme
import Nandina.Tokens
import "../theme_utils.js" as ThemeUtils

Rectangle {
    id: root

    implicitHeight: 38
    implicitWidth: 160
    width: parent ? parent.width : implicitWidth
    height: implicitHeight
    clip: true
    radius: root.itemCornerRadius
    color: {
        if (itemArea.pressed)
            return root.themePalette ? root.themePalette.overlay2 : "#4a4a56";
        if (root.active)
            return root.themePalette ? root.themePalette.surfaceElement1 : "#3a3a45";
        if (itemArea.containsMouse)
            return root.themePalette ? root.themePalette.overlay0 : "#343440";
        return "transparent";
    }
    border.width: root.active ? 1 : 0
    border.color: root.themePalette ? root.themePalette.activeBorder : "#6b6b78"

    property var sidebar: null
    property var themeManager: NanStyle.themeManager
    property alias text: label.text
    property font font: ThemeUtils.resolveFont(root, NanStyle.font, typographyTokens.bodyLarge)
    property font textFont: root.font
    property string iconSource: ""
    property url fallbackIconSource: ""
    property string fallbackGlyph: "•"
    property font fallbackGlyphFont: Qt.font({
        pixelSize: 10,
        weight: Font.DemiBold
    })
    readonly property alias textItem: label
    readonly property alias fallbackGlyphItem: fallbackGlyphText

    property bool active: false
    property int itemCornerRadius: radiusTokens.md
    property int horizontalInset: spacingTokens.sm
    property int stateTransitionDuration: motionTokens.fast
    property int layoutTransitionDuration: motionTokens.normal

    readonly property var spacingTokens: NanSpacing
    readonly property var radiusTokens: NanRadius
    readonly property var typographyTokens: NanTypography
    readonly property var motionTokens: NanMotion

    readonly property var resolvedSidebar: root.sidebar ? root.sidebar : ThemeUtils.resolveSidebar(root)

    readonly property bool collapsed: root.resolvedSidebar ? root.resolvedSidebar.collapsed : false
    readonly property bool rightSided: root.resolvedSidebar ? root.resolvedSidebar.side === NanSideBar.Side.Right : false
    readonly property bool hasIcon: root.iconSource.length > 0
    readonly property bool hasFallbackIcon: root.fallbackIconSource.toString().length > 0
    readonly property var resolvedThemeManager: root.themeManager ? root.themeManager : NanTheme.themeManager
    readonly property var themePalette: {
        if (root.resolvedSidebar && root.resolvedSidebar.themePalette)
            return root.resolvedSidebar.themePalette;
        return root.resolvedThemeManager && root.resolvedThemeManager.currentPaletteCollection ? root.resolvedThemeManager.currentPaletteCollection : null;
    }

    signal clicked

    Rectangle {
        width: 3
        radius: 2
        color: root.themePalette ? root.themePalette.activeBorder : "#7a7a8a"
        anchors.verticalCenter: parent.verticalCenter
        x: root.rightSided ? root.width - width - (root.horizontalInset - 6) : (root.horizontalInset - 6)
        height: root.active ? 18 : 8
        opacity: root.active ? 1 : 0

        Behavior on x {
            NumberAnimation {
                duration: root.layoutTransitionDuration
                easing.type: Easing.OutCubic
            }
        }

        Behavior on opacity {
            NumberAnimation {
                duration: root.stateTransitionDuration
                easing.type: Easing.OutCubic
            }
        }

        Behavior on height {
            NumberAnimation {
                duration: root.stateTransitionDuration
                easing.type: Easing.OutCubic
            }
        }
    }

    Rectangle {
        id: iconHolder
        width: 22
        height: 22
        radius: 11
        color: root.active ? (root.themePalette ? root.themePalette.overlay2 : "#666674") : (root.themePalette ? root.themePalette.overlay0 : "#5a5a66")
        anchors.verticalCenter: parent.verticalCenter
        x: {
            if (root.collapsed)
                return Math.floor((root.width - width) / 2);
            return root.rightSided ? root.width - width - root.horizontalInset : root.horizontalInset;
        }

        Behavior on x {
            NumberAnimation {
                duration: root.layoutTransitionDuration
                easing.type: Easing.InOutSine
            }
        }

        Image {
            anchors.centerIn: parent
            width: 14
            height: 14
            source: root.iconSource
            visible: root.hasIcon
            fillMode: Image.PreserveAspectFit
        }

        Image {
            anchors.centerIn: parent
            width: 14
            height: 14
            source: root.fallbackIconSource
            visible: !root.hasIcon && root.hasFallbackIcon
            fillMode: Image.PreserveAspectFit
        }

        Text {
            id: fallbackGlyphText
            anchors.centerIn: parent
            visible: !root.hasIcon && !root.hasFallbackIcon
            text: root.fallbackGlyph.length > 0 ? root.fallbackGlyph : "•"
            color: root.themePalette ? root.themePalette.mainHeadline : "#f0f0f5"
            font.family: root.fallbackGlyphFont.family
            font.pixelSize: root.fallbackGlyphFont.pixelSize > 0 ? root.fallbackGlyphFont.pixelSize : 10
            font.weight: root.active ? Font.Bold : root.fallbackGlyphFont.weight
            font.italic: root.fallbackGlyphFont.italic
        }
    }

    Text {
        id: label
        color: root.active ? (root.themePalette ? root.themePalette.mainHeadline : "#f2f2f8") : (root.themePalette ? root.themePalette.bodyCopy : "#dbdbe5")
        anchors.verticalCenter: parent.verticalCenter
        x: root.rightSided ? 10 : iconHolder.x + iconHolder.width + 10
        width: root.rightSided ? Math.max(0, iconHolder.x - 20) : Math.max(0, root.width - x - root.horizontalInset)
        elide: Text.ElideRight
        font.family: root.textFont.family
        font.pixelSize: root.textFont.pixelSize > 0 ? root.textFont.pixelSize : root.typographyTokens.bodyLarge.pixelSize
        font.weight: root.active ? Font.DemiBold : (root.textFont.weight > 0 ? root.textFont.weight : Font.Medium)
        font.italic: root.textFont.italic
        horizontalAlignment: root.rightSided ? Text.AlignRight : Text.AlignLeft
        opacity: root.collapsed ? 0 : 1

        Behavior on x {
            NumberAnimation {
                duration: root.layoutTransitionDuration
                easing.type: Easing.InOutSine
            }
        }

        Behavior on width {
            NumberAnimation {
                duration: root.layoutTransitionDuration
                easing.type: Easing.InOutSine
            }
        }

        Behavior on opacity {
            NumberAnimation {
                duration: root.stateTransitionDuration
                easing.type: Easing.InOutSine
            }
        }
    }

    MouseArea {
        id: itemArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }

    Behavior on border.width {
        NumberAnimation {
            duration: root.stateTransitionDuration
            easing.type: Easing.OutCubic
        }
    }

    Behavior on color {
        ColorAnimation {
            duration: root.stateTransitionDuration
        }
    }
}
