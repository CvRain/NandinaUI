//
// Created by cvrain on 2026/3/2.
//

#ifndef NANDINA_THEME_TYPE_HPP
#define NANDINA_THEME_TYPE_HPP

#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <array>

namespace Nandina::Core::Types {

    class ThemeVariant : public QObject {
        Q_OBJECT
        QML_ELEMENT
    public:
        explicit ThemeVariant(QObject *parent = nullptr) : QObject(parent) {}

        // ── Theme presets ──────────────────────────────────────────
        enum class ThemeTypes { catppuccin, cerberus, concord, crimson, fennec, legacy };
        Q_ENUM(ThemeTypes)

        // ── Color variant (7 semantic families) ────────────────────
        enum class ColorVariantTypes {
            primary,
            secondary,
            tertiary,
            success,
            warning,
            error,
            surface,
        };
        Q_ENUM(ColorVariantTypes)

        // ── Shade accent levels (11 steps) ─────────────────────────
        enum class ColorAccentTypes {
            accent_50,
            accent_100,
            accent_200,
            accent_300,
            accent_400,
            accent_500,
            accent_600,
            accent_700,
            accent_800,
            accent_900,
            accent_950
        };
        Q_ENUM(ColorAccentTypes)

        // ── Constexpr iteration helpers ────────────────────────────
        static constexpr int ThemeCount   = 6;
        static constexpr int VariantCount = 7;
        static constexpr int AccentCount  = 11;

        static constexpr std::array<ThemeTypes, ThemeCount> AllThemes = {
            ThemeTypes::catppuccin, ThemeTypes::cerberus, ThemeTypes::concord,
            ThemeTypes::crimson,    ThemeTypes::fennec,   ThemeTypes::legacy,
        };

        static constexpr std::array<ColorVariantTypes, VariantCount> AllVariants = {
            ColorVariantTypes::primary,   ColorVariantTypes::secondary,
            ColorVariantTypes::tertiary,  ColorVariantTypes::success,
            ColorVariantTypes::warning,   ColorVariantTypes::error,
            ColorVariantTypes::surface,
        };

        static constexpr std::array<ColorAccentTypes, AccentCount> AllAccents = {
            ColorAccentTypes::accent_50,  ColorAccentTypes::accent_100,
            ColorAccentTypes::accent_200, ColorAccentTypes::accent_300,
            ColorAccentTypes::accent_400, ColorAccentTypes::accent_500,
            ColorAccentTypes::accent_600, ColorAccentTypes::accent_700,
            ColorAccentTypes::accent_800, ColorAccentTypes::accent_900,
            ColorAccentTypes::accent_950,
        };

        // ── Name ↔ enum helpers ────────────────────────────────────
        static QString themeTypeName(ThemeTypes type) {
            switch (type) {
                case ThemeTypes::catppuccin: return QStringLiteral("catppuccin");
                case ThemeTypes::cerberus:   return QStringLiteral("cerberus");
                case ThemeTypes::concord:    return QStringLiteral("concord");
                case ThemeTypes::crimson:    return QStringLiteral("crimson");
                case ThemeTypes::fennec:     return QStringLiteral("fennec");
                case ThemeTypes::legacy:     return QStringLiteral("legacy");
            }
            return QStringLiteral("cerberus");
        }

        static ThemeTypes themeTypeFromName(const QString &name) {
            const auto lower = name.toLower();
            if (lower == u"catppuccin") return ThemeTypes::catppuccin;
            if (lower == u"cerberus")   return ThemeTypes::cerberus;
            if (lower == u"concord")    return ThemeTypes::concord;
            if (lower == u"crimson")    return ThemeTypes::crimson;
            if (lower == u"fennec")     return ThemeTypes::fennec;
            if (lower == u"legacy")     return ThemeTypes::legacy;
            return ThemeTypes::cerberus; // default fallback
        }

        static QStringList allThemeTypeNames() {
            QStringList names;
            names.reserve(ThemeCount);
            for (auto t : AllThemes) {
                names.append(themeTypeName(t));
            }
            return names;
        }
    };

} // namespace Nandina::Core::Types

#endif // NANDINA_THEME_TYPE_HPP
