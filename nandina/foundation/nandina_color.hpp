//
// Created by cvrain on 2026/6/27.
//

#ifndef NANDINA_EXPERIMENT_NANDINA_COLOR_HPP
#define NANDINA_EXPERIMENT_NANDINA_COLOR_HPP

#include "nandina_color_space.hpp"

namespace nandina::foundation
{
    class NanColor {
    public:
        constexpr NanColor() = default;

        explicit NanColor(const NanOklch& color):
            color_ {
                nan_clamp01(color.light),
                std::max(0.0F, color.chroma),
                nan_normalize_degrees(color.hue),
                nan_clamp01(color.alpha),
            } {}

        template<NanColorSpace ColorSpace>
        [[nodiscard]] static auto from(const ColorSpace& color) -> NanColor {
            return NanColor {NanColorConverter<std::remove_cvref_t<ColorSpace>>::to_oklch(color)};
        }

        /**
         * 从 0xRRGGBB 打包值构造颜色（alpha 单独指定，默认不透明）。
         * 颜色字节拆解收敛在此，避免调用方手写位运算。
         */
        [[nodiscard]] static auto from_hex(std::uint32_t rgb, float alpha = 1.0F) -> NanColor;

        /** 从归一化 sRGB 分量构造（[0,1]，alpha 默认不透明）。 */
        [[nodiscard]] static auto
        from_rgb(float red, float green, float blue, float alpha = 1.0F) -> NanColor {
            return from(NanRgb {.red = red, .green = green, .blue = blue, .alpha = alpha});
        }

        /** 从 OKLCH 分量构造（light/chroma/alpha 归一化，hue 为度）。 */
        [[nodiscard]] static auto
        from_oklch(float light, float chroma, float hue, float alpha = 1.0F) -> NanColor {
            return NanColor {
                NanOklch {.light = light, .chroma = chroma, .hue = hue, .alpha = alpha}
            };
        }

        /// 逐分量精确相等（含 alpha）。
        [[nodiscard]] auto operator==(const NanColor& other) const -> bool {
            return color_.light == other.color_.light && color_.chroma == other.color_.chroma
                && color_.hue == other.color_.hue && color_.alpha == other.color_.alpha;
        }

        [[nodiscard]] auto operator!=(const NanColor& other) const -> bool {
            return !(*this == other);
        }

        /// 近似相等（OKLCH 分量差 ≤ epsilon；hue 按最短弧比较）。
        [[nodiscard]] auto approx_equals(const NanColor& other, float epsilon = nan_epsilon) const
            -> bool {
            const auto a = color_;
            const auto b = other.color_;
            const auto hue_delta = std::min(
                std::abs(a.hue - b.hue),
                360.0F - std::abs(a.hue - b.hue)
            );
            return std::abs(a.light - b.light) <= epsilon
                && std::abs(a.chroma - b.chroma) <= epsilon && hue_delta <= epsilon
                && std::abs(a.alpha - b.alpha) <= epsilon;
        }

        template<NanColorSpace ColorSpace>
        [[nodiscard]] auto to() const -> ColorSpace {
            return NanColorConverter<std::remove_cvref_t<ColorSpace>>::from_oklch(color_);
        }

        [[nodiscard]] constexpr auto oklch() const -> NanOklch {
            return color_;
        }

        [[nodiscard]] constexpr auto alpha() const -> float {
            return color_.alpha;
        }

        [[nodiscard]] auto with_alpha(float alpha) const -> NanColor;

        [[nodiscard]] auto lighten(float amount) const -> NanColor;

        [[nodiscard]] auto darken(float amount) const -> NanColor;

        [[nodiscard]] auto saturate(float amount) const -> NanColor;

        [[nodiscard]] auto desaturate(float amount) const -> NanColor;

        [[nodiscard]] auto rotate_hue(float degrees) const -> NanColor;

        [[nodiscard]] auto clamped() const -> NanColor;

        [[nodiscard]] auto mix(const NanColor& other, float amount) const -> NanColor;

    private:
        NanOklch color_ {};
    };
} // namespace nandina::foundation

#endif // NANDINA_EXPERIMENT_NANDINA_COLOR_HPP
