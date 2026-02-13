pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Nandina.Theme

ApplicationWindow {
    id: root

    enum TitleBarMode {
        DefaultTitleBar,
        Frameless,
        CustomTitleBar
    }

    property int titleBarMode: NanWindow.DefaultTitleBar
    property string windowTitle: "Nandina"
    property int titleBarHeight: 40
    property Component customTitleBar: null
    property bool useSystemResize: true
    property bool alwaysOnTop: false
    property int resizeMargin: 6
    property int windowRadius: 10
    property bool defaultTitleBarDraggable: true
    property bool defaultTitleBarShowControls: true
    property bool defaultTitleBarDoubleClickMaximize: true
    property bool customTitleBarInjectSystemControls: false
    property int customTitleBarControlsRightMargin: 8
    property int customTitleBarControlsSpacing: 6

    readonly property bool isMaximized: visibility === Window.Maximized
    readonly property bool isFramelessMode: titleBarMode !== NanWindow.DefaultTitleBar
    readonly property int effectiveWindowRadius: isMaximized || visibility
                                                 === Window.FullScreen ? 0 : windowRadius

    readonly property alias themeManager: internalThemeManager
    default property alias content: contentRoot.data

    title: windowTitle
    visible: true

    flags: {
        let value = Qt.Window
        if (isFramelessMode)
        value |= Qt.FramelessWindowHint
        if (alwaysOnTop)
        value |= Qt.WindowStaysOnTopHint
        return value
    }

    color: isFramelessMode ? "transparent" : (themeManager.currentPaletteCollection ? themeManager.getCurrentPaletteCollection().backgroundPane : "transparent")

    ThemeManager {
        id: internalThemeManager
    }

    Rectangle {
        id: layoutRoot
        anchors.fill: parent
        radius: root.effectiveWindowRadius
        clip: true
        color: internalThemeManager.currentPaletteCollection ? internalThemeManager.currentPaletteCollection.backgroundPane : "transparent"

        border.width: root.isFramelessMode ? 1 : 0
        border.color: internalThemeManager.currentPaletteCollection ? internalThemeManager.currentPaletteCollection.surfaceElement0 : "transparent"

        Loader {
            id: titleBarLoader
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: root.titleBarMode === NanWindow.CustomTitleBar ? root.titleBarHeight : 0
            active: root.titleBarMode === NanWindow.CustomTitleBar
            sourceComponent: root.customTitleBar ? root.customTitleBar : InnerDefaultTitleBar
        }

        Row {
            visible: root.titleBarMode === NanWindow.CustomTitleBar
                     && root.customTitleBarInjectSystemControls
            anchors.verticalCenter: titleBarLoader.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: root.customTitleBarControlsRightMargin
            spacing: root.customTitleBarControlsSpacing
            z: 5

            TitleBarButton {
                text: "—"
                textColor: internalThemeManager.currentPaletteCollection ? internalThemeManager.currentPaletteCollection.bodyCopy : "white"
                hoverColor: internalThemeManager.currentPaletteCollection ? internalThemeManager.currentPaletteCollection.overlay0 : "#4a4a4a"
                pressedColor: internalThemeManager.currentPaletteCollection ? internalThemeManager.currentPaletteCollection.overlay1 : "#5a5a5a"
                onClicked: root.showMinimized()
            }

            TitleBarButton {
                text: root.visibility === Window.Maximized ? "❐" : "□"
                textColor: internalThemeManager.currentPaletteCollection ? internalThemeManager.currentPaletteCollection.bodyCopy : "white"
                hoverColor: internalThemeManager.currentPaletteCollection ? internalThemeManager.currentPaletteCollection.overlay0 : "#4a4a4a"
                pressedColor: internalThemeManager.currentPaletteCollection ? internalThemeManager.currentPaletteCollection.overlay1 : "#5a5a5a"
                onClicked: {
                    if (root.visibility === Window.Maximized)
                    root.showNormal()
                    else
                    root.showMaximized()
                }
            }

            TitleBarButton {
                text: "✕"
                isCloseButton: true
                textColor: "white"
                onClicked: root.close()
            }
        }

        Item {
            id: contentRoot
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: titleBarLoader.bottom
            anchors.bottom: parent.bottom
        }

        WindowResizer {
            anchors.fill: parent
            targetWindow: root
            resizeMargin: root.resizeMargin
            topIgnoreArea: root.titleBarMode === NanWindow.CustomTitleBar ? root.titleBarHeight : 0
            visible: root.isFramelessMode && root.useSystemResize
                     && !root.isMaximized
        }
    }

    component InnerDefaultTitleBar: DefaultTitleBar {
        titleText: root.windowTitle
        targetWindow: root
        themeManager: internalThemeManager
        draggable: root.defaultTitleBarDraggable
        showWindowControls: root.defaultTitleBarShowControls
        enableDoubleClickToggleMaximize: root.defaultTitleBarDoubleClickMaximize
        topLeftRadius: root.effectiveWindowRadius
        topRightRadius: root.effectiveWindowRadius
    }
}
