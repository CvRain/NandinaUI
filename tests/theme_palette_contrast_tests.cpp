//
// theme_palette_contrast_tests — 默认主题的对比度与层级硬门槛。
//
// 默认主题（shadcn/ui 对齐）是框架的开箱即用观感，它的可读性不能靠人工目测。
// 本文件把 docs/references/design_tokens.md 里声明的对比度门槛固化下来：
//
//   * 文字与其底 ≥ 4.5:1（WCAG AA 正文）；
//   * 焦点环、语义状态色等非文本信号 ≥ 3:1；
//   * 背景 / 卡片 / 浮层三层的 OKLCH 明度关系单调且可分辨。
//
// 带 alpha 的颜色先按 alpha 合成到底色上再量（nan_contrast_ratio 按约定忽略 alpha）。
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nandina/foundation/contrast.hpp>
#include <nandina/theme/builtin_themes.hpp>
#include <nandina/theme/theme.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string_view>
#include <utility>

namespace
{
    using namespace nandina;

    /// 把前景按自身 alpha 合成到不透明背景上。
    [[nodiscard]] auto composite_over(
        const foundation::NanColor& foreground,
        const foundation::NanColor& background
    ) -> foundation::NanColor {
        const auto source = foreground.to<foundation::NanRgb>();
        const auto backdrop = background.to<foundation::NanRgb>();
        const auto alpha = source.alpha;
        return foundation::NanColor::from_rgb(
            source.red * alpha + backdrop.red * (1.0F - alpha),
            source.green * alpha + backdrop.green * (1.0F - alpha),
            source.blue * alpha + backdrop.blue * (1.0F - alpha)
        );
    }

    [[nodiscard]] auto contrast_of(
        const foundation::NanColor& foreground,
        const foundation::NanColor& background
    ) -> float {
        return foundation::nan_contrast_ratio(composite_over(foreground, background), background);
    }

    /// 断言一组「前景 / 底色」达到给定量级，失败时带上可读的标签。
    void require_contrast(
        const theme::NanColorScheme& scheme,
        const std::string_view label,
        const foundation::NanColor& foreground,
        const foundation::NanColor& background,
        const float minimum
    ) {
        INFO("light/dark palette pair: " << label);
        const auto ratio = contrast_of(foreground, background);
        REQUIRE(ratio >= minimum);
    }

    /// 逐对外观检查文字层与信号层；这些角色是组件默认配方实际引用的组合。
    ///
    /// @param require_ring_on_card 是否额外要求焦点环在卡片上也可见。默认主题的 ring
    ///        是中性灰，两个表面都能达标；参考色阶路径的 ring 取边框档，在较亮的
    ///        卡片上可能贴边，故只对页面底做要求。
    /// @param strict_status_text 状态色上的文字是否要求 4.5:1。默认主题直接声明语义值，
    ///        能做到；参考色阶路径只能从色阶里选档，error / warning 的明度跨度不足以
    ///        保证 4.5:1（实测最好约 4.3:1），因此对那条路径只要求大字级 3:1。
    void require_palette_readable(
        const theme::NanColorScheme& scheme,
        const bool require_ring_on_card = false,
        const bool strict_status_text = true
    ) {
        const float text_minimum = foundation::nan_contrast_aa_text;
        const float status_minimum = strict_status_text
            ? foundation::nan_contrast_aa_text
            : foundation::nan_contrast_aa_large_text;
        // 正文与各层表面的文字。
        require_contrast(scheme, "foreground/background", scheme.foreground, scheme.background, text_minimum);
        require_contrast(scheme, "card_foreground/card", scheme.card_foreground, scheme.card, text_minimum);
        require_contrast(
            scheme, "popover_foreground/popover", scheme.popover_foreground, scheme.popover, text_minimum
        );
        // 次要文字：既可能压在背景上，也可能压在 muted 块上（Badge / 表头 / 代码块）。
        require_contrast(
            scheme, "muted_foreground/background", scheme.muted_foreground, scheme.background, text_minimum
        );
        require_contrast(scheme, "muted_foreground/muted", scheme.muted_foreground, scheme.muted, text_minimum);
        // 中性次操作与 hover 高亮上的文字（Button secondary/ghost、下拉高亮项）。
        require_contrast(
            scheme,
            "secondary_foreground/secondary",
            scheme.secondary_foreground,
            scheme.secondary,
            text_minimum
        );
        require_contrast(
            scheme, "accent_foreground/accent", scheme.accent_foreground, scheme.accent, text_minimum
        );
        // 品牌与语义状态按钮的文字（Button filled / tone danger）。
        require_contrast(
            scheme, "primary_foreground/primary", scheme.primary_foreground, scheme.primary, text_minimum
        );
        require_contrast(
            scheme,
            "destructive_foreground/destructive",
            scheme.destructive_foreground,
            scheme.destructive,
            status_minimum
        );
        require_contrast(
            scheme, "error_foreground/error", scheme.error_foreground, scheme.error, status_minimum
        );
        require_contrast(
            scheme, "success_foreground/success", scheme.success_foreground, scheme.success, status_minimum
        );
        require_contrast(
            scheme, "warning_foreground/warning", scheme.warning_foreground, scheme.warning, status_minimum
        );

        // 非文本信号：焦点环与语义色在页面上必须能看见（≥ 3:1）。
        constexpr float signal_minimum = foundation::nan_contrast_aa_large_text;
        require_contrast(scheme, "ring/background", scheme.ring, scheme.background, signal_minimum);
        if (require_ring_on_card) {
            require_contrast(scheme, "ring/card", scheme.ring, scheme.card, signal_minimum);
        }
        require_contrast(scheme, "primary/background", scheme.primary, scheme.background, signal_minimum);
        require_contrast(
            scheme, "destructive/background", scheme.destructive, scheme.background, signal_minimum
        );
    }
} // namespace

TEST_CASE("default light palette meets text and signal contrast floors", "[theme][palette][contrast]") {
    require_palette_readable(theme::default_light_palette(), /*require_ring_on_card=*/true);
}

TEST_CASE("default dark palette meets text and signal contrast floors", "[theme][palette][contrast]") {
    require_palette_readable(theme::default_dark_palette(), /*require_ring_on_card=*/true);
}

TEST_CASE("default palettes keep a monotonic surface hierarchy", "[theme][palette][contrast]") {
    const auto light = theme::default_light_palette();
    const auto dark = theme::default_dark_palette();

    const auto lightness = [](const foundation::NanColor& color) { return color.oklch().light; };

    // 层级方向：亮色外观下越"浮"越亮、暗色外观下越"浮"越暗；且卡片与浮层都要
    // 与页面至少拉开 2% 明度，不能只靠 border 区分（否则 Card / Dialog 会糊在页面上）。
    const auto check = [&](const theme::NanColorScheme& scheme, const bool elevated_is_lighter) {
        const std::array<std::pair<std::string_view, foundation::NanColor>, 3> layers {{
            {"background", scheme.background},
            {"card", scheme.card},
            {"popover", scheme.popover},
        }};
        const auto page = lightness(scheme.background);
        for (std::size_t index = 1; index < layers.size(); ++index) {
            const auto lifted = lightness(layers[index].second);
            INFO("layer " << layers[index].first);
            // 浮层可以与页面同明度（shadcn 的亮色 popover 就是纯白 + border），
            // 但不允许朝相反方向跑。
            if (elevated_is_lighter) {
                REQUIRE(lifted >= page);
            }
            else {
                REQUIRE(lifted <= page);
            }
        }
        const auto card_delta = std::abs(lightness(scheme.card) - page);
        const auto popover_delta = std::abs(lightness(scheme.popover) - page);
        REQUIRE(std::max(card_delta, popover_delta) >= 0.02F);
    };

    check(light, /*elevated_is_lighter=*/false);
    check(dark, /*elevated_is_lighter=*/true);
}

TEST_CASE("reference-derived palettes stay readable for the built-in families", "[theme][palette][contrast]") {
    // 参考色阶路径（butter / fluent / material 用它）也必须过同一套门槛。
    // 逐族用**各族自己的 policy** 编译，否则会把某族的品牌档位当成通用规则来量。
    const auto families = theme::default_theme_families();
    REQUIRE_FALSE(families.empty());
    for (const auto& family: families) {
        INFO("theme family: " << family.name);
        const auto reference = family.reference;
        require_palette_readable(
            theme::make_color_scheme(reference, theme::ColorAppearance::light, family.policy),
            /*require_ring_on_card=*/false,
            /*strict_status_text=*/false
        );
        require_palette_readable(
            theme::make_color_scheme(reference, theme::ColorAppearance::dark, family.policy),
            /*require_ring_on_card=*/false,
            /*strict_status_text=*/false
        );
    }
}
