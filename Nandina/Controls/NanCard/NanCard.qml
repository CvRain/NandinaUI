// NanCard.qml
// Structured card component built on NanSurface + NanPressable.
//
// Follows the shadcn/Skeleton dual philosophy:
//   ─ shadcn anatomy : header (title + description + action) → content → footer
//   ─ Skeleton presets: "outlined" | "tonal" | "filled"
//
// Preset semantics (mirrors Skeleton preset-filled / preset-tonal / preset-outlined):
//   "outlined"  — near-transparent surface bg + subtle border   [default]
//   "tonal"     — lightly tinted bg (shade 100/800) + border
//   "filled"    — solid 500-shade fill, white text, no border
//
// ── Minimal usage ────────────────────────────────────────────────────
//   NanCard {
//       width: 320
//       title: "Hello Card"
//       description: "Some helper text"
//       Text { text: "Body content goes here." }
//   }
//
// ── With image banner ────────────────────────────────────────────────
//   NanCard {
//       imageSource: "https://example.com/banner.jpg"
//       title: "Design Systems Meetup"
//       description: "A practical talk on component APIs"
//       width: 360
//   }
//
// ── Interactive / clickable ──────────────────────────────────────────
//   NanCard {
//       interactive: true
//       preset: "tonal"
//       colorVariant: "primary"
//       onClicked: console.log("card tapped")
//       Text { text: "Click me" }
//   }
//
// ── With footer row ──────────────────────────────────────────────────
//   NanCard {
//       title: "Login"
//       footer: Row {
//           Button { text: "Cancel" }
//           Button { text: "Sign in" }
//       }
//       Text { text: "Enter your credentials." }
//   }

import QtQuick
import QtQuick.Layouts
import Nandina.Theme
import Nandina.Controls

Item {
    id: root

    // ── Geometry ───────────────────────────────────────────────────
    implicitWidth: 300
    implicitHeight: _mainLayout.implicitHeight

    // ── Color ──────────────────────────────────────────────────────
    /// Semantic colour family — forwards to NanSurface.
    /// "surface" | "primary" | "secondary" | "tertiary" |
    /// "success" | "warning" | "error"
    property string colorVariant: "surface"

    // ── Preset ─────────────────────────────────────────────────────
    /// Visual fill style.  "outlined" | "tonal" | "filled"
    property string preset: "outlined"

    // ── Media ──────────────────────────────────────────────────────
    /// Optional banner image displayed above the header (edge-to-edge).
    /// Pass a local resource path or a URL string.
    property url imageSource: ""

    /// Aspect ratio (width ÷ height) of the media area. Default 21 : 9
    /// (Skeleton's wide-card convention). Use 16/9 for standard video.
    property real imageAspectRatio: 21 / 9

    // ── Header ─────────────────────────────────────────────────────
    /// Primary heading.  Hidden when empty.
    property string title: ""

    /// Secondary muted text beneath the title (CardDescription).
    property string description: ""

    /// Optional widget anchored to the top-right of the header (CardAction).
    /// Assign any Item — it will be reparented into the header action slot.
    property Item headerAction: null

    // ── Content (default slot) ──────────────────────────────────────
    /// All direct children flow into the card body.
    default property alias content: _contentArea.data

    // ── Footer ─────────────────────────────────────────────────────
    /// Optional item placed in the footer area (e.g. a Row of buttons).
    /// The item is automatically stretched to the card width.
    property Item footer: null

    // ── Layout ─────────────────────────────────────────────────────
    /// Inner padding applied to the header / content / footer sections.
    /// Defaults to 4 × spacing token (16 px at 4 px base).
    property real cardPadding: ThemeManager.primitives.spacing * 4

    /// Show full-width dividers between image ↔ header, header ↔ content,
    /// content ↔ footer — mirrors the Skeleton card divide-y convention.
    property bool showDividers: false

    // ── Interaction ─────────────────────────────────────────────────
    /// Enables hover / press / click behaviour on the whole card surface.
    /// Adds a scale-down press animation and border highlight on hover.
    property bool interactive: false

    signal clicked
    signal longPressed

    readonly property bool hovered: _pressable.hovered
    readonly property bool pressed: _pressable.pressed

    // ── Convenience read-only colours ─────────────────────────────
    readonly property color resolvedBgColor: _surface.resolvedBackgroundColor
    readonly property color resolvedBorderColor: _surface.resolvedBorderColor

    // ── Private: preset → shade mapping ────────────────────────────
    readonly property bool _isDark: ThemeManager.darkMode
    readonly property bool _isFilled: preset === "filled"

    /// Background shade derived from preset + dark-mode + interaction state.
    readonly property int _bgShade: {
        if (interactive && _pressable.pressed) {
            switch (preset) {
            case "filled":
                return 700;
            case "tonal":
                return _isDark ? 700 : 200;
            case "outlined":
                return _isDark ? 900 : 100;
            }
        }
        switch (preset) {
        case "filled":
            return 500;
        case "tonal":
            return _isDark ? 800 : 100;
        case "outlined":
            return _isDark ? 950 : 50;
        default:
            return -1;
        }
    }

    /// Border shade — brightens on hover in interactive mode.
    readonly property int _borderShade: {
        if (interactive && (_pressable.hovered || _pressable.pressed)) {
            switch (preset) {
            case "tonal":
                return _isDark ? 400 : 300;
            case "outlined":
                return _isDark ? 500 : 400;
            default:
                return 300;
            }
        }
        switch (preset) {
        case "filled":
            return 400;
        case "tonal":
            return _isDark ? 600 : 200;
        case "outlined":
            return _isDark ? 700 : 200;
        default:
            return -1;
        }
    }

    // ── Private: text colour helpers ───────────────────────────────
    readonly property color _titleColor: {
        if (_isFilled)
            return "#ffffff";
        return _isDark ? ThemeManager.primitives.headingFont.fontColorDark : ThemeManager.primitives.headingFont.fontColor;
    }

    readonly property color _descriptionColor: {
        if (_isFilled)
            return Qt.rgba(1, 1, 1, 0.65);
        return _isDark ? ThemeManager.colors.surface.shade400 : ThemeManager.colors.surface.shade500;
    }

    readonly property color _dividerColor: {
        if (_isFilled)
            return Qt.rgba(1, 1, 1, 0.20);
        return _isDark ? ThemeManager.colors.surface.shade700 : ThemeManager.colors.surface.shade200;
    }

    // ── Press-scale animation ──────────────────────────────────────
    scale: interactive && _pressable.pressed ? 0.982 : 1.0
    Behavior on scale {
        NumberAnimation {
            duration: 100
            easing.type: Easing.OutQuad
        }
    }

    // ── Background surface ─────────────────────────────────────────
    NanSurface {
        id: _surface
        anchors.fill: parent
        colorVariant: root.colorVariant
        backgroundShade: root._bgShade
        borderShade: root._borderShade
        bordered: root.preset !== "filled"
        cornerRadius: ThemeManager.primitives.radiusContainer
    }

    // ── Main layout ────────────────────────────────────────────────
    ColumnLayout {
        id: _mainLayout
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        spacing: 0

        // ── Media / image (edge-to-edge, clips to card top radius) ──
        Rectangle {
            id: _imageContainer
            visible: root.imageSource != ""
            Layout.fillWidth: true
            implicitHeight: visible ? Math.round(width / root.imageAspectRatio) : 0
            // Round only the top two corners so they follow the card outline.
            topLeftRadius: _surface.radius
            topRightRadius: _surface.radius
            clip: true
            // Placeholder colour while the image loads or if URL is invalid.
            color: _isDark ? ThemeManager.colors.surface.shade800 : ThemeManager.colors.surface.shade200

            Image {
                anchors.fill: parent
                source: root.imageSource
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                smooth: true
            }
        }

        // Divider — image → header
        Rectangle {
            visible: root.showDividers && _imageContainer.visible && _headerRow.visible
            Layout.fillWidth: true
            implicitHeight: ThemeManager.primitives.divideWidth
            color: root._dividerColor
        }

        // ── Header ────────────────────────────────────────────────
        RowLayout {
            id: _headerRow
            visible: root.title !== "" || root.description !== "" || root.headerAction !== null
            Layout.fillWidth: true
            Layout.topMargin: root.cardPadding
            Layout.leftMargin: root.cardPadding
            Layout.rightMargin: root.cardPadding
            Layout.bottomMargin: root.cardPadding * 0.5
            spacing: root.cardPadding * 0.5

            // Title + description column
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    Layout.fillWidth: true
                    visible: root.title !== ""
                    text: root.title
                    font.family: ThemeManager.primitives.headingFont.fontFamily
                    font.weight: Font.SemiBold
                    font.pixelSize: Math.round(16 * ThemeManager.primitives.textScaling)
                    color: root._titleColor
                    elide: Text.ElideRight
                    wrapMode: Text.NoWrap
                }

                Text {
                    Layout.fillWidth: true
                    visible: root.description !== ""
                    text: root.description
                    font.family: ThemeManager.primitives.baseFont.fontFamily
                    font.pixelSize: Math.round(13 * ThemeManager.primitives.textScaling)
                    color: root._descriptionColor
                    wrapMode: Text.WordWrap
                    lineHeight: 1.4
                }
            }

            // Header action slot (CardAction — top-right widget)
            Item {
                id: _headerActionSlot
                visible: root.headerAction !== null
                implicitWidth: root.headerAction ? root.headerAction.implicitWidth : 0
                implicitHeight: root.headerAction ? root.headerAction.implicitHeight : 0

                Component.onCompleted: _reparentAction()
                onVisibleChanged: _reparentAction()

                function _reparentAction() {
                    if (root.headerAction && root.headerAction.parent !== _headerActionSlot) {
                        root.headerAction.parent = _headerActionSlot;
                        root.headerAction.anchors.fill = _headerActionSlot;
                    }
                }
            }
        }

        // Divider — header → content
        Rectangle {
            visible: root.showDividers && _headerRow.visible
            Layout.fillWidth: true
            implicitHeight: ThemeManager.primitives.divideWidth
            color: root._dividerColor
        }

        // ── Content area (default slot) ────────────────────────────
        Item {
            id: _contentArea
            Layout.fillWidth: true
            Layout.topMargin: _headerRow.visible ? root.cardPadding * 0.5 : root.cardPadding
            Layout.leftMargin: root.cardPadding
            Layout.rightMargin: root.cardPadding
            Layout.bottomMargin: _footerSlot.visible ? root.cardPadding * 0.5 : root.cardPadding
            implicitHeight: childrenRect.height
        }

        // Divider — content → footer
        Rectangle {
            visible: root.showDividers && _footerSlot.visible
            Layout.fillWidth: true
            implicitHeight: ThemeManager.primitives.divideWidth
            color: root._dividerColor
        }

        // ── Footer ─────────────────────────────────────────────────
        Item {
            id: _footerSlot
            visible: root.footer !== null
            Layout.fillWidth: true
            Layout.leftMargin: root.cardPadding
            Layout.rightMargin: root.cardPadding
            Layout.topMargin: root.showDividers ? root.cardPadding * 0.75 : 0
            Layout.bottomMargin: root.cardPadding
            implicitHeight: root.footer ? root.footer.implicitHeight : 0

            Component.onCompleted: _reparentFooter()
            onVisibleChanged: _reparentFooter()

            function _reparentFooter() {
                if (root.footer && root.footer.parent !== _footerSlot) {
                    root.footer.parent = _footerSlot;
                    root.footer.width = Qt.binding(() => _footerSlot.width);
                }
            }
        }
    }

    // ── Interaction overlay (always on top) ────────────────────────
    NanPressable {
        id: _pressable
        anchors.fill: parent
        enabled: root.interactive
        onClicked: root.clicked()
        onLongPressed: root.longPressed()
    }
}
