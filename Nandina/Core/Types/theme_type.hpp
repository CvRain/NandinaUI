//
// Created by cvrain on 2026/3/2.
//

#ifndef NANDINA_THEME_TYPE_HPP
#define NANDINA_THEME_TYPE_HPP

#include <QQmlEngine>
#include <QtQmlIntegration/qqmlintegration.h>
#include <array>

#include "global_export.hpp"

namespace Nandina::Core::Types::ThemeVariant {
Q_NAMESPACE_EXPORT(NANDINA_EXPORT)
QML_NAMED_ELEMENT(ThemeVariant)

// ── Theme presets ──────────────────────────────────────────
enum class ThemeTypes {
    Aurora,
    Catppuccin,
    Cerberus,
    Concord,
    Crimson,
    Fennec,
    Legacy,
    AdwaitaSoft,
    Orchis,
    Count
};
Q_ENUM_NS(ThemeTypes)

// ── Color variant (7 semantic families) ────────────────────
enum class ColorVariantTypes {
    Primary,
    Secondary,
    Tertiary,
    Success,
    Warning,
    Error,
    Surface,
};
Q_ENUM_NS(ColorVariantTypes)

// ── Shade accent levels (11 steps) ─────────────────────────
enum class ColorAccentTypes {
    Accent50,
    Accent100,
    Accent200,
    Accent300,
    Accent400,
    Accent500,
    Accent600,
    Accent700,
    Accent800,
    Accent900,
    Accent950
};
Q_ENUM_NS(ColorAccentTypes)

// ── Preset variants (shared across all controls) ───────────
enum class PresetTypes {
    Filled = 0,
    Tonal = 1,
    Outlined = 2,
    Ghost = 3,
    Link = 4
};
Q_ENUM_NS(PresetTypes)

// ── Component sizes (shared across all controls) ───────────
enum class SizeTypes { Sm = 0, Md = 1, Lg = 2 };
Q_ENUM_NS(SizeTypes)

// ── Constexpr iteration helpers ────────────────────────────
inline constexpr int ThemeCount = static_cast<int>(ThemeTypes::Count);
inline constexpr int VariantCount = 7;
inline constexpr int AccentCount = 11;
inline constexpr int PresetCount = 5;
inline constexpr int SizeCount = 3;

inline constexpr std::array<ColorVariantTypes, VariantCount> AllVariants = {
        ColorVariantTypes::Primary,
        ColorVariantTypes::Secondary,
        ColorVariantTypes::Tertiary,
        ColorVariantTypes::Success,
        ColorVariantTypes::Warning,
        ColorVariantTypes::Error,
        ColorVariantTypes::Surface,
};

inline constexpr std::array<ColorAccentTypes, AccentCount> AllAccents = {
        ColorAccentTypes::Accent50,
        ColorAccentTypes::Accent100,
        ColorAccentTypes::Accent200,
        ColorAccentTypes::Accent300,
        ColorAccentTypes::Accent400,
        ColorAccentTypes::Accent500,
        ColorAccentTypes::Accent600,
        ColorAccentTypes::Accent700,
        ColorAccentTypes::Accent800,
        ColorAccentTypes::Accent900,
        ColorAccentTypes::Accent950,
};
} // namespace Nandina::Core::Types::ThemeVariant

#endif // NANDINA_THEME_TYPE_HPP
