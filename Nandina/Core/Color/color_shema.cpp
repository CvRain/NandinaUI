//
// Created by cvrain on 2026/3/1.
//

#include "color_shema.hpp"

namespace Nandina::Core::Color {

    // ─── ColorPalette ──────────────────────────────────────────────

    ColorPalette::ColorPalette(QObject *parent) : QObject(parent) {}

    QColor ColorPalette::shade(const int accentType) const {
        if (accentType >= 0 && accentType < AccentCount) {
            return m_shades[accentType];
        }
        return {};
    }

    void ColorPalette::setShade(const ThemeVariant::ColorAccentTypes accent, const QColor &color) {
        const int idx = static_cast<int>(accent);
        if (idx >= 0 && idx < AccentCount && m_shades[idx] != color) {
            m_shades[idx] = color;
            emit changed();
        }
    }

    void ColorPalette::setAllShades(const std::array<QColor, AccentCount> &shades) {
        if (m_shades != shades) {
            m_shades = shades;
            emit changed();
        }
    }

    // ─── ColorSchema ───────────────────────────────────────────────

    ColorSchema::ColorSchema(QObject *parent) : QObject(parent) {
        for (auto &p : m_palettes) {
            p = new ColorPalette(this);
            connect(p, &ColorPalette::changed, this, &ColorSchema::changed);
        }
    }

    ColorPalette *ColorSchema::palette(const int variantType) const {
        if (variantType >= 0 && variantType < VariantCount) {
            return m_palettes[variantType];
        }
        return nullptr;
    }

    QColor ColorSchema::color(const int variantType, const int accentType) const {
        if (auto *p = palette(variantType)) {
            return p->shade(accentType);
        }
        return {};
    }

} // namespace Nandina::Core::Color
