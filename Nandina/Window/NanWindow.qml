pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Nandina.Theme

ApplicationWindow {
    id: root

    enum TitleBarMode {
        DefaultTitleBar, //默认使用系统标题栏，Windows 上为标准窗口，macOS 上为隐藏标题栏的窗口，Linux 上表现依平台和窗口管理器而异
        Frameless, //无边框窗口，完全自定义标题栏和边框
        CustomTitleBar //自定义标题栏，允许注入系统控件（最小化、最大化、关闭按钮）
    }

    property int titleBarMode: NanWindow.DefaultTitleBar //标题栏模式
    property string windowTitle: "Nandina" //窗口标题，CustomTitleBar 模式下会传递给自定义标题栏组件
    property int titleBarHeight: 40 //标题栏高度
    property Component customTitleBar: null //自定义标题栏组件
    property bool useSystemResize: true //是否使用系统调整大小
    property bool alwaysOnTop: false //是否总在最前
    property int resizeMargin: 6 //调整大小边距
    property int windowRadius: 10 //窗口圆角半径
    property bool defaultTitleBarDraggable: true //默认标题栏是否可拖动
    property bool defaultTitleBarShowControls: true //默认标题栏是否显示控件
    property bool defaultTitleBarDoubleClickMaximize: true //默认标题栏双击是否最大化
    property bool customTitleBarInjectSystemControls: false //自定义标题栏是否注入系统控件
    property int customTitleBarControlsRightMargin: 8 //自定义标题栏系统控件右边距
    property int customTitleBarControlsSpacing: 6 //自定义标题栏系统控件间距
    property bool enableThemeGradient: true //是否启用主题渐变背景
    property bool autoAdjustThemeTransitionDuration: true //是否自动调整主题过渡动画时长（根据当前主题亮度调整，亮色主题使用 lightThemeTransitionDuration，暗色主题使用 darkThemeTransitionDuration）
    property int themeTransitionDuration: 240 //主题过渡动画时长
    property int lightThemeTransitionDuration: 180 //亮色主题过渡动画时长
    property int darkThemeTransitionDuration: 260 //暗色主题过渡动画时长
    property real themeGradientOverlayOpacity: 0.18 //主题渐变覆盖层不透明度

    readonly property bool isMaximized: visibility === Window.Maximized //是否最大化
    readonly property bool isFramelessMode: titleBarMode !== NanWindow.DefaultTitleBar //是否无边框模式
    readonly property int effectiveWindowRadius: isMaximized || visibility === Window.FullScreen ? 0 : windowRadius //有效的窗口圆角半径
    readonly property real currentThemeLuminance: internalThemeManager.currentPaletteCollection ? (0.2126 * internalThemeManager.currentPaletteCollection.backgroundPane.r + 0.7152 * internalThemeManager.currentPaletteCollection.backgroundPane.g + 0.0722 * internalThemeManager.currentPaletteCollection.backgroundPane.b) : 0.5 //当前主题亮度
    readonly property bool isLightTheme: currentThemeLuminance >= 0.55 //是否为亮色主题
    readonly property int effectiveThemeTransitionDuration: autoAdjustThemeTransitionDuration ? (isLightTheme ? lightThemeTransitionDuration : darkThemeTransitionDuration) : themeTransitionDuration //有效的主题过渡动画时长

    readonly property alias themeManager: internalThemeManager //主题管理器，优先使用内部的 ThemeManager，外部也可以通过这个属性访问和设置
    default property alias content: contentRoot.data //窗口内容，直接添加到 contentRoot 中

    title: windowTitle
    visible: true

    flags: {
        let value = Qt.Window;
        if (isFramelessMode)
            value |= Qt.FramelessWindowHint;
        if (alwaysOnTop)
            value |= Qt.WindowStaysOnTopHint;
        return value;
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
        color: "transparent"
        gradient: Gradient {
            GradientStop {
                id: gradientTopStop
                position: 0.0
                color: internalThemeManager.currentPaletteCollection ? internalThemeManager.currentPaletteCollection.backgroundPane : "transparent"
                Behavior on color {
                    enabled: root.enableThemeGradient
                    ColorAnimation {
                        duration: root.effectiveThemeTransitionDuration
                    }
                }
            }
            GradientStop {
                id: gradientBottomStop
                position: 1.0
                color: internalThemeManager.currentPaletteCollection ? internalThemeManager.currentPaletteCollection.secondaryPane : "transparent"
                Behavior on color {
                    enabled: root.enableThemeGradient
                    ColorAnimation {
                        duration: root.effectiveThemeTransitionDuration
                    }
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: internalThemeManager.currentPaletteCollection ? internalThemeManager.currentPaletteCollection.secondaryPane : "transparent"
            opacity: root.enableThemeGradient ? root.themeGradientOverlayOpacity : 0
            Behavior on color {
                enabled: root.enableThemeGradient
                ColorAnimation {
                    duration: root.effectiveThemeTransitionDuration
                }
            }
            Behavior on opacity {
                NumberAnimation {
                    duration: root.effectiveThemeTransitionDuration
                }
            }
        }

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
            visible: root.titleBarMode === NanWindow.CustomTitleBar && root.customTitleBarInjectSystemControls
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
                useAccentForHover: true
                accentColor: internalThemeManager.currentPaletteCollection ? internalThemeManager.currentPaletteCollection.activeBorder : "#4f8cff"
                onClicked: root.showMinimized()
            }

            TitleBarButton {
                text: root.visibility === Window.Maximized ? "❐" : "□"
                textColor: internalThemeManager.currentPaletteCollection ? internalThemeManager.currentPaletteCollection.bodyCopy : "white"
                hoverColor: internalThemeManager.currentPaletteCollection ? internalThemeManager.currentPaletteCollection.overlay0 : "#4a4a4a"
                pressedColor: internalThemeManager.currentPaletteCollection ? internalThemeManager.currentPaletteCollection.overlay1 : "#5a5a5a"
                useAccentForHover: true
                accentColor: internalThemeManager.currentPaletteCollection ? internalThemeManager.currentPaletteCollection.activeBorder : "#4f8cff"
                onClicked: {
                    if (root.visibility === Window.Maximized)
                        root.showNormal();
                    else
                        root.showMaximized();
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
            visible: root.isFramelessMode && root.useSystemResize && !root.isMaximized
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
