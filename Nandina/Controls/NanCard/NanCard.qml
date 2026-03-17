// NanCard.qml
// Structured card component built on NanSurface + NanPressable.
//
// Follows the shadcn/Skeleton dual philosophy:
//   ─ shadcn anatomy : header (title + description + action) → content → footer
//   ─ Skeleton presets: Outlined | Tonal | Filled
//
// Preset semantics (mirrors Skeleton preset-filled / preset-tonal / preset-outlined):
//   Outlined — near-transparent surface bg + subtle border   [default]
//   Tonal    — lightly tinted bg (shade 100/800) + border
//   Filled   — solid 500-shade fill, white text, no border
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
import Nandina.Tokens
import Nandina.Controls

Item {
    id: root

    readonly property int _colorSurface: NanTokens.colorSurface
    readonly property int _presetFilled: NanTokens.presetFilled
    readonly property int _presetTonal: NanTokens.presetTonal
    readonly property int _presetOutlined: NanTokens.presetOutlined

    // ── Geometry ───────────────────────────────────────────────────
    implicitWidth: 300
    implicitHeight: _mainLayout.implicitHeight

    // ── Color ──────────────────────────────────────────────────────
    /// Semantic colour family — forwards to NanSurface.
    property int colorVariant: root._colorSurface

    // ── Preset ─────────────────────────────────────────────────────
    /// Visual fill style.
    property int preset: root._presetOutlined

    // ── Media ──────────────────────────────────────────────────────
    /// Optional banner image displayed above the header (edge-to-edge).
    /// Pass a local resource path or a URL string.
    property url imageSource: ""

    /// Enable banner image display above the header
    property bool enableBanner: true

    /// Aspect ratio (width ÷ height) of the media area. Default 21 : 9
    /// (Skeleton's wide-card convention). Use 16/9 for standard video.
    property real imageAspectRatio: 21 / 9

    // ── Header ─────────────────────────────────────────────────────
    /// Primary heading.  Hidden when empty.
    property string title: ""

    /// Secondary muted text beneath the title (CardDescription).
    property string description: ""

    /// Optional widget anchored to the top-right of the header (CardAction).
    /// Pass a Component — it will be instantiated into the header action slot.
    property Component headerAction: null

    // ── Content (default slot) ──────────────────────────────────────────
    /// All direct children flow into the card body.
    default property alias content: _contentArea.data

    // ── Footer ────────────────────────────────────────────────────
    /// Optional Component placed in the footer area (e.g. a Row of buttons).
    property Component footer: null

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
    readonly property bool _isFilled: preset === root._presetFilled
    readonly property var _bgShadeConfigs: [
        {
            idle: 500,
            press: 700
        },
        {
            idle: _isDark ? 200 : 100,
            press: _isDark ? 100 : 200
        },
        {
            idle: _isDark ? 100 : 50,
            press: _isDark ? 50 : 100
        }
    ]
    readonly property var _borderShadeConfigs: [
        {
            idle: 400,
            active: 300
        },
        {
            idle: _isDark ? 300 : 200,
            active: _isDark ? 400 : 300
        },
        {
            idle: _isDark ? 300 : 200,
            active: _isDark ? 500 : 400
        }
    ]

    /// Background shade derived from preset + dark-mode + interaction state.
    // Dark mode: palette is reversed — shade50=darkest, shade950=lightest.
    // Use low shade numbers so containers stay dark and non-glaring.
    readonly property int _bgShade: {
        const config = _bgShadeConfigs[preset];
        if (!config)
            return -1;
        return interactive && _pressable.pressed ? (config.press ?? config.idle) : config.idle;
    }

    /// Border shade — brightens on hover in interactive mode.
    readonly property int _borderShade: {
        const config = _borderShadeConfigs[preset];
        if (!config)
            return -1;
        return interactive && (_pressable.hovered || _pressable.pressed) ? (config.active ?? config.idle) : config.idle;
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
        // shade600 in dark mode gives adequate contrast against the dark container bg
        return _isDark ? ThemeManager.colors.surface.shade600 : ThemeManager.colors.surface.shade500;
    }

    readonly property color _dividerColor: {
        if (_isFilled)
            return Qt.rgba(1, 1, 1, 0.20);
        // shade300 in dark = original shade700 = visible but not glaring divider line
        return _isDark ? ThemeManager.colors.surface.shade300 : ThemeManager.colors.surface.shade200;
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
        bordered: root.preset !== root._presetFilled
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
            visible: root.enableBanner
            Layout.fillWidth: true
            implicitHeight: visible ? Math.round(width / root.imageAspectRatio) : 0
            // Round only the top two corners so they follow the card outline.
            topLeftRadius: _surface.radius
            topRightRadius: _surface.radius
            clip: true
            // Placeholder colour while the image loads or if URL is invalid.
            color: root._isDark ? ThemeManager.colors.surface.shade800 : ThemeManager.colors.surface.shade200

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
                    font.weight: Font.Medium
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

            // Header action slot (CardAction — 用 Loader 加载，避免手动 reparent)
            Loader {
                id: _headerActionSlot
                visible: root.headerAction !== null
                active: root.headerAction !== null
                sourceComponent: root.headerAction
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
        Loader {
            id: _footerSlot
            visible: root.footer !== null
            active: root.footer !== null
            Layout.fillWidth: true
            Layout.leftMargin: root.cardPadding
            Layout.rightMargin: root.cardPadding
            Layout.topMargin: root.showDividers ? root.cardPadding * 0.75 : 0
            Layout.bottomMargin: root.cardPadding
            sourceComponent: root.footer
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
