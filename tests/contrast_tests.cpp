//
// Foundation-layer WCAG contrast tests.
//

#include "foundation/contrast.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>

using namespace nandina;

namespace
{
    [[nodiscard]] auto rgb_color(const float red, const float green, const float blue)
        -> foundation::NanColor {
        return foundation::NanColor::from(foundation::NanRgb {red, green, blue, 1.0F});
    }

    [[nodiscard]] auto hex_color(
        const std::uint8_t red,
        const std::uint8_t green,
        const std::uint8_t blue
    ) -> foundation::NanColor {
        return foundation::NanColor::from(foundation::NanHexRgb {red, green, blue, 255});
    }
} // namespace

TEST_CASE("relative luminance covers the WCAG reference range", "[foundation][contrast]") {
    REQUIRE(
        foundation::nan_relative_luminance(rgb_color(1.0F, 1.0F, 1.0F))
        == Catch::Approx(1.0F).margin(0.001F)
    );
    REQUIRE(
        foundation::nan_relative_luminance(rgb_color(0.0F, 0.0F, 0.0F))
        == Catch::Approx(0.0F).margin(0.0001F)
    );
    // sRGB mid gray linearizes to ~0.2140 under the WCAG channel weights.
    REQUIRE(
        foundation::nan_relative_luminance(rgb_color(0.5F, 0.5F, 0.5F))
        == Catch::Approx(0.21404F).margin(0.002F)
    );
    // Pure red contributes only its channel weight, not a plain average.
    REQUIRE(
        foundation::nan_relative_luminance(rgb_color(1.0F, 0.0F, 0.0F))
        == Catch::Approx(0.2126F).margin(0.001F)
    );
}

TEST_CASE("contrast ratio is symmetric and bounded by 21", "[foundation][contrast]") {
    const auto black = rgb_color(0.0F, 0.0F, 0.0F);
    const auto white = rgb_color(1.0F, 1.0F, 1.0F);
    REQUIRE(foundation::nan_contrast_ratio(black, white) == Catch::Approx(21.0F).margin(0.01F));
    REQUIRE(foundation::nan_contrast_ratio(white, black) == Catch::Approx(21.0F).margin(0.01F));
    REQUIRE(foundation::nan_contrast_ratio(black, black) == Catch::Approx(1.0F).margin(0.001F));
}

TEST_CASE("contrast ratio matches WCAG reference gray values", "[foundation][contrast]") {
    // WCAG working examples: white vs #767676 = 4.54:1 (passes AA text),
    // white vs #777777 = 4.48:1 (fails AA text).
    const auto white = rgb_color(1.0F, 1.0F, 1.0F);
    const auto gray_76 = hex_color(0x76, 0x76, 0x76);
    const auto gray_77 = hex_color(0x77, 0x77, 0x77);
    REQUIRE(foundation::nan_contrast_ratio(white, gray_76) == Catch::Approx(4.54F).margin(0.02F));
    REQUIRE(foundation::nan_contrast_ratio(white, gray_76) >= foundation::nan_contrast_aa_text);
    REQUIRE(foundation::nan_contrast_ratio(white, gray_77) < foundation::nan_contrast_aa_text);
}

TEST_CASE("contrast helpers work with OKLCH-constructed colors", "[foundation][contrast]") {
    // NanColor stores OKLCH; the helpers must round-trip through sRGB.
    const auto oklch_black = foundation::NanColor {foundation::NanOklch {0.0F, 0.0F, 0.0F, 1.0F}};
    const auto oklch_white = foundation::NanColor {foundation::NanOklch {1.0F, 0.0F, 0.0F, 1.0F}};
    REQUIRE(
        foundation::nan_contrast_ratio(oklch_black, oklch_white)
        == Catch::Approx(21.0F).margin(0.01F)
    );
    // Luminance ignores alpha by contract.
    const auto transparent_white = oklch_white.with_alpha(0.0F);
    REQUIRE(
        foundation::nan_relative_luminance(transparent_white)
        == Catch::Approx(foundation::nan_relative_luminance(oklch_white))
    );
}
