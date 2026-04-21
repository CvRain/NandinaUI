//
// Created by cvrain on 2026/4/19.
//
module;

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <tuple>

export module nandina.foundation.nan_color_schema;

/**
 * nandina.foundation.nan_color_schema
 *
 * 只定义颜色值对象本身，不负责颜色空间之间的换算。
 * 转换逻辑位于 `nandina.foundation.nan_color_converter`，
 * 统一封装入口位于 `nandina.foundation.color`。
 *
 * Example:
 *   auto rgb = nandina::NanRgb{170, 173, 111, 200};
 *   auto hex = nandina::NanHex{0xAAAD6FC8u};
 *   auto oklch = nandina::NanOklch::from_raw(0.72f, 0.12f, 110.0f);
 */
export namespace nandina::color {
    /// 解析单个十六进制字符，例如 `'A' -> 10`。
    [[nodiscard]] constexpr auto parse_hex_digit(const char value) -> std::uint8_t {
        if (value >= '0' && value <= '9')
            return static_cast<std::uint8_t>(value - '0');
        if (value >= 'a' && value <= 'f')
            return static_cast<std::uint8_t>(value - 'a' + 10);
        if (value >= 'A' && value <= 'F')
            return static_cast<std::uint8_t>(value - 'A' + 10);
        throw std::invalid_argument{"invalid hex digit"};
    }

    /// 从 `#RRGGBB` / `#RRGGBBAA` 文本里读取一个字节。
    [[nodiscard]] constexpr auto parse_hex_byte(const std::string_view text, const std::size_t offset) -> std::uint8_t {
        return static_cast<std::uint8_t>((parse_hex_digit(text[offset]) << 4u) | parse_hex_digit(text[offset + 1u]));
    }

    /// 归一化的 RGBA 解析结果，供 `NanRgb` / `NanHex` 等值对象复用。
    struct HexColorParts {
        std::uint8_t red;
        std::uint8_t green;
        std::uint8_t blue;
        std::uint8_t alpha;
        bool has_alpha;
    };

    /// 解析 `#RRGGBB` 或 `#RRGGBBAA`。
    [[nodiscard]] constexpr auto parse_hash_rgb(std::string_view text) -> HexColorParts {
        if (text.size() != 7u && text.size() != 9u)
            throw std::invalid_argument{"rgb text must be #RRGGBB or #RRGGBBAA"};
        if (text.front() != '#')
            throw std::invalid_argument{"rgb text must start with #"};

        return {
            .red = parse_hex_byte(text, 1u),
            .green = parse_hex_byte(text, 3u),
            .blue = parse_hex_byte(text, 5u),
            .alpha = text.size() == 9u ? parse_hex_byte(text, 7u) : static_cast<std::uint8_t>(255u),
            .has_alpha = text.size() == 9u,
        };
    }

    /// 解析 `0xRRGGBB` 或 `0xRRGGBBAA`。
    [[nodiscard]] constexpr auto parse_hex_value(const std::uint32_t value) -> HexColorParts {
        if (value <= 0x00FFFFFFu) {
            return {
                .red = static_cast<std::uint8_t>((value >> 16u) & 0xFFu),
                .green = static_cast<std::uint8_t>((value >> 8u) & 0xFFu),
                .blue = static_cast<std::uint8_t>(value & 0xFFu),
                .alpha = static_cast<std::uint8_t>(255u),
                .has_alpha = false,
            };
        }

        return {
            .red = static_cast<std::uint8_t>((value >> 24u) & 0xFFu),
            .green = static_cast<std::uint8_t>((value >> 16u) & 0xFFu),
            .blue = static_cast<std::uint8_t>((value >> 8u) & 0xFFu),
            .alpha = static_cast<std::uint8_t>(value & 0xFFu),
            .has_alpha = true,
        };
    }
}

export namespace nandina {
    /// 所有内置颜色值对象的共同基类，用于 traits / concept 约束。
    class NanBaseColor {
    public:
        [[nodiscard]] constexpr auto operator==(const NanBaseColor &) const noexcept -> bool = default;
    };

    /**
     * Oklab 使用直角坐标表示颜色，适合做插值和统一内部存储。
     * `NanColor` 当前就以它作为 canonical color space。
     */
    class NanOklab final : public NanBaseColor {
    public:
        constexpr
        NanOklab(const float lightness, const float a_axis, const float b_axis,
                 const float alpha = 1.0f) noexcept
            : lightness_(lightness), a_axis_(a_axis), b_axis_(b_axis), alpha_(alpha) {
        }

        constexpr NanOklab() noexcept = default;

        constexpr NanOklab(const NanOklab &) noexcept = default;

        constexpr auto operator=(const NanOklab &) noexcept -> NanOklab& = default;

        constexpr NanOklab(NanOklab &&) noexcept = default;

        constexpr auto operator=(NanOklab &&) noexcept -> NanOklab& = default;

        [[nodiscard]] constexpr auto operator==(const NanOklab &) const noexcept -> bool = default;

        [[nodiscard]] constexpr auto lightness() const noexcept -> float { return lightness_; }
        [[nodiscard]] constexpr auto a_axis() const noexcept -> float { return a_axis_; }
        [[nodiscard]] constexpr auto b_axis() const noexcept -> float { return b_axis_; }
        [[nodiscard]] constexpr auto alpha() const noexcept -> float { return alpha_; }

    private:
        float lightness_{0.0f};
        float a_axis_{0.0f};
        float b_axis_{0.0f};
        float alpha_{1.0f};
    };

    /// CIE Lab 颜色空间。
    class NanLab final : public NanBaseColor {
    public:
        constexpr
        NanLab(float lightness, float a_axis, float b_axis, float alpha = 1.0f) noexcept : lightness_(lightness),
            a_axis_(a_axis), b_axis_(b_axis), alpha_(alpha) {
        }

        constexpr NanLab() noexcept = default;

        [[nodiscard]] constexpr auto operator==(const NanLab &) const noexcept -> bool = default;

        [[nodiscard]] constexpr auto lightness() const noexcept -> float { return lightness_; }
        [[nodiscard]] constexpr auto a_axis() const noexcept -> float { return a_axis_; }
        [[nodiscard]] constexpr auto b_axis() const noexcept -> float { return b_axis_; }
        [[nodiscard]] constexpr auto alpha() const noexcept -> float { return alpha_; }

    private:
        float lightness_{0.0f};
        float a_axis_{0.0f};
        float b_axis_{0.0f};
        float alpha_{1.0f};
    };

    /**
     * Oklch 是 Oklab 的极坐标形式。
     *
     * 注意：
     *   - 构造函数与 `from_raw()` 都接受原始数值，例如 `L=0.27f, C=0.026f`
     *   - `from_css()` 接受 MDN / CSS 语义的百分比，例如 `27%, 3%, 275deg`
     *
     * Example:
     *   auto raw = nandina::NanOklch::from_raw(0.272f, 0.026f, 275.1f);
     *   auto css = nandina::NanOklch::from_css(27.0f, 3.0f, 275.0f);
     */
    class NanOklch final : public NanBaseColor {
    public:
        constexpr NanOklch(const float lightness, const float chroma, const float hue,
                           const float alpha = 1.0f) noexcept : lightness_(lightness), chroma_(chroma), hue_(hue),
                                                                alpha_(alpha) {
        }

        constexpr NanOklch() noexcept = default;

        [[nodiscard]] constexpr auto operator==(const NanOklch &) const noexcept -> bool = default;

        [[nodiscard]] static constexpr auto from_raw(float lightness,
                                                     float chroma,
                                                     float hue,
                                                     float alpha = 1.0f) noexcept -> NanOklch {
            return {lightness, chroma, hue, alpha};
        }

        [[nodiscard]] static constexpr auto from_css(float lightness_percent,
                                                     float chroma_percent,
                                                     float hue,
                                                     float alpha_percent = 100.0f) noexcept -> NanOklch {
            // CSS / MDN 语义：L 与 alpha 的 100% 映射到 1.0；
            // C 的 100% 映射到原始 chroma 0.4。
            return {
                lightness_percent / 100.0f,
                (chroma_percent / 100.0f) * 0.4f,
                hue,
                alpha_percent / 100.0f,
            };
        }

        [[nodiscard]] constexpr auto lightness() const noexcept -> float { return lightness_; }
        [[nodiscard]] constexpr auto chroma() const noexcept -> float { return chroma_; }
        [[nodiscard]] constexpr auto hue() const noexcept -> float { return hue_; }
        [[nodiscard]] constexpr auto alpha() const noexcept -> float { return alpha_; }

    private:
        float lightness_{0.0f};
        float chroma_{0.0f};
        float hue_{0.0f};
        float alpha_{1.0f};
    };

    class NanLch final : public NanBaseColor {
    public:
        constexpr NanLch(float lightness, float chroma, float hue, float alpha = 1.0f) noexcept : lightness_(lightness),
            chroma_(chroma), hue_(hue), alpha_(alpha) {
        }

        constexpr NanLch() noexcept = default;

        [[nodiscard]] constexpr auto operator==(const NanLch &) const noexcept -> bool = default;

        [[nodiscard]] constexpr auto lightness() const noexcept -> float { return lightness_; }
        [[nodiscard]] constexpr auto chroma() const noexcept -> float { return chroma_; }
        [[nodiscard]] constexpr auto hue() const noexcept -> float { return hue_; }
        [[nodiscard]] constexpr auto alpha() const noexcept -> float { return alpha_; }

    private:
        float lightness_{0.0f};
        float chroma_{0.0f};
        float hue_{0.0f};
        float alpha_{1.0f};
    };

    /**
     * sRGB 颜色值对象，通道范围为 `0..255`。
     *
     * Example:
     *   auto rgb = nandina::NanRgb{170, 173, 111, 200};
     *   auto from_text = nandina::NanRgb{"#AAAD6FC8"};
     */
    class NanRgb final : public NanBaseColor {
    public:
        constexpr NanRgb(const std::uint8_t red, const std::uint8_t green, const std::uint8_t blue,
                         const std::uint8_t alpha = 255u) noexcept
            : red_(red), green_(green), blue_(blue), alpha_(alpha) {
        }

        constexpr NanRgb() noexcept = default;

        explicit constexpr NanRgb(const std::string_view text)
            : NanRgb([](const std::string_view source) constexpr {
                return color::parse_hash_rgb(source);
            }(text)) {
        }

        [[nodiscard]] constexpr auto operator==(const NanRgb &) const noexcept -> bool = default;

        [[nodiscard]] constexpr auto red() const noexcept -> std::uint8_t { return red_; }
        [[nodiscard]] constexpr auto green() const noexcept -> std::uint8_t { return green_; }
        [[nodiscard]] constexpr auto blue() const noexcept -> std::uint8_t { return blue_; }
        [[nodiscard]] constexpr auto alpha() const noexcept -> std::uint8_t { return alpha_; }

    private:
        explicit constexpr NanRgb(const color::HexColorParts parts) noexcept : NanRgb(
            parts.red, parts.green, parts.blue, parts.alpha) {
        }

        std::uint8_t red_{0u};
        std::uint8_t green_{0u};
        std::uint8_t blue_{0u};
        std::uint8_t alpha_{255u};
    };

    /// HSL 颜色空间，`saturation` / `lightness` / `alpha` 范围通常为 `0..1`。
    class NanHsl final : public NanBaseColor {
    public:
        constexpr NanHsl(float hue, float saturation, float lightness, float alpha = 1.0f) noexcept : hue_(hue),
            saturation_(saturation), lightness_(lightness), alpha_(alpha) {
        }

        constexpr NanHsl() noexcept = default;

        [[nodiscard]] constexpr auto operator==(const NanHsl &) const noexcept -> bool = default;

        [[nodiscard]] constexpr auto hue() const noexcept -> float { return hue_; }
        [[nodiscard]] constexpr auto saturation() const noexcept -> float { return saturation_; }
        [[nodiscard]] constexpr auto lightness() const noexcept -> float { return lightness_; }
        [[nodiscard]] constexpr auto alpha() const noexcept -> float { return alpha_; }

    private:
        float hue_{0.0f};
        float saturation_{0.0f};
        float lightness_{0.0f};
        float alpha_{1.0f};
    };

    /// HSV 颜色空间，`saturation` / `value` / `alpha` 范围通常为 `0..1`。
    class NanHsv final : public NanBaseColor {
    public:
        constexpr NanHsv(const float hue, const float saturation, const float value, const float alpha = 1.0f) noexcept
            : hue_(hue), saturation_(saturation), value_(value), alpha_(alpha) {
        }

        constexpr NanHsv() noexcept = default;

        [[nodiscard]] constexpr auto operator==(const NanHsv &) const noexcept -> bool = default;

        [[nodiscard]] constexpr auto hue() const noexcept -> float { return hue_; }
        [[nodiscard]] constexpr auto saturation() const noexcept -> float { return saturation_; }
        [[nodiscard]] constexpr auto value() const noexcept -> float { return value_; }
        [[nodiscard]] constexpr auto alpha() const noexcept -> float { return alpha_; }

    private:
        float hue_{0.0f};
        float saturation_{0.0f};
        float value_{0.0f};
        float alpha_{1.0f};
    };

    /// CMYK 颜色空间，分量范围通常为 `0..1`。
    class NanCymk final : public NanBaseColor {
    public:
        constexpr NanCymk(float cyan, float magenta, float yellow, float key_black,
                          float alpha = 1.0f) noexcept : cyan_(cyan), magenta_(magenta), yellow_(yellow),
                                                         key_black_(key_black), alpha_(alpha) {
        }

        constexpr NanCymk() noexcept = default;

        [[nodiscard]] constexpr auto operator==(const NanCymk &) const noexcept -> bool = default;

        [[nodiscard]] constexpr auto cyan() const noexcept -> float { return cyan_; }
        [[nodiscard]] constexpr auto magenta() const noexcept -> float { return magenta_; }
        [[nodiscard]] constexpr auto yellow() const noexcept -> float { return yellow_; }
        [[nodiscard]] constexpr auto key_black() const noexcept -> float { return key_black_; }
        [[nodiscard]] constexpr auto alpha() const noexcept -> float { return alpha_; }

    private:
        float cyan_{0.0f};
        float magenta_{0.0f};
        float yellow_{0.0f};
        float key_black_{0.0f};
        float alpha_{1.0f};
    };

    /**
     * 十六进制颜色值对象。
     *
     * 它是一个“打包格式”入口，而不是独立的色度学颜色空间。
     * 进入转换链时会先投影到 `NanRgb`。
     *
     * Example:
     *   auto a = nandina::NanHex{0x232634u};
     *   auto b = nandina::NanHex{0xAAAD6FC8u};
     *   auto c = nandina::NanHex{"#AAAD6FC8"};
     */
    class NanHex final : public NanBaseColor {
    public:
        constexpr NanHex(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha = 255u,
                         bool has_alpha = true) noexcept : red_(red), green_(green), blue_(blue), alpha_(alpha),
                                                           has_alpha_(has_alpha) {
        }

        explicit constexpr NanHex(const std::uint32_t value) : NanHex([](const std::uint32_t source) constexpr {
            return color::parse_hex_value(source);
        }(value)) {
        }

        explicit constexpr NanHex(const std::string_view text) : NanHex([](const std::string_view source) constexpr {
            return color::parse_hash_rgb(source);
        }(text)) {
        }

        constexpr NanHex() noexcept = default;

        [[nodiscard]] constexpr auto operator==(const NanHex &) const noexcept -> bool = default;

        [[nodiscard]] constexpr auto red() const noexcept -> std::uint8_t { return red_; }
        [[nodiscard]] constexpr auto green() const noexcept -> std::uint8_t { return green_; }
        [[nodiscard]] constexpr auto blue() const noexcept -> std::uint8_t { return blue_; }
        [[nodiscard]] constexpr auto alpha() const noexcept -> std::uint8_t { return alpha_; }
        [[nodiscard]] constexpr auto has_alpha() const noexcept -> bool { return has_alpha_; }

        [[nodiscard]] constexpr auto packed() const noexcept -> std::uint32_t {
            if (has_alpha_)
                return (static_cast<std::uint32_t>(red_) << 24u) |
                       (static_cast<std::uint32_t>(green_) << 16u) |
                       (static_cast<std::uint32_t>(blue_) << 8u) |
                       static_cast<std::uint32_t>(alpha_);

            return (static_cast<std::uint32_t>(red_) << 16u) |
                   (static_cast<std::uint32_t>(green_) << 8u) |
                   static_cast<std::uint32_t>(blue_);
        }

    private:
        explicit constexpr NanHex(const color::HexColorParts parts) noexcept : NanHex(
            parts.red, parts.green, parts.blue, parts.alpha, parts.has_alpha) {
        }

        std::uint8_t red_{0u};
        std::uint8_t green_{0u};
        std::uint8_t blue_{0u};
        std::uint8_t alpha_{255u};
        bool has_alpha_{false};
    };
}

export namespace nandina::color {
    template<typename T>
    struct ColorTupleTraits;

    template<>
    struct ColorTupleTraits<nandina::NanOklab> {
        using element_type = float;

        template<std::size_t Index>
        [[nodiscard]] static constexpr auto get(const nandina::NanOklab &color) noexcept -> element_type {
            static_assert(Index < 4);
            if constexpr (Index == 0) return color.lightness();
            else if constexpr (Index == 1) return color.a_axis();
            else if constexpr (Index == 2) return color.b_axis();
            else return color.alpha();
        }
    };

    template<>
    struct ColorTupleTraits<nandina::NanLab> {
        using element_type = float;

        template<std::size_t Index>
        [[nodiscard]] static constexpr auto get(const nandina::NanLab &color) noexcept -> element_type {
            static_assert(Index < 4);
            if constexpr (Index == 0) return color.lightness();
            else if constexpr (Index == 1) return color.a_axis();
            else if constexpr (Index == 2) return color.b_axis();
            else return color.alpha();
        }
    };

    template<>
    struct ColorTupleTraits<nandina::NanOklch> {
        using element_type = float;

        template<std::size_t Index>
        [[nodiscard]] static constexpr auto get(const nandina::NanOklch &color) noexcept -> element_type {
            static_assert(Index < 4);
            if constexpr (Index == 0) return color.lightness();
            else if constexpr (Index == 1) return color.chroma();
            else if constexpr (Index == 2) return color.hue();
            else return color.alpha();
        }
    };

    template<>
    struct ColorTupleTraits<nandina::NanLch> {
        using element_type = float;

        template<std::size_t Index>
        [[nodiscard]] static constexpr auto get(const nandina::NanLch &color) noexcept -> element_type {
            static_assert(Index < 4);
            if constexpr (Index == 0) return color.lightness();
            else if constexpr (Index == 1) return color.chroma();
            else if constexpr (Index == 2) return color.hue();
            else return color.alpha();
        }
    };

    template<>
    struct ColorTupleTraits<nandina::NanRgb> {
        using element_type = std::uint8_t;

        template<std::size_t Index>
        [[nodiscard]] static constexpr auto get(const nandina::NanRgb &color) noexcept -> element_type {
            static_assert(Index < 4);
            if constexpr (Index == 0) return color.red();
            else if constexpr (Index == 1) return color.green();
            else if constexpr (Index == 2) return color.blue();
            else return color.alpha();
        }
    };

    template<>
    struct ColorTupleTraits<nandina::NanHsl> {
        using element_type = float;

        template<std::size_t Index>
        [[nodiscard]] static constexpr auto get(const nandina::NanHsl &color) noexcept -> element_type {
            static_assert(Index < 4);
            if constexpr (Index == 0) return color.hue();
            else if constexpr (Index == 1) return color.saturation();
            else if constexpr (Index == 2) return color.lightness();
            else return color.alpha();
        }
    };

    template<>
    struct ColorTupleTraits<nandina::NanHsv> {
        using element_type = float;

        template<std::size_t Index>
        [[nodiscard]] static constexpr auto get(const nandina::NanHsv &color) noexcept -> element_type {
            static_assert(Index < 4);
            if constexpr (Index == 0) return color.hue();
            else if constexpr (Index == 1) return color.saturation();
            else if constexpr (Index == 2) return color.value();
            else return color.alpha();
        }
    };

    template<>
    struct ColorTupleTraits<nandina::NanCymk> {
        using element_type = float;

        template<std::size_t Index>
        [[nodiscard]] static constexpr auto get(const nandina::NanCymk &color) noexcept -> element_type {
            static_assert(Index < 4);
            if constexpr (Index == 0) return color.cyan();
            else if constexpr (Index == 1) return color.magenta();
            else if constexpr (Index == 2) return color.yellow();
            else return color.alpha();
        }
    };

    template<>
    struct ColorTupleTraits<nandina::NanHex> {
        using element_type = std::uint8_t;

        template<std::size_t Index>
        [[nodiscard]] static constexpr auto get(const nandina::NanHex &color) noexcept -> element_type {
            static_assert(Index < 4);
            if constexpr (Index == 0) return color.red();
            else if constexpr (Index == 1) return color.green();
            else if constexpr (Index == 2) return color.blue();
            else return color.alpha();
        }
    };

    template<typename T>
    concept TupleLikeColor = requires(const T &color)
    {
        typename ColorTupleTraits<T>::element_type;
        { ColorTupleTraits<T>::template get<0>(color) } -> std::same_as<typename ColorTupleTraits<T>::element_type>;
        { ColorTupleTraits<T>::template get<1>(color) } -> std::same_as<typename ColorTupleTraits<T>::element_type>;
        { ColorTupleTraits<T>::template get<2>(color) } -> std::same_as<typename ColorTupleTraits<T>::element_type>;
        { ColorTupleTraits<T>::template get<3>(color) } -> std::same_as<typename ColorTupleTraits<T>::element_type>;
    };
}

export namespace nandina {
    template<std::size_t Index, color::TupleLikeColor T>
    [[nodiscard]] constexpr auto get(const T &color) noexcept -> typename color::ColorTupleTraits<T>::element_type {
        return color::ColorTupleTraits<T>::template get<Index>(color);
    }
}

export namespace std {
    template<>
    struct tuple_size<nandina::NanOklab> : integral_constant<size_t, 4> {
    };

    template<>
    struct tuple_size<nandina::NanLab> : integral_constant<size_t, 4> {
    };

    template<>
    struct tuple_size<nandina::NanOklch> : integral_constant<size_t, 4> {
    };

    template<>
    struct tuple_size<nandina::NanLch> : integral_constant<size_t, 4> {
    };

    template<>
    struct tuple_size<nandina::NanRgb> : integral_constant<size_t, 4> {
    };

    template<>
    struct tuple_size<nandina::NanHsl> : integral_constant<size_t, 4> {
    };

    template<>
    struct tuple_size<nandina::NanHsv> : integral_constant<size_t, 4> {
    };

    template<>
    struct tuple_size<nandina::NanCymk> : integral_constant<size_t, 4> {
    };

    template<>
    struct tuple_size<nandina::NanHex> : integral_constant<size_t, 4> {
    };

    template<size_t Index>
    struct tuple_element<Index, nandina::NanOklab> {
        using type = float;
    };

    template<size_t Index>
    struct tuple_element<Index, nandina::NanLab> {
        using type = float;
    };

    template<size_t Index>
    struct tuple_element<Index, nandina::NanOklch> {
        using type = float;
    };

    template<size_t Index>
    struct tuple_element<Index, nandina::NanLch> {
        using type = float;
    };

    template<size_t Index>
    struct tuple_element<Index, nandina::NanRgb> {
        using type = std::uint8_t;
    };

    template<size_t Index>
    struct tuple_element<Index, nandina::NanHsl> {
        using type = float;
    };

    template<size_t Index>
    struct tuple_element<Index, nandina::NanHsv> {
        using type = float;
    };

    template<size_t Index>
    struct tuple_element<Index, nandina::NanCymk> {
        using type = float;
    };

    template<size_t Index>
    struct tuple_element<Index, nandina::NanHex> {
        using type = std::uint8_t;
    };
}
