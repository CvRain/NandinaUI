// NanTitleBar.qml
// Default title bar for NanWindow (frameless / custom modes).
// Shows an optional window icon, a title, and window controls
// (minimize / maximize / close).
//
// Typically created automatically by NanWindow; you can also embed it
// in a completely custom title bar as a starting point.
//
// Usage (manual):
//   NanTitleBar {
//       width: parent.width
//       height: 40
//       title: "My App"
//       targetWindow: myWindow
//   }

import QtQuick
import QtQuick.Window
import Nandina.Theme
import Nandina.Controls

Item {
    id: root

    // ── Geometry ───────────────────────────────────────────────────
    implicitHeight: 40

    // ── Required ───────────────────────────────────────────────────
    /// The window this title bar controls.
    required property Window targetWindow

    // ── Configuration ──────────────────────────────────────────────
    /// Title string shown in the centre of the title bar.
    property string title: ""

    /// Source URL for the window icon (optional).
    property url iconSource: ""

    /// Show the small icon circle / initial badge.
    property bool showIcon: true

    /// Show the minimize / maximize / close buttons.
    property bool showControls: true

    /// Allow dragging the window by clicking this bar.
    property bool draggable: true

    /// Double-click toggles maximize / restore.
    property bool doubleClickMaximize: true

    /// Top-left and top-right corner radius (should match NanWindow's radius).
    property real topRadius: 10

    // ── Private colours ────────────────────────────────────────────
    readonly property bool _isDark: ThemeManager.darkMode

    // title bar sits one shade above the body background
    readonly property color _bgColor: _isDark ? ThemeManager.colors.surface.shade100 : ThemeManager.colors.surface.shade100

    readonly property color _borderColor: _isDark ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade200

    readonly property color _titleColor: _isDark ? ThemeManager.primitives.baseFont.fontColorDark : ThemeManager.primitives.baseFont.fontColor

    readonly property color _iconBgColor: _isDark ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade300

    readonly property color _iconTextColor: _isDark ? ThemeManager.colors.surface.shade800 : ThemeManager.colors.surface.shade700

    // ── Background rectangle (only top two corners are rounded) ────
    Rectangle {
        id: _bg
        anchors.fill: parent
        color: root._bgColor
        topLeftRadius: root.topRadius
        topRightRadius: root.topRadius

        Behavior on color {
            ColorAnimation {
                duration: 120
            }
        }
    }

    // ── Bottom border / divider ────────────────────────────────────
    Rectangle {
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: ThemeManager.primitives.borderWidth
        color: root._borderColor
        Behavior on color {
            ColorAnimation {
                duration: 120
            }
        }
    }

    // ── Left side: icon + title ────────────────────────────────────
    Row {
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            leftMargin: ThemeManager.primitives.spacing * 3
        }
        spacing: ThemeManager.primitives.spacing * 2

        // Icon badge / initials circle
        Rectangle {
            visible: root.showIcon
            width: 20
            height: 20
            radius: root.iconSource.toString().length > 0 ? 4 : 10
            color: root._iconBgColor
            anchors.verticalCenter: parent.verticalCenter

            Image {
                anchors {
                    fill: parent
                    margins: 1
                }
                visible: root.iconSource.toString().length > 0
                source: root.iconSource
                fillMode: Image.PreserveAspectFit
                mipmap: true
                smooth: true
            }

            Text {
                anchors.centerIn: parent
                visible: root.iconSource.toString().length === 0
                text: root.title.length > 0 ? root.title.charAt(0).toUpperCase() : "N"
                font.pixelSize: 11
                font.weight: Font.DemiBold
                color: root._iconTextColor
            }
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.title
            font.family: ThemeManager.primitives.baseFont.fontFamily
            font.pixelSize: Math.round(13 * ThemeManager.primitives.textScaling)
            font.weight: Font.Medium
            color: root._titleColor
        }
    }

    // ── Right side: window controls ────────────────────────────────
    Row {
        visible: root.showControls
        anchors {
            verticalCenter: parent.verticalCenter
            right: parent.right
            rightMargin: ThemeManager.primitives.spacing
        }
        spacing: 2

        NanTitleBarButton {
            iconText: "−"
            implicitHeight: root.implicitHeight - 8
            onClicked: root.targetWindow.showMinimized()
        }

        NanTitleBarButton {
            iconText: root.targetWindow.visibility === Window.Maximized ? "❐" : "□"
            implicitHeight: root.implicitHeight - 8
            onClicked: {
                if (root.targetWindow.visibility === Window.Maximized)
                    root.targetWindow.showNormal();
                else
                    root.targetWindow.showMaximized();
            }
        }

        NanTitleBarButton {
            isClose: true
            iconText: "✕"
            implicitHeight: root.implicitHeight - 8
            onClicked: root.targetWindow.close()
        }
    }

    // ── Drag / double-click ────────────────────────────────────────
    DragHandler {
        id: _drag
        target: null
        enabled: root.draggable
        grabPermissions: PointerHandler.CanTakeOverFromAnything
        onActiveChanged: {
            if (active)
                root.targetWindow.startSystemMove();
        }
    }

    TapHandler {
        enabled: root.draggable && root.doubleClickMaximize
        gesturePolicy: TapHandler.DragThreshold
        onDoubleTapped: {
            if (root.targetWindow.visibility === Window.Maximized)
                root.targetWindow.showNormal();
            else
                root.targetWindow.showMaximized();
        }
    }
}
