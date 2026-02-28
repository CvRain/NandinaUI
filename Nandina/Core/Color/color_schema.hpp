//
// Created by cvrain on 2026/2/27.
// Refactored to use ColorPalette composition with enum-based indexed access.
//

#ifndef NANDINA_COLOR_SCHEMA_HPP
#define NANDINA_COLOR_SCHEMA_HPP

#include <QObject>
#include <QQmlEngine>
#include <array>
#include <utility>

#include "color_palette.hpp"
#include "nandina_types.hpp"

#if defined(_WIN32)
#if defined(NandinaColor_EXPORTS)
#define NANDINA_COLOR_EXPORT Q_DECL_EXPORT
#else
#define NANDINA_COLOR_EXPORT Q_DECL_IMPORT
#endif
#else
#define NANDINA_COLOR_EXPORT
#endif

namespace Nandina::Core::Color {

    namespace Types = Nandina::Types;

    /**
     * @brief Complete color schema containing all 7 color families.
     *
     * Supports two access patterns:
     *
     * 1. Named property access (QML binding-friendly):
     *      ThemeManager.colors.primary.shade500
     *      ThemeManager.colors.surface.shade50
     *
     * 2. Enum-indexed access (C++ generic / QML dynamic):
     *      ThemeManager.colors.palette(NandinaType.Primary)
     *      ThemeManager.colors.color(NandinaType.Primary, NandinaType.Shade500)
     */
    class NANDINA_COLOR_EXPORT ColorSchema : public QObject {
        Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(ColorPalette *primary READ primary CONSTANT)
        Q_PROPERTY(ColorPalette *secondary READ secondary CONSTANT)
        Q_PROPERTY(ColorPalette *tertiary READ tertiary CONSTANT)
        Q_PROPERTY(ColorPalette *success READ success CONSTANT)
        Q_PROPERTY(ColorPalette *warning READ warning CONSTANT)
        Q_PROPERTY(ColorPalette *error READ error CONSTANT)
        Q_PROPERTY(ColorPalette *surface READ surface CONSTANT)

    public:
        explicit ColorSchema(QObject *parent = nullptr) : QObject(parent) {
            // Create palettes and store in both named members and indexed array
            for (auto &p: m_palettes)
                p = new ColorPalette(this);

            auto fwd = [this]() { emit changed(); };
            for (auto *p: m_palettes)
                connect(p, &ColorPalette::changed, this, fwd);
        }

        // ── Enum-indexed access ──

        /// Get palette by variant enum.
        Q_INVOKABLE [[nodiscard]] ColorPalette *palette(Nandina::Types::ColorVariant variant) const {
            const auto idx = std::to_underlying(variant);
            if (idx < 0 || idx >= Types::ColorVariantCount)
                return nullptr;
            return m_palettes[idx];
        }

        /// Get a specific color by variant + accent.
        Q_INVOKABLE [[nodiscard]] QColor color(Nandina::Types::ColorVariant variant,
                                               Nandina::Types::ColorAccent accent) const {
            auto *p = palette(variant);
            return p ? p->color(accent) : QColor{};
        }

        // ── Named property accessors ──

        [[nodiscard]] ColorPalette *primary() const {
            return m_palettes[std::to_underlying(Types::ColorVariant::Primary)];
        }
        [[nodiscard]] ColorPalette *secondary() const {
            return m_palettes[std::to_underlying(Types::ColorVariant::Secondary)];
        }
        [[nodiscard]] ColorPalette *tertiary() const {
            return m_palettes[std::to_underlying(Types::ColorVariant::Tertiary)];
        }
        [[nodiscard]] ColorPalette *success() const {
            return m_palettes[std::to_underlying(Types::ColorVariant::Success)];
        }
        [[nodiscard]] ColorPalette *warning() const {
            return m_palettes[std::to_underlying(Types::ColorVariant::Warning)];
        }
        [[nodiscard]] ColorPalette *error() const {
            return m_palettes[std::to_underlying(Types::ColorVariant::Error)];
        }
        [[nodiscard]] ColorPalette *surface() const {
            return m_palettes[std::to_underlying(Types::ColorVariant::Surface)];
        }

    signals:
        void changed();

    private:
        std::array<ColorPalette *, Types::ColorVariantCount> m_palettes{};
    };

} // namespace Nandina::Core::Color

#endif // NANDINA_COLOR_SCHEMA_HPP
