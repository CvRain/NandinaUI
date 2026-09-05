//
// Created by ClaudeRainer on 2026/6/29.
//

#ifndef NANDINA_EXPERIMENT_NANDINA_COLOR_SPACE_HPP
#define NANDINA_EXPERIMENT_NANDINA_COLOR_SPACE_HPP

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <type_traits>

namespace nandina::foundation
{
    inline constexpr float nan_pi = 3.14159265358979323846F;
    inline constexpr float nan_epsilon = 0.000001F;

    /// sRGB color. Red, green, blue, and alpha are normalized to [0, 1].
    struct NanRgb {
        float red {0.0F};
        float green {0.0F};
        float blue {0.0F};
        float alpha {1.0F};
    };

    /// Linear RGB color. Red, green, blue, and alpha are normalized to [0, 1].
    struct NanLinearRgb {
        float red {0.0F};
        float green {0.0F};
        float blue {0.0F};
        float alpha {1.0F};
    };

    /// 8-bit sRGB color. Red, green, blue, and alpha are integers in [0, 255].
    /// 名称沿用历史（“Hex”指 0xRRGGBBAA 打包值），内部为 4 个字节分量。
    struct NanHexRgb {
        std::uint8_t red {0};
        std::uint8_t green {0};
        std::uint8_t blue {0};
        std::uint8_t alpha {255};

        /// 从 0xRRGGBBAA 打包值构造（red 在最高字节）。
        [[nodiscard]] static constexpr auto from_uint32(const std::uint32_t rgba) -> NanHexRgb {
            return {
                .red = static_cast<std::uint8_t>(rgba >> 24),
                .green = static_cast<std::uint8_t>(rgba >> 16),
                .blue = static_cast<std::uint8_t>(rgba >> 8),
                .alpha = static_cast<std::uint8_t>(rgba),
            };
        }

        /// 打包为 0xRRGGBBAA。
        [[nodiscard]] constexpr auto packed() const -> std::uint32_t {
            return (static_cast<std::uint32_t>(red) << 24)
                | (static_cast<std::uint32_t>(green) << 16)
                | (static_cast<std::uint32_t>(blue) << 8)
                | static_cast<std::uint32_t>(alpha);
        }

        auto operator==(const NanHexRgb&) const -> bool = default;
    };

    /// OKLab color. Light and alpha are normalized to [0, 1]; a and b are unbounded axes.
    struct NanOklab {
        float light {0.0F};
        float a {0.0F};
        float b {0.0F};
        float alpha {1.0F};
    };

    /// OKLCH color. Light and alpha are in [0, 1], chroma is >= 0, hue is degrees in [0, 360).
    struct NanOklch {
        float light {0.0F};
        float chroma {0.0F};
        float hue {0.0F};
        float alpha {1.0F};
    };

    /// HSV color. Hue is degrees in [0, 360); saturation, value, and alpha are in [0, 1].
    struct NanHsv {
        float hue {0.0F};
        float saturation {0.0F};
        float value {0.0F};
        float alpha {1.0F};
    };

    /// HSL color. Hue is degrees in [0, 360); saturation, lightness, and alpha are in [0, 1].
    struct NanHsl {
        float hue {0.0F};
        float saturation {0.0F};
        float lightness {0.0F};
        float alpha {1.0F};
    };

    [[nodiscard]] constexpr auto nan_clamp01(const float value) -> float {
        return std::clamp(value, 0.0F, 1.0F);
    }

    [[nodiscard]] constexpr auto nan_lerp(const float first, const float second, const float amount)
        -> float {
        return first + (second - first) * amount;
    }

    [[nodiscard]] inline auto nan_normalize_degrees(float degrees) -> float {
        auto result = std::fmod(degrees, 360.0F);
        if (result < 0.0F) {
            result += 360.0F;
        }
        return result;
    }

    [[nodiscard]] inline auto nan_srgb_to_linear_channel(float channel) -> float {
        channel = nan_clamp01(channel);
        if (channel <= 0.04045F) {
            return channel / 12.92F;
        }
        return std::pow((channel + 0.055F) / 1.055F, 2.4F);
    }

    [[nodiscard]] inline auto nan_linear_to_srgb_channel(float channel) -> float {
        channel = nan_clamp01(channel);
        if (channel <= 0.0031308F) {
            return channel * 12.92F;
        }
        return 1.055F * std::pow(channel, 1.0F / 2.4F) - 0.055F;
    }

    [[nodiscard]] constexpr auto nan_float_to_u8(const float value) -> std::uint8_t {
        return static_cast<std::uint8_t>(nan_clamp01(value) * 255.0F + 0.5F);
    }

    [[nodiscard]] inline auto
    nan_hsl_hue_to_rgb(const float chroma_minimum, const float chroma_maximum, float hue) -> float {
        hue = nan_normalize_degrees(hue);

        if (hue < 60.0F) {
            return chroma_minimum + (chroma_maximum - chroma_minimum) * hue / 60.0F;
        }
        if (hue < 180.0F) {
            return chroma_maximum;
        }
        if (hue < 240.0F) {
            return chroma_minimum + (chroma_maximum - chroma_minimum) * (240.0F - hue) / 60.0F;
        }

        return chroma_minimum;
    }

    template<typename ColorSpace>
    struct NanColorConverter;

    template<typename ColorSpace>
    concept NanColorSpace =
        requires(const std::remove_cvref_t<ColorSpace>& color, const NanOklch& oklch) {
            {
                NanColorConverter<std::remove_cvref_t<ColorSpace>>::to_oklch(color)
            } -> std::same_as<NanOklch>;
            {
                NanColorConverter<std::remove_cvref_t<ColorSpace>>::from_oklch(oklch)
            } -> std::same_as<std::remove_cvref_t<ColorSpace>>;
        };

    template<>
    struct NanColorConverter<NanOklab> {
        [[nodiscard]] static auto to_oklch(const NanOklab& color) -> NanOklch {
            const auto chroma = std::sqrt(color.a * color.a + color.b * color.b);
            auto hue = 0.0F;
            if (chroma > nan_epsilon) {
                hue = std::atan2(color.b, color.a) * 180.0F / nan_pi;
            }

            return {
                nan_clamp01(color.light),
                std::max(0.0F, chroma),
                nan_normalize_degrees(hue),
                nan_clamp01(color.alpha),
            };
        }

        [[nodiscard]] static auto from_oklch(const NanOklch& color) -> NanOklab {
            const auto hue_radians = nan_normalize_degrees(color.hue) * nan_pi / 180.0F;
            const auto chroma = std::max(0.0F, color.chroma);
            return {
                nan_clamp01(color.light),
                chroma * std::cos(hue_radians),
                chroma * std::sin(hue_radians),
                nan_clamp01(color.alpha),
            };
        }
    };

    template<>
    struct NanColorConverter<NanLinearRgb> {
        [[nodiscard]] static auto to_oklab(const NanLinearRgb& color) -> NanOklab {
            const auto red = nan_clamp01(color.red);
            const auto green = nan_clamp01(color.green);
            const auto blue = nan_clamp01(color.blue);

            const auto l = 0.4122214708F * red + 0.5363325363F * green + 0.0514459929F * blue;
            const auto m = 0.2119034982F * red + 0.6806995451F * green + 0.1073969566F * blue;
            const auto s = 0.0883024619F * red + 0.2817188376F * green + 0.6299787005F * blue;

            const auto l_root = std::cbrt(l);
            const auto m_root = std::cbrt(m);
            const auto s_root = std::cbrt(s);

            return {
                0.2104542553F * l_root + 0.7936177850F * m_root - 0.0040720468F * s_root,
                1.9779984951F * l_root - 2.4285922050F * m_root + 0.4505937099F * s_root,
                0.0259040371F * l_root + 0.7827717662F * m_root - 0.8086757660F * s_root,
                nan_clamp01(color.alpha),
            };
        }

        [[nodiscard]] static auto from_oklab(const NanOklab& color) -> NanLinearRgb {
            const auto l_root = color.light + 0.3963377774F * color.a + 0.2158037573F * color.b;
            const auto m_root = color.light - 0.1055613458F * color.a - 0.0638541728F * color.b;
            const auto s_root = color.light - 0.0894841775F * color.a - 1.2914855480F * color.b;

            const auto l = l_root * l_root * l_root;
            const auto m = m_root * m_root * m_root;
            const auto s = s_root * s_root * s_root;

            return {
                nan_clamp01(4.0767416621F * l - 3.3077115913F * m + 0.2309699292F * s),
                nan_clamp01(-1.2684380046F * l + 2.6097574011F * m - 0.3413193965F * s),
                nan_clamp01(-0.0041960863F * l - 0.7034186147F * m + 1.7076147010F * s),
                nan_clamp01(color.alpha),
            };
        }

        [[nodiscard]] static auto to_oklch(const NanLinearRgb& color) -> NanOklch {
            return NanColorConverter<NanOklab>::to_oklch(to_oklab(color));
        }

        [[nodiscard]] static auto from_oklch(const NanOklch& color) -> NanLinearRgb {
            return from_oklab(NanColorConverter<NanOklab>::from_oklch(color));
        }
    };

    template<>
    struct NanColorConverter<NanOklch> {
        [[nodiscard]] static auto to_oklch(const NanOklch& color) -> NanOklch {
            return {
                nan_clamp01(color.light),
                std::max(0.0F, color.chroma),
                nan_normalize_degrees(color.hue),
                nan_clamp01(color.alpha),
            };
        }

        [[nodiscard]] static auto from_oklch(const NanOklch& color) -> NanOklch {
            return to_oklch(color);
        }
    };

    template<>
    struct NanColorConverter<NanRgb> {
        [[nodiscard]] static auto to_linear_rgb(const NanRgb& color) -> NanLinearRgb {
            return {
                nan_srgb_to_linear_channel(color.red),
                nan_srgb_to_linear_channel(color.green),
                nan_srgb_to_linear_channel(color.blue),
                nan_clamp01(color.alpha),
            };
        }

        [[nodiscard]] static auto from_linear_rgb(const NanLinearRgb& color) -> NanRgb {
            return {
                nan_linear_to_srgb_channel(color.red),
                nan_linear_to_srgb_channel(color.green),
                nan_linear_to_srgb_channel(color.blue),
                nan_clamp01(color.alpha),
            };
        }

        [[nodiscard]] static auto to_oklch(const NanRgb& color) -> NanOklch {
            return NanColorConverter<NanLinearRgb>::to_oklch(to_linear_rgb(color));
        }

        [[nodiscard]] static auto from_oklch(const NanOklch& color) -> NanRgb {
            return from_linear_rgb(NanColorConverter<NanLinearRgb>::from_oklch(color));
        }
    };

    template<>
    struct NanColorConverter<NanHexRgb> {
        [[nodiscard]] static auto to_rgb(const NanHexRgb& color) -> NanRgb {
            return {
                static_cast<float>(color.red) / 255.0F,
                static_cast<float>(color.green) / 255.0F,
                static_cast<float>(color.blue) / 255.0F,
                static_cast<float>(color.alpha) / 255.0F,
            };
        }

        [[nodiscard]] static auto from_rgb(const NanRgb& color) -> NanHexRgb {
            return {
                nan_float_to_u8(color.red),
                nan_float_to_u8(color.green),
                nan_float_to_u8(color.blue),
                nan_float_to_u8(color.alpha),
            };
        }

        [[nodiscard]] static auto to_oklch(const NanHexRgb& color) -> NanOklch {
            return NanColorConverter<NanRgb>::to_oklch(to_rgb(color));
        }

        [[nodiscard]] static auto from_oklch(const NanOklch& color) -> NanHexRgb {
            return from_rgb(NanColorConverter<NanRgb>::from_oklch(color));
        }
    };

    template<>
    struct NanColorConverter<NanHsv> {
        [[nodiscard]] static auto to_rgb(const NanHsv& color) -> NanRgb {
            const auto hue = nan_normalize_degrees(color.hue);
            const auto saturation = nan_clamp01(color.saturation);
            const auto value = nan_clamp01(color.value);

            const auto chroma = value * saturation;
            const auto hue_sector = hue / 60.0F;
            const auto x = chroma * (1.0F - std::fabs(std::fmod(hue_sector, 2.0F) - 1.0F));
            const auto match = value - chroma;

            auto red = 0.0F;
            auto green = 0.0F;
            auto blue = 0.0F;

            if (hue_sector < 1.0F) {
                red = chroma;
                green = x;
            }
            else if (hue_sector < 2.0F) {
                red = x;
                green = chroma;
            }
            else if (hue_sector < 3.0F) {
                green = chroma;
                blue = x;
            }
            else if (hue_sector < 4.0F) {
                green = x;
                blue = chroma;
            }
            else if (hue_sector < 5.0F) {
                red = x;
                blue = chroma;
            }
            else {
                red = chroma;
                blue = x;
            }

            return {
                red + match,
                green + match,
                blue + match,
                nan_clamp01(color.alpha),
            };
        }

        [[nodiscard]] static auto from_rgb(const NanRgb& color) -> NanHsv {
            const auto red = nan_clamp01(color.red);
            const auto green = nan_clamp01(color.green);
            const auto blue = nan_clamp01(color.blue);
            const auto maximum = std::max({red, green, blue});
            const auto minimum = std::min({red, green, blue});
            const auto delta = maximum - minimum;

            auto hue = 0.0F;
            if (delta > nan_epsilon) {
                if (maximum == red) {
                    hue = 60.0F * std::fmod((green - blue) / delta, 6.0F);
                }
                else if (maximum == green) {
                    hue = 60.0F * ((blue - red) / delta + 2.0F);
                }
                else {
                    hue = 60.0F * ((red - green) / delta + 4.0F);
                }
            }

            return {
                nan_normalize_degrees(hue),
                maximum <= nan_epsilon ? 0.0F : delta / maximum,
                maximum,
                nan_clamp01(color.alpha),
            };
        }

        [[nodiscard]] static auto to_oklch(const NanHsv& color) -> NanOklch {
            return NanColorConverter<NanRgb>::to_oklch(to_rgb(color));
        }

        [[nodiscard]] static auto from_oklch(const NanOklch& color) -> NanHsv {
            return from_rgb(NanColorConverter<NanRgb>::from_oklch(color));
        }
    };

    template<>
    struct NanColorConverter<NanHsl> {
        [[nodiscard]] static auto to_rgb(const NanHsl& color) -> NanRgb {
            const auto hue = nan_normalize_degrees(color.hue);
            const auto saturation = nan_clamp01(color.saturation);
            const auto lightness = nan_clamp01(color.lightness);

            if (saturation <= nan_epsilon) {
                return {
                    lightness,
                    lightness,
                    lightness,
                    nan_clamp01(color.alpha),
                };
            }

            const auto chroma_maximum = lightness < 0.5F
                ? lightness * (1.0F + saturation)
                : lightness + saturation - lightness * saturation;
            const auto chroma_minimum = 2.0F * lightness - chroma_maximum;

            return {
                nan_hsl_hue_to_rgb(chroma_minimum, chroma_maximum, hue + 120.0F),
                nan_hsl_hue_to_rgb(chroma_minimum, chroma_maximum, hue),
                nan_hsl_hue_to_rgb(chroma_minimum, chroma_maximum, hue - 120.0F),
                nan_clamp01(color.alpha),
            };
        }

        [[nodiscard]] static auto from_rgb(const NanRgb& color) -> NanHsl {
            const auto red = nan_clamp01(color.red);
            const auto green = nan_clamp01(color.green);
            const auto blue = nan_clamp01(color.blue);
            const auto maximum = std::max({red, green, blue});
            const auto minimum = std::min({red, green, blue});
            const auto delta = maximum - minimum;

            const auto lightness = (maximum + minimum) * 0.5F;
            auto hue = 0.0F;
            auto saturation = 0.0F;

            if (delta > nan_epsilon) {
                saturation = delta / (1.0F - std::fabs(2.0F * lightness - 1.0F));

                if (maximum == red) {
                    hue = 60.0F * std::fmod((green - blue) / delta, 6.0F);
                }
                else if (maximum == green) {
                    hue = 60.0F * ((blue - red) / delta + 2.0F);
                }
                else {
                    hue = 60.0F * ((red - green) / delta + 4.0F);
                }
            }

            return {
                nan_normalize_degrees(hue),
                nan_clamp01(saturation),
                lightness,
                nan_clamp01(color.alpha),
            };
        }

        [[nodiscard]] static auto to_oklch(const NanHsl& color) -> NanOklch {
            return NanColorConverter<NanRgb>::to_oklch(to_rgb(color));
        }

        [[nodiscard]] static auto from_oklch(const NanOklch& color) -> NanHsl {
            return from_rgb(NanColorConverter<NanRgb>::from_oklch(color));
        }
    };
} // namespace nandina::foundation

#endif // NANDINA_EXPERIMENT_NANDINA_COLOR_SPACE_HPP
