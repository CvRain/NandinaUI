// NanPanel.qml
// Themed container panel built on top of NanSurface.
// Provides an optional titled header, a divider, and a padded content area.
//
// Basic usage:
//   NanPanel {
//       width: 320
//       Text { text: "Hello inside panel" }
//   }
//
// With header:
//   NanPanel {
//       title: "Statistics"
//       width: 320
//       Text { text: "Content goes here" }
//   }
//
// Custom padding / variant:
//   NanPanel {
//       backgroundShade: 100
//       contentPadding: 24
//       title: "Info"
//       Text { text: "..." }
//   }

import QtQuick
import QtQuick.Layouts
import Nandina.Theme
import Nandina.Controls

Item {
    id: root

    // ── Geometry ───────────────────────────────────────────────────
    implicitWidth: _layout.implicitWidth + _layout.anchors.leftMargin + _layout.anchors.rightMargin
    implicitHeight: _layout.implicitHeight + _layout.anchors.topMargin + _layout.anchors.bottomMargin

    // ── Content slot ───────────────────────────────────────────────
    /// All direct children of NanPanel are reparented here.
    default property alias content: _contentContainer.data

    // ── Header ─────────────────────────────────────────────────────
    /// Optional title string.  Leave empty ("") to hide the header.
    property string title: ""

    /// Optional Component placed in the header right-side slot.
    /// e.g.:  headerAction: NanButton { text: "Edit" }
    property Component headerAction: null

    // ── Appearance ────────────────────────────────────────────────
    /// Forwarded to NanSurface.
    property alias colorVariant: _surface.colorVariant
    property alias backgroundShade: _surface.backgroundShade
    property alias borderShade: _surface.borderShade
    property alias bordered: _surface.bordered

    /// Inner padding applied to both the header and the content area.
    /// Defaults to 3 × ThemeManager.primitives.spacing (= 12 px for 4 px unit).
    property real contentPadding: ThemeManager.primitives.spacing * 3

    // ── Convenience readonly colours (from NanSurface) ─────────────
    readonly property color resolvedBgColor: _surface.resolvedBackgroundColor
    readonly property color resolvedBorderColor: _surface.resolvedBorderColor

    // ── Background surface ─────────────────────────────────────────
    NanSurface {
        id: _surface
        anchors.fill: parent
        // Panels use the larger container radius
        cornerRadius: ThemeManager.primitives.radiusContainer
    }

    // ── Internal layout ────────────────────────────────────────────
    ColumnLayout {
        id: _layout
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        spacing: 0

        // ── Header ─────────────────────────────────────────────────
        RowLayout {
            id: _header
            visible: root.title !== "" || (root.headerAction !== null)
            Layout.fillWidth: true
            Layout.topMargin: root.contentPadding
            Layout.leftMargin: root.contentPadding
            Layout.rightMargin: root.contentPadding
            Layout.bottomMargin: root.contentPadding * 0.5
            spacing: root.contentPadding * 0.5

            Text {
                id: _titleText
                Layout.fillWidth: true
                text: root.title
                visible: root.title !== ""
                font.family: ThemeManager.primitives.headingFont.fontFamily
                font.weight: ThemeManager.primitives.headingFont.fontWeight
                font.pixelSize: 14 * ThemeManager.primitives.textScaling
                color: ThemeManager.darkMode ? ThemeManager.primitives.headingFont.fontColorDark : ThemeManager.primitives.headingFont.fontColor
                elide: Text.ElideRight
            }

            // Header action slot (用 Loader 加载 — 避免手动 reparent)
            Loader {
                id: _headerActionSlot
                visible: root.headerAction !== null
                active: root.headerAction !== null
                sourceComponent: root.headerAction
            }
        }

        // ── Divider (only when header is visible) ──────────────────
        Rectangle {
            id: _divider
            visible: _header.visible
            Layout.fillWidth: true
            implicitHeight: ThemeManager.primitives.divideWidth
            // shade300 in dark = original shade700 = visible but not glaring divider line
            color: ThemeManager.darkMode ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade200
        }

        // ── Content area ───────────────────────────────────────────
        Item {
            id: _contentContainer
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: _header.visible ? root.contentPadding * 0.75 : root.contentPadding
            Layout.leftMargin: root.contentPadding
            Layout.rightMargin: root.contentPadding
            Layout.bottomMargin: root.contentPadding
            implicitHeight: childrenRect.height
        }
    }
}
