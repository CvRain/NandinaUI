//
// Created by cvrain on 2026/3/1.
//

#ifndef NANDINA_COLOR_SHEMA_HPP
#define NANDINA_COLOR_SHEMA_HPP

#include <QColor>
#include <QQmlEngine>
#include <array>

#include "theme_type.hpp"

namespace Nandina::Core::Color {

    using ThemeVariant = Types::ThemeVariant;
    inline constexpr int AccentCount  = ThemeVariant::AccentCount;   // 11
    inline constexpr int VariantCount = ThemeVariant::VariantCount;  // 7

    // ═══════════════════════════════════════════════════════════════
    //  ColorPalette — one semantic color family (e.g. "primary")
    //  Stores 11 shades in a flat std::array, indexed by ColorAccentTypes.
    // ═══════════════════════════════════════════════════════════════

    class ColorPalette : public QObject {
        Q_OBJECT
        QML_ELEMENT

        // ── Named shade properties (QML static binding) ───────────
        Q_PROPERTY(QColor shade50  READ shade50  NOTIFY changed)
        Q_PROPERTY(QColor shade100 READ shade100 NOTIFY changed)
        Q_PROPERTY(QColor shade200 READ shade200 NOTIFY changed)
        Q_PROPERTY(QColor shade300 READ shade300 NOTIFY changed)
        Q_PROPERTY(QColor shade400 READ shade400 NOTIFY changed)
        Q_PROPERTY(QColor shade500 READ shade500 NOTIFY changed)
        Q_PROPERTY(QColor shade600 READ shade600 NOTIFY changed)
        Q_PROPERTY(QColor shade700 READ shade700 NOTIFY changed)
        Q_PROPERTY(QColor shade800 READ shade800 NOTIFY changed)
        Q_PROPERTY(QColor shade900 READ shade900 NOTIFY changed)
        Q_PROPERTY(QColor shade950 READ shade950 NOTIFY changed)

    public:
        explicit ColorPalette(QObject *parent = nullptr);

        // Named getters
        [[nodiscard]] QColor shade50()  const { return m_shades[0]; }
        [[nodiscard]] QColor shade100() const { return m_shades[1]; }
        [[nodiscard]] QColor shade200() const { return m_shades[2]; }
        [[nodiscard]] QColor shade300() const { return m_shades[3]; }
        [[nodiscard]] QColor shade400() const { return m_shades[4]; }
        [[nodiscard]] QColor shade500() const { return m_shades[5]; }
        [[nodiscard]] QColor shade600() const { return m_shades[6]; }
        [[nodiscard]] QColor shade700() const { return m_shades[7]; }
        [[nodiscard]] QColor shade800() const { return m_shades[8]; }
        [[nodiscard]] QColor shade900() const { return m_shades[9]; }
        [[nodiscard]] QColor shade950() const { return m_shades[10]; }

        // Generic access by enum index (QML dynamic / loop scenarios)
        Q_INVOKABLE QColor shade(int accentType) const;

        // ── Mutation (used by ColorFactory) ────────────────────────
        void setShade(ThemeVariant::ColorAccentTypes accent, const QColor &color);
        void setAllShades(const std::array<QColor, AccentCount> &shades);

    signals:
        void changed();

    private:
        std::array<QColor, AccentCount> m_shades{};
    };

    // ═══════════════════════════════════════════════════════════════
    //  ColorSchema — 7 ColorPalette* pointers (one per variant).
    //  Pointers are CONSTANT (created in ctor, never replaced).
    //  Internal changed() signals bubble up for QML binding.
    // ═══════════════════════════════════════════════════════════════

    class ColorSchema : public QObject {
        Q_OBJECT
        QML_ELEMENT

        // ── Named palette properties (CONSTANT pointers) ──────────
        Q_PROPERTY(Nandina::Core::Color::ColorPalette* primary   READ primary   CONSTANT)
        Q_PROPERTY(Nandina::Core::Color::ColorPalette* secondary READ secondary CONSTANT)
        Q_PROPERTY(Nandina::Core::Color::ColorPalette* tertiary  READ tertiary  CONSTANT)
        Q_PROPERTY(Nandina::Core::Color::ColorPalette* success   READ success   CONSTANT)
        Q_PROPERTY(Nandina::Core::Color::ColorPalette* warning   READ warning   CONSTANT)
        Q_PROPERTY(Nandina::Core::Color::ColorPalette* error     READ error     CONSTANT)
        Q_PROPERTY(Nandina::Core::Color::ColorPalette* surface   READ surface   CONSTANT)

    public:
        explicit ColorSchema(QObject *parent = nullptr);

        // Named palette getters
        [[nodiscard]] ColorPalette* primary()   const { return m_palettes[0]; }
        [[nodiscard]] ColorPalette* secondary() const { return m_palettes[1]; }
        [[nodiscard]] ColorPalette* tertiary()  const { return m_palettes[2]; }
        [[nodiscard]] ColorPalette* success()   const { return m_palettes[3]; }
        [[nodiscard]] ColorPalette* warning()   const { return m_palettes[4]; }
        [[nodiscard]] ColorPalette* error()     const { return m_palettes[5]; }
        [[nodiscard]] ColorPalette* surface()   const { return m_palettes[6]; }

        // Generic access by enum index
        Q_INVOKABLE Nandina::Core::Color::ColorPalette* palette(int variantType) const;

        // Convenience: one-shot color lookup
        Q_INVOKABLE QColor color(int variantType, int accentType) const;

    signals:
        void changed();

    private:
        std::array<ColorPalette*, VariantCount> m_palettes{};
    };

} // namespace Nandina::Core::Color


#endif // NANDINA_COLOR_SHEMA_HPP
