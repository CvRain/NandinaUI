//
// Created by cvrain on 2026/2/28.
// Central type definitions for the Nandina design system.
//

#ifndef NANDINA_TYPES_HPP
#define NANDINA_TYPES_HPP

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <array>
#include <utility>

#if defined(_WIN32)
#if defined(NandinaCore_EXPORTS)
#define NANDINA_CORE_EXPORT Q_DECL_EXPORT
#else
#define NANDINA_CORE_EXPORT Q_DECL_IMPORT
#endif
#else
#define NANDINA_CORE_EXPORT
#endif

namespace Nandina::Types {

    Q_NAMESPACE_EXPORT(NANDINA_CORE_EXPORT)
    QML_NAMED_ELEMENT(NandinaType)

    // ── Theme Presets ──────────────────────────────────────────────────────

    /**
     * @brief Available built-in theme presets from Skeleton CSS.
     *
     * QML usage:
     *   ThemeManager.currentTheme = NandinaType.Cerberus
     */
    enum class ThemePreset : int {
        Catppuccin = 0,
        Cerberus,
        Concord,
        Crimson,
        Fennec,
        Legacy,
    };
    Q_ENUM_NS(ThemePreset)

    inline constexpr int ThemePresetCount = 6;

    /// All preset values for iteration.
    inline constexpr std::array<ThemePreset, ThemePresetCount> AllThemePresets = {
            ThemePreset::Catppuccin,
            ThemePreset::Cerberus,
            ThemePreset::Concord,
            ThemePreset::Crimson,
            ThemePreset::Fennec,
            ThemePreset::Legacy,
    };

    /// Convert ThemePreset to its lowercase string name.
    inline QString themePresetName(ThemePreset preset) {
        switch (preset) {
            case ThemePreset::Catppuccin:
                return QStringLiteral("catppuccin");
            case ThemePreset::Cerberus:
                return QStringLiteral("cerberus");
            case ThemePreset::Concord:
                return QStringLiteral("concord");
            case ThemePreset::Crimson:
                return QStringLiteral("crimson");
            case ThemePreset::Fennec:
                return QStringLiteral("fennec");
            case ThemePreset::Legacy:
                return QStringLiteral("legacy");
        }
        return {};
    }

    /// Parse a theme name string into ThemePreset. Returns Cerberus if unknown.
    inline ThemePreset themePresetFromName(const QString &name) {
        for (auto p: AllThemePresets) {
            if (themePresetName(p) == name)
                return p;
        }
        return ThemePreset::Cerberus; // default fallback
    }

    /// Get all preset names as a QStringList (for QML display).
    inline QStringList allThemePresetNames() {
        QStringList result;
        result.reserve(ThemePresetCount);
        for (auto p: AllThemePresets)
            result.append(themePresetName(p));
        return result;
    }

    // ── Color Variant ──────────────────────────────────────────────────────

    /**
     * @brief Color family variants in the design system.
     *
     * QML usage:
     *   ThemeManager.colors.palette(NandinaType.Primary)
     */
    enum class ColorVariant : int {
        Primary = 0,
        Secondary,
        Tertiary,
        Success,
        Warning,
        Error,
        Surface,
    };
    Q_ENUM_NS(ColorVariant)

    inline constexpr int ColorVariantCount = 7;

    /// All variant values for iteration.
    inline constexpr std::array<ColorVariant, ColorVariantCount> AllColorVariants = {
            ColorVariant::Primary,
            ColorVariant::Secondary,
            ColorVariant::Tertiary,
            ColorVariant::Success,
            ColorVariant::Warning,
            ColorVariant::Error,
            ColorVariant::Surface,
    };

    /// Convert ColorVariant to lowercase string (matches CSS family name).
    inline QString colorVariantName(ColorVariant variant) {
        switch (variant) {
            case ColorVariant::Primary:
                return QStringLiteral("primary");
            case ColorVariant::Secondary:
                return QStringLiteral("secondary");
            case ColorVariant::Tertiary:
                return QStringLiteral("tertiary");
            case ColorVariant::Success:
                return QStringLiteral("success");
            case ColorVariant::Warning:
                return QStringLiteral("warning");
            case ColorVariant::Error:
                return QStringLiteral("error");
            case ColorVariant::Surface:
                return QStringLiteral("surface");
        }
        return {};
    }

    // ── Color Accent ───────────────────────────────────────────────────────

    /**
     * @brief Shade and contrast levels within a single color family.
     *
     * QML usage:
     *   ThemeManager.colors.color(NandinaType.Primary, NandinaType.Shade500)
     */
    enum class ColorAccent : int {
        // Shade levels (light → dark)
        Shade50 = 0,
        Shade100,
        Shade200,
        Shade300,
        Shade400,
        Shade500,
        Shade600,
        Shade700,
        Shade800,
        Shade900,
        Shade950,
        // Contrast anchors
        ContrastDark,
        ContrastLight,
        // Per-shade contrast (accessibility text color)
        Contrast50,
        Contrast100,
        Contrast200,
        Contrast300,
        Contrast400,
        Contrast500,
        Contrast600,
        Contrast700,
        Contrast800,
        Contrast900,
        Contrast950,
    };
    Q_ENUM_NS(ColorAccent)

    inline constexpr int ColorAccentCount = 24;

    /// Shade-only accent values (the 11 shade levels).
    inline constexpr std::array<ColorAccent, 11> ShadeAccents = {
            ColorAccent::Shade50,
            ColorAccent::Shade100,
            ColorAccent::Shade200,
            ColorAccent::Shade300,
            ColorAccent::Shade400,
            ColorAccent::Shade500,
            ColorAccent::Shade600,
            ColorAccent::Shade700,
            ColorAccent::Shade800,
            ColorAccent::Shade900,
            ColorAccent::Shade950,
    };

    /// Numeric shade labels (for display / mapping).
    inline constexpr std::array<int, 11> ShadeLabels = {
            50,
            100,
            200,
            300,
            400,
            500,
            600,
            700,
            800,
            900,
            950,
    };

} // namespace Nandina::Types

#endif // NANDINA_TYPES_HPP
