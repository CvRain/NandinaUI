// NanWindow.qml
// Theme-aware ApplicationWindow with three title-bar modes.
//
// ── Title bar modes ───────────────────────────────────────────────────────
//   "system"    — OS-native decoration; no custom title bar shown.
//                 Background = theme body-background color.
//   "frameless" — Qt.FramelessWindowHint. Uses the built-in NanTitleBar
//                 (icon, title, minimize/maximize/close).
//                 Rounded corners, thin border, 8-direction system resize.
//   "custom"    — Same as "frameless" but you supply your own title-bar
//                 via the titleBar property.  Optionally inject the OS
//                 window-control buttons by setting injectControls: true.
//
// ── Minimal usage ─────────────────────────────────────────────────────────
//   NanWindow {
//       width: 1024; height: 720
//       windowTitle: "My App"
//       Text { text: "Hello!"; anchors.centerIn: parent }
//   }
//
// ── Custom title bar (frameless) ──────────────────────────────────────────
//   NanWindow {
//       titleBarMode: "custom"
//       titleBar: Item {
//           // your layout here; use DragHandler { onActiveChanged: … startSystemMove() }
//       }
//   }
//
// ── Inject OS controls into a custom title bar ────────────────────────────
//   NanWindow {
//       titleBarMode: "custom"
//       injectControls: true         // adds ─ □ ✕ at the right of the title bar
//       titleBar: MyTitleBarComponent { }
//   }

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import Nandina.Theme
import Nandina.Controls

ApplicationWindow {
    id: root

    // ── Window identity ────────────────────────────────────────────
    property string windowTitle: "Nandina"
    title: windowTitle

    visible: true

    enum TitleBarModes {
        System,
        Frameless,
        Custom
    }

    // ── Title bar mode ─────────────────────────────────────────────
    /// "system" | "frameless" | "custom"
    property int titleBarMode: NanWindow.TitleBarModes.Frameless

    /// Height of the title bar in frameless / custom modes.
    property int titleBarHeight: 40

    // ── Custom title bar (used when titleBarMode === "custom") ─────
    /// Assign any Item/Component as the title bar (fills titleBarHeight).
    property Component titleBar: null

    /// When titleBarMode === "custom", overlay the built-in minimize /
    /// maximize / close buttons at the right edge of the title bar area.
    property bool injectControls: false

    /// Right margin for the injected controls row.
    property int injectControlsRightMargin: ThemeManager.primitives.spacing

    // ── Default title bar configuration ───────────────────────────
    property url windowIconSource: ""
    property bool showWindowIcon: true
    property bool titleBarDraggable: true
    property bool titleBarShowControls: true
    property bool titleBarDoubleClickMaximize: true

    // ── Window geometry ────────────────────────────────────────────
    /// Corner radius when frameless and not maximised / fullscreen.
    property real windowRadius: ThemeManager.primitives.radiusContainer + 4

    /// Pixel width of each resize hot zone (frameless only).
    property int resizeMargin: 6

    /// Use Qt's native startSystemResize (recommended true).
    property bool systemResize: true

    /// Pin window above all others.
    property bool alwaysOnTop: false

    // ── Content slot ───────────────────────────────────────────────
    /// All direct children land here (below the title bar).
    default property alias content: _contentArea.data

    // ── Derived read-only helpers ──────────────────────────────────
    readonly property bool _isFrameless: titleBarMode !== NanWindow.TitleBarModes.System
    readonly property bool _isMaximized: root.visibility === Window.Maximized
    readonly property bool _isFullscreen: root.visibility === Window.FullScreen
    readonly property real _effectiveRadius: (_isMaximized || _isFullscreen) ? 0 : windowRadius

    // ── Qt window flags ────────────────────────────────────────────
    flags: {
        let f = Qt.Window;
        if (_isFrameless)
            f |= Qt.FramelessWindowHint;
        if (alwaysOnTop)
            f |= Qt.WindowStaysOnTopHint;
        return f;
    }

    // In frameless mode the ApplicationWindow shell must be transparent
    // so the inner Rectangle's rounded corners are visible against the desktop.
    color: _isFrameless ? "transparent" : (ThemeManager.darkMode ? ThemeManager.primitives.bodyBackgroundColorDark : ThemeManager.primitives.bodyBackgroundColor)

    // ── Private colours ────────────────────────────────────────────
    readonly property bool _isDark: ThemeManager.darkMode

    readonly property color _bodyBg: _isDark ? ThemeManager.primitives.bodyBackgroundColorDark : ThemeManager.primitives.bodyBackgroundColor

    readonly property color _borderColor: _isDark ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade200

    // ── Inner root rectangle (frameless only) ─────────────────────
    // Provides rounded corners, background fill and the thin border.
    Rectangle {
        id: _frame
        anchors.fill: parent
        visible: root._isFrameless

        radius: root._effectiveRadius
        color: root._bodyBg
        border.color: root._borderColor
        border.width: 1
        clip: true

        Behavior on color {
            ColorAnimation {
                duration: 120
            }
        }
        Behavior on border.color {
            ColorAnimation {
                duration: 120
            }
        }
    }

    // ── Title bar loader ───────────────────────────────────────────
    Loader {
        id: _titleBarLoader
        visible: root._isFrameless
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        height: root._isFrameless ? root.titleBarHeight : 0

        // "custom" mode uses the user-supplied Component; else use built-in.
        sourceComponent: root._isFrameless ? (root.titleBarMode === NanWindow.TitleBarModes.Custom && root.titleBar ? root.titleBar : _defaultTitleBarComponent) : null
    }

    // ── Injected OS controls (custom mode only) ────────────────────
    Row {
        id: _injectedControls
        visible: root.titleBarMode === NanWindow.TitleBarModes.Custom && root.injectControls && root._isFrameless
        anchors {
            verticalCenter: _titleBarLoader.verticalCenter
            right: parent.right
            rightMargin: root.injectControlsRightMargin
        }
        spacing: 2
        z: 10

        NanTitleBarButton {
            iconText: "−"
            implicitHeight: root.titleBarHeight - 8
            onClicked: root.showMinimized()
        }
        NanTitleBarButton {
            iconText: root.visibility === Window.Maximized ? "❐" : "□"
            implicitHeight: root.titleBarHeight - 8
            onClicked: {
                if (root.visibility === Window.Maximized)
                    root.showNormal();
                else
                    root.showMaximized();
            }
        }
        NanTitleBarButton {
            isClose: true
            iconText: "✕"
            implicitHeight: root.titleBarHeight - 8
            onClicked: root.close()
        }
    }

    // ── Content area ───────────────────────────────────────────────
    Item {
        id: _contentArea
        anchors {
            left: parent.left
            right: parent.right
            top: root._isFrameless ? _titleBarLoader.bottom : parent.top
            bottom: parent.bottom
            // Keep content away from the rounded corners when frameless
            leftMargin: root._isFrameless && !root._isMaximized ? 1 : 0
            rightMargin: root._isFrameless && !root._isMaximized ? 1 : 0
            bottomMargin: root._isFrameless && !root._isMaximized ? 1 : 0
        }
    }

    // ── Resize handlers (frameless only) ──────────────────────────
    NanWindowResizer {
        anchors.fill: parent
        visible: root._isFrameless && root.systemResize && !root._isMaximized && !root._isFullscreen
        targetWindow: root
        resizeMargin: root.resizeMargin
        topIgnoreHeight: root.titleBarHeight
    }

    // ── Default title bar component ────────────────────────────────
    Component {
        id: _defaultTitleBarComponent

        NanTitleBar {
            width: _titleBarLoader.width
            height: _titleBarLoader.height
            title: root.windowTitle
            iconSource: root.windowIconSource
            showIcon: root.showWindowIcon
            draggable: root.titleBarDraggable
            showControls: root.titleBarShowControls
            doubleClickMaximize: root.titleBarDoubleClickMaximize
            topRadius: root._effectiveRadius
            targetWindow: root
        }
    }
}
