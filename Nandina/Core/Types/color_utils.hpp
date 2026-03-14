//
// Created by cvrain on 2026/3/4.
//

#ifndef NANDINA_COLOR_UTILS_HPP
#define NANDINA_COLOR_UTILS_HPP

#include <QColor>
#include <cstdint>

namespace Nandina::Core {

    // ═══════════════════════════════════════════════════════════════
    //  Shared color conversion utilities
    //  Used by both NandinaColor and NandinaPrimitives.
    // ═══════════════════════════════════════════════════════════════

    /// Convert RGBA hex (0xRRGGBBAA) → QRgb ARGB (0xAARRGGBB).
    /// Fully constexpr — safe to use in compile-time contexts.
    [[nodiscard("discard means the conversion was pointless")]]
    constexpr uint32_t rgbaToArgb(uint32_t rgba) noexcept {
        return ((rgba >> 8u) & 0x00FFFFFFu) | ((rgba & 0xFFu) << 24u);
    }

    /// Convert RGBA hex (0xRRGGBBAA) → QColor.
    [[nodiscard("discard means the conversion was pointless")]]
    inline QColor rgbaToQColor(uint32_t rgba) noexcept {
        return QColor::fromRgba(rgbaToArgb(rgba));
    }

    /// Convert QColor → RGBA hex (0xRRGGBBAA).
    [[nodiscard("discard means the conversion was pointless")]]
    constexpr uint32_t qColorToRgba(const QColor &color) noexcept {
        return (static_cast<uint32_t>(color.red()) << 24u) | (static_cast<uint32_t>(color.green()) << 16u) |
               (static_cast<uint32_t>(color.blue()) << 8u) | static_cast<uint32_t>(color.alpha());
    }

} // namespace Nandina::Core

#endif // NANDINA_COLOR_UTILS_HPP
