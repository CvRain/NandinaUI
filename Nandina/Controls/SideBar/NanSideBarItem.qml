pragma ComponentBehavior: Bound

import QtQuick
import Nandina.Theme
import "../theme_utils.js" as ThemeUtils

Rectangle {
    id: root

    implicitHeight: 36
    implicitWidth: 160
    radius: 9
    color: {
        if (itemArea.pressed)
            return root.themePalette ? root.themePalette.overlay1 : "#4a4a56";
        if (root.active)
            return root.themePalette ? root.themePalette.surfaceElement1 : "#3a3a45";
        if (itemArea.containsMouse)
            return root.themePalette ? root.themePalette.surfaceElement0 : "#343440";
        return "transparent";
    }

    property var sidebar: null
    property var themeManager: null
    property string text: ""
    property string iconSource: ""
    property string fallbackGlyph: "•"
    property bool active: false

    readonly property var resolvedSidebar: root.sidebar

    readonly property bool collapsed: root.resolvedSidebar ? root.resolvedSidebar.collapsed : false
    readonly property bool rightSided: root.resolvedSidebar ? root.resolvedSidebar.side === NanSideBar.Side.Right : false
    readonly property bool hasIcon: root.iconSource.length > 0
    readonly property var resolvedThemeManager: ThemeUtils.resolveThemeManager(root, root.themeManager, fallbackThemeManager)
    readonly property var themePalette: {
        if (root.resolvedSidebar && root.resolvedSidebar.themePalette)
            return root.resolvedSidebar.themePalette;
        return root.resolvedThemeManager && root.resolvedThemeManager.currentPaletteCollection ? root.resolvedThemeManager.currentPaletteCollection : null;
    }

    ThemeManager {
        id: fallbackThemeManager
    }

    signal clicked

    Rectangle {
        id: iconHolder
        width: 22
        height: 22
        radius: 11
        color: root.themePalette ? root.themePalette.overlay0 : "#5a5a66"
        anchors.verticalCenter: parent.verticalCenter
        x: {
            if (root.collapsed)
                return Math.floor((root.width - width) / 2);
            return root.rightSided ? root.width - width - 8 : 8;
        }

        Behavior on x {
            NumberAnimation {
                duration: 180
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

        Text {
            anchors.centerIn: parent
            visible: !root.hasIcon
            text: root.fallbackGlyph.length > 0 ? root.fallbackGlyph.charAt(0).toUpperCase() : "•"
            color: root.themePalette ? root.themePalette.mainHeadline : "#f0f0f5"
            font.pixelSize: 10
            font.weight: Font.DemiBold
        }
    }

    Text {
        id: label
        text: root.text
        color: root.themePalette ? root.themePalette.bodyCopy : "#dbdbe5"
        anchors.verticalCenter: parent.verticalCenter
        x: root.rightSided ? 10 : iconHolder.x + iconHolder.width + 10
        width: root.rightSided ? Math.max(0, iconHolder.x - 20) : Math.max(0, root.width - x - 10)
        elide: Text.ElideRight
        font.pixelSize: 12
        horizontalAlignment: root.rightSided ? Text.AlignRight : Text.AlignLeft
        opacity: root.collapsed ? 0 : 1

        Behavior on x {
            NumberAnimation {
                duration: 180
                easing.type: Easing.InOutSine
            }
        }

        Behavior on width {
            NumberAnimation {
                duration: 180
                easing.type: Easing.InOutSine
            }
        }

        Behavior on opacity {
            NumberAnimation {
                duration: 140
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

    Behavior on color {
        ColorAnimation {
            duration: 120
        }
    }
}
