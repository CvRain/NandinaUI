//
// Created by cvrain on 2026/3/2.
//

#ifndef NANDINA_THEME_TYPE_HPP
#define NANDINA_THEME_TYPE_HPP

#include <QQmlEngine>
#include <array>

namespace Nandina::Core::Types {

    class ThemeVariant : public QObject {
        Q_OBJECT
        QML_ELEMENT
    public:
        explicit ThemeVariant(QObject *parent = nullptr) : QObject(parent) {
        }

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
        Q_ENUM(ThemeTypes)

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
        Q_ENUM(ColorVariantTypes)

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
        Q_ENUM(ColorAccentTypes)

        // ── Preset variants (shared across all controls) ───────────
        // Maps directly to the visual fill styles used by NanBadge, NanButton, NanCard, etc.
        enum class PresetTypes {
            Filled = 0, // solid shade-500 fill
            Tonal = 1, // lightly tinted bg (shade-100/200)
            Outlined = 2, // transparent bg + coloured border
            Ghost = 3, // near-invisible; hover-only tint
            Link = 4 // text-only with underline (buttons only)
        };
        Q_ENUM(PresetTypes)

        // ── Component sizes (shared across all controls) ───────────
        enum class SizeTypes { Sm = 0, Md = 1, Lg = 2 };
        Q_ENUM(SizeTypes)

        // ── Constexpr iteration helpers ────────────────────────────
        static constexpr int ThemeCount = static_cast<int>(ThemeTypes::Count);
        static constexpr int VariantCount = 7;
        static constexpr int AccentCount = 11;
        static constexpr int PresetCount = 5;
        static constexpr int SizeCount = 3;

        static constexpr std::array<ColorVariantTypes, VariantCount> AllVariants = {
                ColorVariantTypes::Primary,
                ColorVariantTypes::Secondary,
                ColorVariantTypes::Tertiary,
                ColorVariantTypes::Success,
                ColorVariantTypes::Warning,
                ColorVariantTypes::Error,
                ColorVariantTypes::Surface,
        };

        static constexpr std::array<ColorAccentTypes, AccentCount> AllAccents = {
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
    };

} // namespace Nandina::Core::Types

#endif // NANDINA_THEME_TYPE_HPP
