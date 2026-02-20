pragma ComponentBehavior: Bound

import QtQuick
import Nandina.Theme
import Nandina.Tokens
import "../theme_utils.js" as ThemeUtils

Item {
    id: root

    default property alias contentData: contentColumn.data

    implicitWidth: 200
    implicitHeight: groupColumn.implicitHeight
    width: parent ? parent.width : implicitWidth

    property var sidebar: null
    property var themeManager: null
    property alias title: titleText.text
    property alias font: titleText.font
    property bool collapsible: false
    property bool expanded: true
    property int spacing: spacingTokens.sm
    property int headerHeight: spacingTokens.xl + (spacingTokens.xs / 2)
    property int headerCornerRadius: radiusTokens.md
    property int titleHorizontalInset: spacingTokens.sm
    property int titlePixelSize: typographyTokens.caption.pixelSize - 1
    property int headerStateDuration: motionTokens.fast
    property int headerExpandDuration: motionTokens.bounceOut
    property int contentExpandDuration: motionTokens.normal

    readonly property var spacingTokens: NanSpacing
    readonly property var radiusTokens: NanRadius
    readonly property var typographyTokens: NanTypography
    readonly property var motionTokens: NanMotion

    readonly property var resolvedSidebar: root.sidebar ? root.sidebar : ThemeUtils.resolveSidebar(root)

    readonly property bool collapsed: root.resolvedSidebar ? root.resolvedSidebar.collapsed : false
    readonly property var resolvedThemeManager: ThemeUtils.resolveThemeManager(root, root.themeManager, fallbackThemeManager)
    readonly property var themePalette: {
        if (root.resolvedSidebar && root.resolvedSidebar.themePalette)
            return root.resolvedSidebar.themePalette;
        return root.resolvedThemeManager && root.resolvedThemeManager.currentPaletteCollection ? root.resolvedThemeManager.currentPaletteCollection : null;
    }

    ThemeManager {
        id: fallbackThemeManager
    }

    Column {
        id: groupColumn
        width: parent.width
        spacing: root.spacing

        Item {
            id: headerRow
            width: parent.width
            height: root.collapsed || root.title.length === 0 ? 0 : root.headerHeight
            visible: height > 0

            Rectangle {
                anchors.fill: parent
                radius: root.headerCornerRadius
                color: headerArea.pressed ? (root.themePalette ? root.themePalette.overlay1 : "#4a4a56") : (headerArea.containsMouse ? (root.themePalette ? root.themePalette.overlay0 : "#343440") : "transparent")
                visible: root.collapsible

                Behavior on color {
                    ColorAnimation {
                        duration: root.headerStateDuration
                    }
                }
            }

            Text {
                id: titleText
                anchors.left: parent.left
                anchors.leftMargin: root.titleHorizontalInset
                anchors.verticalCenter: parent.verticalCenter
                color: root.themePalette ? root.themePalette.subHeadlines0 : "#b6b6c4"
                font.pixelSize: root.titlePixelSize
                font.weight: Font.DemiBold
                font.letterSpacing: 0.4
                elide: Text.ElideRight
            }

            Text {
                anchors.right: parent.right
                anchors.rightMargin: root.titleHorizontalInset
                anchors.verticalCenter: parent.verticalCenter
                visible: root.collapsible
                text: root.expanded ? "▾" : "▸"
                color: root.themePalette ? root.themePalette.subHeadlines1 : "#9a9aaa"
                font.pixelSize: root.titlePixelSize
            }

            MouseArea {
                id: headerArea
                anchors.fill: parent
                enabled: root.collapsible
                onClicked: root.expanded = !root.expanded
            }

            Behavior on height {
                NumberAnimation {
                    duration: root.headerExpandDuration
                    easing.type: Easing.OutCubic
                }
            }
        }

        Item {
            id: contentClip
            width: parent.width
            height: (root.collapsible && !root.expanded) ? 0 : contentColumn.implicitHeight
            clip: true

            Column {
                id: contentColumn
                width: parent.width
                spacing: root.spacing
            }

            Behavior on height {
                NumberAnimation {
                    duration: root.contentExpandDuration
                    easing.type: Easing.OutCubic
                }
            }
        }
    }
}
