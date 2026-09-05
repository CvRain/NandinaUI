//
// Created by cvrain on 2026/6/27.
//

#include "nandina_color.hpp"

#include <cmath>

namespace nandina::foundation
{
    auto NanColor::from_hex(const std::uint32_t rgb, const float alpha) -> NanColor {
        // 走浮点 sRGB 路径，避免把单独传入的 float alpha 量化到 8-bit。
        return from_rgb(
            static_cast<float>((rgb >> 16) & 0xFF) / 255.0F,
            static_cast<float>((rgb >> 8) & 0xFF) / 255.0F,
            static_cast<float>(rgb & 0xFF) / 255.0F,
            alpha
        );
    }

    auto NanColor::with_alpha(const float alpha) const -> NanColor {
        return NanColor {{
            color_.light,
            color_.chroma,
            color_.hue,
            nan_clamp01(alpha),
        }};
    }

    auto NanColor::lighten(const float amount) const -> NanColor {
        return NanColor {{
            nan_clamp01(color_.light + amount),
            color_.chroma,
            color_.hue,
            color_.alpha,
        }};
    }

    auto NanColor::darken(const float amount) const -> NanColor {
        return lighten(-amount);
    }

    auto NanColor::saturate(const float amount) const -> NanColor {
        return NanColor {{
            color_.light,
            std::max(0.0F, color_.chroma + amount),
            color_.hue,
            color_.alpha,
        }};
    }

    auto NanColor::desaturate(const float amount) const -> NanColor {
        return saturate(-amount);
    }

    auto NanColor::rotate_hue(const float degrees) const -> NanColor {
        return NanColor {{
            color_.light,
            color_.chroma,
            nan_normalize_degrees(color_.hue + degrees),
            color_.alpha,
        }};
    }

    auto NanColor::clamped() const -> NanColor {
        return NanColor {NanColorConverter<NanOklch>::to_oklch(color_)};
    }

    auto NanColor::mix(const NanColor& other, float amount) const -> NanColor {
        const auto t = nan_clamp01(amount);
        // hue 沿最短弧插值，避免 350° ↔ 10° 绕远路。
        const auto delta = nan_normalize_degrees(other.color_.hue - color_.hue);
        const auto hue = delta <= 180.0F
            ? color_.hue + delta * t
            : color_.hue - (360.0F - delta) * t;
        return NanColor {{
            nan_lerp(color_.light, other.color_.light, t),
            nan_lerp(color_.chroma, other.color_.chroma, t),
            nan_normalize_degrees(hue),
            nan_lerp(color_.alpha, other.color_.alpha, t),
        }};
    }
} // namespace nandina::foundation