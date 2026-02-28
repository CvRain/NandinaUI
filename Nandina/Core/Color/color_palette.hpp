//
// Created by cvrain on 2026/2/27.
// Refactored: array storage + enum indexing for minimal boilerplate.
//

#ifndef NANDINA_COLOR_PALETTE_HPP
#define NANDINA_COLOR_PALETTE_HPP

#include <QColor>
#include <QObject>
#include <QQmlEngine>
#include <array>
#include <utility>

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

// ── Macro to generate a getter + setter pair that delegates to the array ──
// Usage: NAN_COLOR_IMPL(shade50, Shade50)
//   -> QColor shade50() const;  void setShade50(const QColor &c);
#define NAN_COLOR_IMPL(prop, EnumVal)                                                                                  \
    [[nodiscard]] QColor prop() const {                                                                                \
        return color(Types::ColorAccent::EnumVal);                                                                     \
    }                                                                                                                  \
    void set##EnumVal(const QColor &c) {                                                                               \
        setColor(Types::ColorAccent::EnumVal, c);                                                                      \
    }

namespace Nandina::Core::Color {

    namespace Types = Nandina::Types;

    /**
     * @brief Represents a single color family (e.g., primary, secondary) with
     *        11 shade levels (50-950) and corresponding contrast colors.
     *
     * All 24 color values are stored in a flat array indexed by ColorAccent enum,
     * providing both typed property access (QML binding) and programmatic
     * indexed access via color()/setColor().
     *
     * QML property access:   palette.shade500
     * C++ indexed access:    palette->color(ColorAccent::Shade500)
     */
    class NANDINA_COLOR_EXPORT ColorPalette : public QObject {
        Q_OBJECT

        // ── Shades (50-950) ──
        Q_PROPERTY(QColor shade50 READ shade50 WRITE setShade50 NOTIFY changed)
        Q_PROPERTY(QColor shade100 READ shade100 WRITE setShade100 NOTIFY changed)
        Q_PROPERTY(QColor shade200 READ shade200 WRITE setShade200 NOTIFY changed)
        Q_PROPERTY(QColor shade300 READ shade300 WRITE setShade300 NOTIFY changed)
        Q_PROPERTY(QColor shade400 READ shade400 WRITE setShade400 NOTIFY changed)
        Q_PROPERTY(QColor shade500 READ shade500 WRITE setShade500 NOTIFY changed)
        Q_PROPERTY(QColor shade600 READ shade600 WRITE setShade600 NOTIFY changed)
        Q_PROPERTY(QColor shade700 READ shade700 WRITE setShade700 NOTIFY changed)
        Q_PROPERTY(QColor shade800 READ shade800 WRITE setShade800 NOTIFY changed)
        Q_PROPERTY(QColor shade900 READ shade900 WRITE setShade900 NOTIFY changed)
        Q_PROPERTY(QColor shade950 READ shade950 WRITE setShade950 NOTIFY changed)

        // ── Contrast anchors ──
        Q_PROPERTY(QColor contrastDark READ contrastDark WRITE setContrastDark NOTIFY changed)
        Q_PROPERTY(QColor contrastLight READ contrastLight WRITE setContrastLight NOTIFY changed)

        // ── Per-shade contrast (accessibility: text color for each shade background) ──
        Q_PROPERTY(QColor contrast50 READ contrast50 WRITE setContrast50 NOTIFY changed)
        Q_PROPERTY(QColor contrast100 READ contrast100 WRITE setContrast100 NOTIFY changed)
        Q_PROPERTY(QColor contrast200 READ contrast200 WRITE setContrast200 NOTIFY changed)
        Q_PROPERTY(QColor contrast300 READ contrast300 WRITE setContrast300 NOTIFY changed)
        Q_PROPERTY(QColor contrast400 READ contrast400 WRITE setContrast400 NOTIFY changed)
        Q_PROPERTY(QColor contrast500 READ contrast500 WRITE setContrast500 NOTIFY changed)
        Q_PROPERTY(QColor contrast600 READ contrast600 WRITE setContrast600 NOTIFY changed)
        Q_PROPERTY(QColor contrast700 READ contrast700 WRITE setContrast700 NOTIFY changed)
        Q_PROPERTY(QColor contrast800 READ contrast800 WRITE setContrast800 NOTIFY changed)
        Q_PROPERTY(QColor contrast900 READ contrast900 WRITE setContrast900 NOTIFY changed)
        Q_PROPERTY(QColor contrast950 READ contrast950 WRITE setContrast950 NOTIFY changed)

    public:
        explicit ColorPalette(QObject *parent = nullptr) : QObject(parent) {
        }

        // ── Indexed access (primary API for C++ & Q_INVOKABLE for QML) ──

        /// Get color by accent enum.
        Q_INVOKABLE [[nodiscard]] QColor color(Nandina::Types::ColorAccent accent) const {
            const auto idx = std::to_underlying(accent);
            if (idx < 0 || idx >= Types::ColorAccentCount)
                return {};
            return m_colors[idx];
        }

        /// Set color by accent enum. Emits changed() if the value differs.
        Q_INVOKABLE void setColor(Nandina::Types::ColorAccent accent, const QColor &c) {
            const auto idx = std::to_underlying(accent);
            if (idx < 0 || idx >= Types::ColorAccentCount)
                return;
            if (m_colors[idx] != c) {
                m_colors[idx] = c;
                emit changed();
            }
        }

        // ── Named property accessors (generated via macro) ──

        NAN_COLOR_IMPL(shade50, Shade50)
        NAN_COLOR_IMPL(shade100, Shade100)
        NAN_COLOR_IMPL(shade200, Shade200)
        NAN_COLOR_IMPL(shade300, Shade300)
        NAN_COLOR_IMPL(shade400, Shade400)
        NAN_COLOR_IMPL(shade500, Shade500)
        NAN_COLOR_IMPL(shade600, Shade600)
        NAN_COLOR_IMPL(shade700, Shade700)
        NAN_COLOR_IMPL(shade800, Shade800)
        NAN_COLOR_IMPL(shade900, Shade900)
        NAN_COLOR_IMPL(shade950, Shade950)

        NAN_COLOR_IMPL(contrastDark, ContrastDark)
        NAN_COLOR_IMPL(contrastLight, ContrastLight)

        NAN_COLOR_IMPL(contrast50, Contrast50)
        NAN_COLOR_IMPL(contrast100, Contrast100)
        NAN_COLOR_IMPL(contrast200, Contrast200)
        NAN_COLOR_IMPL(contrast300, Contrast300)
        NAN_COLOR_IMPL(contrast400, Contrast400)
        NAN_COLOR_IMPL(contrast500, Contrast500)
        NAN_COLOR_IMPL(contrast600, Contrast600)
        NAN_COLOR_IMPL(contrast700, Contrast700)
        NAN_COLOR_IMPL(contrast800, Contrast800)
        NAN_COLOR_IMPL(contrast900, Contrast900)
        NAN_COLOR_IMPL(contrast950, Contrast950)

        /// Bulk copy all colors from another palette (single changed() emission).
        void copyFrom(const ColorPalette *other) {
            if (!other)
                return;
            m_colors = other->m_colors;
            emit changed();
        }

    signals:
        void changed();

    private:
        std::array<QColor, Types::ColorAccentCount> m_colors{};
    };

} // namespace Nandina::Core::Color

#undef NAN_COLOR_IMPL

#endif // NANDINA_COLOR_PALETTE_HPP
