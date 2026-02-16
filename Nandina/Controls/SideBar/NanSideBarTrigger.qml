pragma ComponentBehavior: Bound

import QtQuick
import Nandina.Theme
import "../theme_utils.js" as ThemeUtils

Rectangle {
    id: root

    implicitWidth: 32
    implicitHeight: 32
    radius: 8
    color: triggerArea.pressed ? (themePalette ? themePalette.overlay1 : "#4c4c58") : (triggerArea.containsMouse ? (themePalette ? themePalette.surfaceElement0 : "#3b3b46") : "transparent")

    property var sidebar: null
    property var themeManager: null
    property string leftExpandedText: "◀"
    property string leftCollapsedText: "▶"
    property string rightExpandedText: "▶"
    property string rightCollapsedText: "◀"

    readonly property var resolvedSidebar: root.sidebar

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
        font.pixelSize: 12
        scale: root.resolvedSidebar && root.resolvedSidebar.open ? 1.0 : 0.9

        Behavior on scale {
            NumberAnimation {
                duration: 120
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
            duration: 120
        }
    }
}
