//
// Created by cvrain on 2026/4/19.
//
module;

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <numbers>

export module nandina.foundation.nan_color_converter;

export import nandina.foundation.nan_color_schema;

/**
 * nandina.foundation.nan_color_converter
 *
 * 该模块提供两类公开能力：
 *   1. 基础颜色空间转换函数，例如 `rgb_to_oklab()`、`oklab_to_rgb()`
 *   2. `ColorSpaceTraits<T>` 与 `color::convert<From, To>()`
 *
 * 设计目标不是为每一对颜色格式单独写转换器，
 * 而是让每个颜色值对象只实现一次“进 / 出 canonical color space”。
 *
 * Example:
 *   auto oklch = nandina::color::convert<nandina::NanRgb, nandina::NanOklch>(
 *       nandina::NanRgb{35, 38, 52});
 */
export namespace nandina::color {
    /// 将浮点通道约束到 `0..1`。
    [[nodiscard]] inline auto clamp_unit(float value) noexcept -> float {
        return std::clamp(value, 0.0f, 1.0f);
    }

    /// 将色相角归一化到 `[0, 360)`。
    [[nodiscard]] inline auto normalize_hue(float hue) noexcept -> float {
        auto normalized = std::fmod(hue, 360.0f);
        if (normalized < 0.0f)
            normalized += 360.0f;
        return normalized;
    }

    [[nodiscard]] inline auto byte_to_unit(std::uint8_t value) noexcept -> float {
        return static_cast<float>(value) / 255.0f;
    }

    [[nodiscard]] inline auto unit_to_byte(float value) noexcept -> std::uint8_t {
        return static_cast<std::uint8_t>(std::lround(clamp_unit(value) * 255.0f));
    }

    [[nodiscard]] inline auto srgb_to_linear(float value) noexcept -> float {
        const auto clamped = clamp_unit(value);
        if (clamped <= 0.04045f)
            return clamped / 12.92f;
        return std::pow((clamped + 0.055f) / 1.055f, 2.4f);
    }

    [[nodiscard]] inline auto linear_to_srgb(float value) noexcept -> float {
        const auto clamped = std::max(value, 0.0f);
        if (clamped <= 0.0031308f)
            return 12.92f * clamped;
        return 1.055f * std::pow(clamped, 1.0f / 2.4f) - 0.055f;
    }

    /// 线性 RGB，中间表示，不直接作为外部 API 的主颜色空间使用。
    struct LinearRgb {
        float red;
        float green;
        float blue;
    };

    /// XYZ 中间表示。
    struct Xyz {
        float x;
        float y;
        float z;
    };

    [[nodiscard]] inline auto rgb_to_linear(const NanRgb &color) noexcept -> LinearRgb {
        return {
            .red = srgb_to_linear(byte_to_unit(color.red())),
            .green = srgb_to_linear(byte_to_unit(color.green())),
            .blue = srgb_to_linear(byte_to_unit(color.blue())),
        };
    }

    [[nodiscard]] inline auto linear_to_rgb(const LinearRgb &color, float alpha = 1.0f) noexcept -> NanRgb {
        return {
            unit_to_byte(linear_to_srgb(color.red)),
            unit_to_byte(linear_to_srgb(color.green)),
            unit_to_byte(linear_to_srgb(color.blue)),
            unit_to_byte(alpha),
        };
    }

    [[nodiscard]] inline auto linear_rgb_to_xyz(const LinearRgb &color) noexcept -> Xyz {
        return {
            .x = 0.4124564f * color.red + 0.3575761f * color.green + 0.1804375f * color.blue,
            .y = 0.2126729f * color.red + 0.7151522f * color.green + 0.0721750f * color.blue,
            .z = 0.0193339f * color.red + 0.1191920f * color.green + 0.9503041f * color.blue,
        };
    }

    [[nodiscard]] inline auto xyz_to_linear_rgb(const Xyz &color) noexcept -> LinearRgb {
        return {
            .red = 3.2404542f * color.x - 1.5371385f * color.y - 0.4985314f * color.z,
            .green = -0.9692660f * color.x + 1.8760108f * color.y + 0.0415560f * color.z,
            .blue = 0.0556434f * color.x - 0.2040259f * color.y + 1.0572252f * color.z,
        };
    }

    [[nodiscard]] inline auto rgb_to_oklab(const NanRgb &color) noexcept -> NanOklab {
        const auto linear = rgb_to_linear(color);

        const auto l = std::cbrt(0.4122214708f * linear.red + 0.5363325363f * linear.green + 0.0514459929f * linear.blue);
        const auto m = std::cbrt(0.2119034982f * linear.red + 0.6806995451f * linear.green + 0.1073969566f * linear.blue);
        const auto s = std::cbrt(0.0883024619f * linear.red + 0.2817188376f * linear.green + 0.6299787005f * linear.blue);

        return {
            0.2104542553f * l + 0.7936177850f * m - 0.0040720468f * s,
            1.9779984951f * l - 2.4285922050f * m + 0.4505937099f * s,
            0.0259040371f * l + 0.7827717662f * m - 0.8086757660f * s,
            byte_to_unit(color.alpha()),
        };
    }

    [[nodiscard]] inline auto oklab_to_rgb(const NanOklab &color) noexcept -> NanRgb {
        const auto l = color.lightness() + 0.3963377774f * color.a_axis() + 0.2158037573f * color.b_axis();
        const auto m = color.lightness() - 0.1055613458f * color.a_axis() - 0.0638541728f * color.b_axis();
        const auto s = color.lightness() - 0.0894841775f * color.a_axis() - 1.2914855480f * color.b_axis();

        const auto l3 = l * l * l;
        const auto m3 = m * m * m;
        const auto s3 = s * s * s;

        return linear_to_rgb(
            {
                .red = 4.0767416621f * l3 - 3.3077115913f * m3 + 0.2309699292f * s3,
                .green = -1.2684380046f * l3 + 2.6097574011f * m3 - 0.3413193965f * s3,
                .blue = -0.0041960863f * l3 - 0.7034186147f * m3 + 1.7076147010f * s3,
            },
            color.alpha());
    }

    [[nodiscard]] inline auto oklch_to_oklab(const NanOklch &color) noexcept -> NanOklab {
        const auto hue_rad = normalize_hue(color.hue()) * std::numbers::pi_v<float> / 180.0f;
        return {
            color.lightness(),
            color.chroma() * std::cos(hue_rad),
            color.chroma() * std::sin(hue_rad),
            color.alpha(),
        };
    }

    [[nodiscard]] inline auto oklab_to_oklch(const NanOklab &color) noexcept -> NanOklch {
        const auto chroma = std::hypot(color.a_axis(), color.b_axis());
        const auto hue = normalize_hue(std::atan2(color.b_axis(), color.a_axis()) * 180.0f / std::numbers::pi_v<float>);
        return {color.lightness(), chroma, hue, color.alpha()};
    }

    [[nodiscard]] inline auto rgb_to_hsl(const NanRgb &color) noexcept -> NanHsl {
        const auto red = byte_to_unit(color.red());
        const auto green = byte_to_unit(color.green());
        const auto blue = byte_to_unit(color.blue());

        const auto max_value = std::max({red, green, blue});
        const auto min_value = std::min({red, green, blue});
        const auto delta = max_value - min_value;
        const auto lightness = (max_value + min_value) * 0.5f;

        float hue = 0.0f;
        float saturation = 0.0f;

        if (delta > 0.0f) {
            saturation = delta / (1.0f - std::abs(2.0f * lightness - 1.0f));

            if (max_value == red)
                hue = 60.0f * std::fmod((green - blue) / delta, 6.0f);
            else if (max_value == green)
                hue = 60.0f * (((blue - red) / delta) + 2.0f);
            else
                hue = 60.0f * (((red - green) / delta) + 4.0f);
        }

        return {normalize_hue(hue), saturation, lightness, byte_to_unit(color.alpha())};
    }

    [[nodiscard]] inline auto hsl_to_rgb(const NanHsl &color) noexcept -> NanRgb {
        const auto hue = normalize_hue(color.hue());
        const auto saturation = clamp_unit(color.saturation());
        const auto lightness = clamp_unit(color.lightness());

        const auto chroma = (1.0f - std::abs(2.0f * lightness - 1.0f)) * saturation;
        const auto hue_section = hue / 60.0f;
        const auto x = chroma * (1.0f - std::abs(std::fmod(hue_section, 2.0f) - 1.0f));
        const auto match = lightness - chroma * 0.5f;

        float red = 0.0f;
        float green = 0.0f;
        float blue = 0.0f;

        if (hue_section < 1.0f) {
            red = chroma;
            green = x;
        } else if (hue_section < 2.0f) {
            red = x;
            green = chroma;
        } else if (hue_section < 3.0f) {
            green = chroma;
            blue = x;
        } else if (hue_section < 4.0f) {
            green = x;
            blue = chroma;
        } else if (hue_section < 5.0f) {
            red = x;
            blue = chroma;
        } else {
            red = chroma;
            blue = x;
        }

        return {
            unit_to_byte(red + match),
            unit_to_byte(green + match),
            unit_to_byte(blue + match),
            unit_to_byte(color.alpha()),
        };
    }

    [[nodiscard]] inline auto rgb_to_hsv(const NanRgb &color) noexcept -> NanHsv {
        const auto red = byte_to_unit(color.red());
        const auto green = byte_to_unit(color.green());
        const auto blue = byte_to_unit(color.blue());

        const auto max_value = std::max({red, green, blue});
        const auto min_value = std::min({red, green, blue});
        const auto delta = max_value - min_value;

        float hue = 0.0f;
        if (delta > 0.0f) {
            if (max_value == red)
                hue = 60.0f * std::fmod((green - blue) / delta, 6.0f);
            else if (max_value == green)
                hue = 60.0f * (((blue - red) / delta) + 2.0f);
            else
                hue = 60.0f * (((red - green) / delta) + 4.0f);
        }

        const auto saturation = max_value == 0.0f ? 0.0f : delta / max_value;
        return {normalize_hue(hue), saturation, max_value, byte_to_unit(color.alpha())};
    }

    [[nodiscard]] inline auto hsv_to_rgb(const NanHsv &color) noexcept -> NanRgb {
        const auto hue = normalize_hue(color.hue());
        const auto saturation = clamp_unit(color.saturation());
        const auto value = clamp_unit(color.value());

        const auto chroma = value * saturation;
        const auto hue_section = hue / 60.0f;
        const auto x = chroma * (1.0f - std::abs(std::fmod(hue_section, 2.0f) - 1.0f));
        const auto match = value - chroma;

        float red = 0.0f;
        float green = 0.0f;
        float blue = 0.0f;

        if (hue_section < 1.0f) {
            red = chroma;
            green = x;
        } else if (hue_section < 2.0f) {
            red = x;
            green = chroma;
        } else if (hue_section < 3.0f) {
            green = chroma;
            blue = x;
        } else if (hue_section < 4.0f) {
            green = x;
            blue = chroma;
        } else if (hue_section < 5.0f) {
            red = x;
            blue = chroma;
        } else {
            red = chroma;
            blue = x;
        }

        return {
            unit_to_byte(red + match),
            unit_to_byte(green + match),
            unit_to_byte(blue + match),
            unit_to_byte(color.alpha()),
        };
    }

    [[nodiscard]] inline auto hex_to_rgb(const NanHex &color) noexcept -> NanRgb {
        return {color.red(), color.green(), color.blue(), color.alpha()};
    }

    [[nodiscard]] inline auto rgb_to_hex(const NanRgb &color) noexcept -> NanHex {
        return {color.red(), color.green(), color.blue(), color.alpha(), color.alpha() != static_cast<std::uint8_t>(255u)};
    }

    [[nodiscard]] inline auto rgb_to_cymk(const NanRgb &color) noexcept -> NanCymk {
        const auto red = byte_to_unit(color.red());
        const auto green = byte_to_unit(color.green());
        const auto blue = byte_to_unit(color.blue());
        const auto key_black = 1.0f - std::max({red, green, blue});

        if (key_black >= 1.0f)
            return {0.0f, 0.0f, 0.0f, 1.0f, byte_to_unit(color.alpha())};

        const auto denominator = 1.0f - key_black;
        return {
            (1.0f - red - key_black) / denominator,
            (1.0f - green - key_black) / denominator,
            (1.0f - blue - key_black) / denominator,
            key_black,
            byte_to_unit(color.alpha()),
        };
    }

    [[nodiscard]] inline auto cymk_to_rgb(const NanCymk &color) noexcept -> NanRgb {
        return {
            unit_to_byte((1.0f - clamp_unit(color.cyan())) * (1.0f - clamp_unit(color.key_black()))),
            unit_to_byte((1.0f - clamp_unit(color.magenta())) * (1.0f - clamp_unit(color.key_black()))),
            unit_to_byte((1.0f - clamp_unit(color.yellow())) * (1.0f - clamp_unit(color.key_black()))),
            unit_to_byte(color.alpha()),
        };
    }

    [[nodiscard]] inline auto lab_f(float value) noexcept -> float {
        constexpr auto epsilon = 216.0f / 24389.0f;
        constexpr auto kappa = 24389.0f / 27.0f;
        if (value > epsilon)
            return std::cbrt(value);
        return (kappa * value + 16.0f) / 116.0f;
    }

    [[nodiscard]] inline auto lab_f_inverse(float value) noexcept -> float {
        constexpr auto delta = 6.0f / 29.0f;
        if (value > delta)
            return value * value * value;
        return 3.0f * delta * delta * (value - 4.0f / 29.0f);
    }

    [[nodiscard]] inline auto rgb_to_lab(const NanRgb &color) noexcept -> NanLab {
        constexpr auto white_x = 0.95047f;
        constexpr auto white_y = 1.0f;
        constexpr auto white_z = 1.08883f;

        const auto xyz = linear_rgb_to_xyz(rgb_to_linear(color));
        const auto fx = lab_f(xyz.x / white_x);
        const auto fy = lab_f(xyz.y / white_y);
        const auto fz = lab_f(xyz.z / white_z);

        return {
            116.0f * fy - 16.0f,
            500.0f * (fx - fy),
            200.0f * (fy - fz),
            byte_to_unit(color.alpha()),
        };
    }

    [[nodiscard]] inline auto lab_to_rgb(const NanLab &color) noexcept -> NanRgb {
        constexpr auto white_x = 0.95047f;
        constexpr auto white_y = 1.0f;
        constexpr auto white_z = 1.08883f;

        const auto fy = (color.lightness() + 16.0f) / 116.0f;
        const auto fx = fy + color.a_axis() / 500.0f;
        const auto fz = fy - color.b_axis() / 200.0f;

        const auto xyz = Xyz{
            .x = white_x * lab_f_inverse(fx),
            .y = white_y * lab_f_inverse(fy),
            .z = white_z * lab_f_inverse(fz),
        };

        return linear_to_rgb(xyz_to_linear_rgb(xyz), color.alpha());
    }

    /// 为颜色值对象注册“进 / 出 canonical color space”的适配规则。
    template<typename T>
    struct ColorSpaceTraits {
        static auto to_oklab(const T &) -> NanOklab = delete;
        static auto from_oklab(const NanOklab &) -> T = delete;
    };

    template<>
    struct ColorSpaceTraits<NanOklab> {
        [[nodiscard]] static auto to_oklab(const NanOklab &color) noexcept -> NanOklab { return color; }
        [[nodiscard]] static auto from_oklab(const NanOklab &color) noexcept -> NanOklab { return color; }
    };

    template<>
    struct ColorSpaceTraits<NanOklch> {
        [[nodiscard]] static auto to_oklab(const NanOklch &color) noexcept -> NanOklab { return oklch_to_oklab(color); }
        [[nodiscard]] static auto from_oklab(const NanOklab &color) noexcept -> NanOklch { return oklab_to_oklch(color); }
    };

    template<>
    struct ColorSpaceTraits<NanRgb> {
        [[nodiscard]] static auto to_oklab(const NanRgb &color) noexcept -> NanOklab { return rgb_to_oklab(color); }
        [[nodiscard]] static auto from_oklab(const NanOklab &color) noexcept -> NanRgb { return oklab_to_rgb(color); }
    };

    template<>
    struct ColorSpaceTraits<NanHsl> {
        [[nodiscard]] static auto to_oklab(const NanHsl &color) noexcept -> NanOklab { return rgb_to_oklab(hsl_to_rgb(color)); }
        [[nodiscard]] static auto from_oklab(const NanOklab &color) noexcept -> NanHsl { return rgb_to_hsl(oklab_to_rgb(color)); }
    };

    template<>
    struct ColorSpaceTraits<NanHsv> {
        [[nodiscard]] static auto to_oklab(const NanHsv &color) noexcept -> NanOklab { return rgb_to_oklab(hsv_to_rgb(color)); }
        [[nodiscard]] static auto from_oklab(const NanOklab &color) noexcept -> NanHsv { return rgb_to_hsv(oklab_to_rgb(color)); }
    };

    template<>
    struct ColorSpaceTraits<NanCymk> {
        [[nodiscard]] static auto to_oklab(const NanCymk &color) noexcept -> NanOklab { return rgb_to_oklab(cymk_to_rgb(color)); }
        [[nodiscard]] static auto from_oklab(const NanOklab &color) noexcept -> NanCymk { return rgb_to_cymk(oklab_to_rgb(color)); }
    };

    template<>
    struct ColorSpaceTraits<NanLab> {
        [[nodiscard]] static auto to_oklab(const NanLab &color) noexcept -> NanOklab { return rgb_to_oklab(lab_to_rgb(color)); }
        [[nodiscard]] static auto from_oklab(const NanOklab &color) noexcept -> NanLab { return rgb_to_lab(oklab_to_rgb(color)); }
    };

    template<>
    struct ColorSpaceTraits<NanLch> {
        [[nodiscard]] static auto to_oklab(const NanLch &color) noexcept -> NanOklab {
            const auto hue_rad = normalize_hue(color.hue()) * std::numbers::pi_v<float> / 180.0f;
            return ColorSpaceTraits<NanLab>::to_oklab({
                color.lightness(),
                color.chroma() * std::cos(hue_rad),
                color.chroma() * std::sin(hue_rad),
                color.alpha(),
            });
        }

        [[nodiscard]] static auto from_oklab(const NanOklab &color) noexcept -> NanLch {
            const auto lab = ColorSpaceTraits<NanLab>::from_oklab(color);
            const auto chroma = std::hypot(lab.a_axis(), lab.b_axis());
            const auto hue = normalize_hue(std::atan2(lab.b_axis(), lab.a_axis()) * 180.0f / std::numbers::pi_v<float>);
            return {lab.lightness(), chroma, hue, lab.alpha()};
        }
    };

    template<>
    struct ColorSpaceTraits<NanHex> {
        [[nodiscard]] static auto to_oklab(const NanHex &color) noexcept -> NanOklab { return rgb_to_oklab(hex_to_rgb(color)); }
        [[nodiscard]] static auto from_oklab(const NanOklab &color) noexcept -> NanHex { return rgb_to_hex(oklab_to_rgb(color)); }
    };

    /// 可参与统一转换链的颜色值对象。
    template<typename T>
    concept ColorSpace = std::derived_from<T, nandina::NanBaseColor> &&
        requires(const T &color, const NanOklab &oklab) {
            { ColorSpaceTraits<T>::to_oklab(color) } -> std::same_as<NanOklab>;
            { ColorSpaceTraits<T>::from_oklab(oklab) } -> std::same_as<T>;
        };

    template<typename T>
    concept ApproxComparableColor = TupleLikeColor<T> &&
        std::floating_point<typename ColorTupleTraits<T>::element_type>;

    [[nodiscard]] inline auto almost_equal(const float lhs,
                                           const float rhs,
                                           const float epsilon = 1.0e-4f) noexcept -> bool {
        return std::abs(lhs - rhs) <= epsilon;
    }

    template<ApproxComparableColor T>
    [[nodiscard]] inline auto almost_equal(const T &lhs,
                                           const T &rhs,
                                           const float epsilon = 1.0e-4f) noexcept -> bool {
        return almost_equal(get<0>(lhs), get<0>(rhs), epsilon) &&
               almost_equal(get<1>(lhs), get<1>(rhs), epsilon) &&
               almost_equal(get<2>(lhs), get<2>(rhs), epsilon) &&
               almost_equal(get<3>(lhs), get<3>(rhs), epsilon);
    }

    /**
     * 在任意两个已注册的颜色值对象之间做转换。
     *
     * Example:
     *   auto rgb = nandina::NanRgb{35, 38, 52};
     *   auto hex = nandina::color::convert<nandina::NanRgb, nandina::NanHex>(rgb);
     */
    template<ColorSpace From, ColorSpace To>
    [[nodiscard]] inline auto convert(const From &color) -> To {
        return ColorSpaceTraits<To>::from_oklab(ColorSpaceTraits<From>::to_oklab(color));
    }
}
