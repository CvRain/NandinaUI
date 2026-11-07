// Foundation unique CPU compositor: hand-computed expectations and algebraic properties.
#include <nandina/foundation/composition.hpp>
#include <nandina/foundation/contrast.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace foundation = nandina::foundation;

namespace
{
    auto color(const float red, const float green, const float blue, const float alpha = 1.0F)
        -> foundation::NanColor {
        return foundation::NanColor::from_rgb(red, green, blue, alpha);
    }

    auto channel(const foundation::NanColor& value) -> foundation::NanRgb {
        return value.to<foundation::NanRgb>();
    }
} // namespace

TEST_CASE("single source-over blends encoded sRGB channels", "[foundation][composition]") {
    // Hand-computed: 50% grey with alpha 0.5 over white stays in encoded space.
    const auto blended =
        foundation::nan_composite_over(color(0.5F, 0.5F, 0.5F, 0.5F), color(1, 1, 1));
    CHECK(channel(blended).red == Catch::Approx(0.75F));
    CHECK(channel(blended).green == Catch::Approx(0.75F));
    CHECK(channel(blended).blue == Catch::Approx(0.75F));
    CHECK(channel(blended).alpha == Catch::Approx(1.0F));

    // Alpha endpoints are exact identities.
    const auto opaque_backdrop = color(0.2F, 0.4F, 0.6F);
    const auto invisible = foundation::nan_composite_over(color(1, 0, 0, 0.0F), opaque_backdrop);
    CHECK(channel(invisible).red == Catch::Approx(0.2F));
    CHECK(channel(invisible).green == Catch::Approx(0.4F));
    CHECK(channel(invisible).blue == Catch::Approx(0.6F));
    const auto solid =
        foundation::nan_composite_over(color(0.3F, 0.5F, 0.7F, 1.0F), color(1, 1, 1));
    CHECK(channel(solid).red == Catch::Approx(0.3F));
    CHECK(channel(solid).green == Catch::Approx(0.5F));
    CHECK(channel(solid).blue == Catch::Approx(0.7F));

    // Out-of-range alpha is clamped instead of leaking invalid geometry.
    // NanColor conversion round-trips through OKLCH, so allow float noise at the endpoint.
    const auto negative = foundation::nan_composite_over(color(1, 0, 0, -0.5F), color(0, 0, 1));
    CHECK(channel(negative).red == Catch::Approx(0.0F).margin(0.001F));
    CHECK(channel(negative).blue == Catch::Approx(1.0F).margin(0.001F));
}

TEST_CASE(
    "compositor rejects non-finite colors and non-opaque backdrops",
    "[foundation][composition]"
) {
    const auto solid = color(0.5F, 0.5F, 0.5F);
    const auto translucent = color(0.5F, 0.5F, 0.5F, 0.5F);
    CHECK_THROWS_AS(
        foundation::nan_composite_over(color(0, 0, 0, 0.5F), translucent),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        foundation::nan_composite_over(color(std::numeric_limits<float>::quiet_NaN(), 0, 0), solid),
        std::invalid_argument
    );
}

TEST_CASE("background stack composes from the first opaque endpoint", "[foundation][composition]") {
    const std::array<foundation::NanColor, 3> layers {
        color(1, 0, 0, 0.5F),
        color(0, 0, 1, 0.5F),
        color(1, 1, 1),
    };
    auto result = foundation::nan_composite_background_stack(layers);
    REQUIRE(result);
    // Hand-computed: (0.5, 0.5, 1.0) after the second layer, then red at 0.5 over it.
    CHECK(channel(*result).red == Catch::Approx(0.75F));
    CHECK(channel(*result).green == Catch::Approx(0.25F));
    CHECK(channel(*result).blue == Catch::Approx(0.5F));
    CHECK(channel(*result).alpha == Catch::Approx(1.0F));

    // Layers beyond the opaque endpoint are never inspected (early termination).
    // NaN survives the color conversions, so an implementation that validates every
    // layer fails this case while the early-termination contract passes it.
    const std::array<foundation::NanColor, 3> behind_opaque {
        color(0, 0, 0, 0.5F),
        color(1, 1, 1),
        color(std::numeric_limits<float>::quiet_NaN(), 0, 0),
    };
    auto early = foundation::nan_composite_background_stack(behind_opaque);
    REQUIRE(early);
    CHECK(channel(*early).red == Catch::Approx(0.5F));
}

TEST_CASE(
    "background stack reports insufficient context instead of guessing",
    "[foundation][composition]"
) {
    const std::array<foundation::NanColor, 2> transparent {
        color(0, 0, 0, 0.25F),
        color(1, 1, 1, 0.5F),
    };
    auto no_endpoint = foundation::nan_composite_background_stack(transparent);
    REQUIRE_FALSE(no_endpoint);
    CHECK(no_endpoint.error().kind == foundation::CompositionErrorKind::no_opaque_endpoint);
    CHECK_FALSE(foundation::describe(no_endpoint.error()).empty());

    const std::array<foundation::NanColor, 2> non_finite {
        color(std::numeric_limits<float>::quiet_NaN(), 0, 0, 0.5F),
        color(1, 1, 1),
    };
    auto invalid = foundation::nan_composite_background_stack(non_finite);
    REQUIRE_FALSE(invalid);
    CHECK(invalid.error().kind == foundation::CompositionErrorKind::non_finite_color);
    CHECK(invalid.error().index == 0);
}

TEST_CASE(
    "foreground over stack equals composing both steps by hand",
    "[foundation][composition]"
) {
    const std::array<foundation::NanColor, 2> background {
        color(0, 0, 1, 0.5F),
        color(1, 1, 1),
    };
    auto total = foundation::nan_composite_over_stack(color(1, 0, 0, 0.5F), background);
    REQUIRE(total);
    const auto stack = foundation::nan_composite_background_stack(background);
    REQUIRE(stack);
    const auto manual = foundation::nan_composite_over(color(1, 0, 0, 0.5F), *stack);
    CHECK(channel(*total).red == Catch::Approx(channel(manual).red));
    CHECK(channel(*total).green == Catch::Approx(channel(manual).green));
    CHECK(channel(*total).blue == Catch::Approx(channel(manual).blue));

    // A transparent stack yields the composition error instead of a fabricated color.
    const std::array<foundation::NanColor, 1> fog {color(0, 0, 0, 0.1F)};
    CHECK_FALSE(foundation::nan_composite_over_stack(color(1, 1, 1), fog));
}

TEST_CASE(
    "contrast ratio ignores alpha so callers must compose first",
    "[foundation][composition]"
) {
    const auto translucent = color(0, 0, 0, 0.5F);
    const auto white = color(1, 1, 1);
    const auto composed = foundation::nan_composite_over(translucent, white);
    CHECK(foundation::nan_contrast_ratio(translucent, white) > 1.0F);
    CHECK(
        foundation::nan_contrast_ratio(composed, white)
        < foundation::nan_contrast_ratio(translucent, white)
    );
}

TEST_CASE(
    "composition validates strategy and precise background boundaries",
    "[foundation][composition]"
) {
    const std::array<foundation::NanColor, 1> opaque {color(1, 1, 1)};
    const foundation::CompositeConfig unknown {
        .space = static_cast<foundation::CompositeSpace>(17)
    };
    CHECK_THROWS_AS(
        foundation::nan_composite_over(color(0, 0, 0), opaque[0], unknown),
        std::invalid_argument
    );
    const auto unsupported = foundation::nan_composite_background_stack(opaque, unknown);
    REQUIRE_FALSE(unsupported);
    CHECK(unsupported.error().kind == foundation::CompositionErrorKind::unsupported_space);
    const auto foreground_unsupported =
        foundation::nan_composite_over_stack(color(0, 0, 0), opaque, unknown);
    REQUIRE_FALSE(foreground_unsupported);
    CHECK(
        foreground_unsupported.error().kind == foundation::CompositionErrorKind::unsupported_space
    );

    const auto empty = foundation::nan_composite_background_stack({});
    REQUIRE_FALSE(empty);
    CHECK(empty.error().kind == foundation::CompositionErrorKind::no_opaque_endpoint);
    CHECK(empty.error().index == 0);
    const std::array<foundation::NanColor, 1> almost {color(1, 1, 1, std::nextafter(1.0F, 0.0F))};
    const auto missing = foundation::nan_composite_background_stack(almost);
    REQUIRE_FALSE(missing);
    CHECK(missing.error().kind == foundation::CompositionErrorKind::no_opaque_endpoint);
    CHECK(missing.error().index == 1);
    CHECK_THROWS_AS(
        foundation::nan_composite_over(color(0, 0, 0), almost[0]),
        std::invalid_argument
    );

    const auto invalid_alpha =
        foundation::NanColor::from_oklch(0.5F, 0.0F, 0.0F, std::numeric_limits<float>::quiet_NaN());
    const auto invalid_foreground = foundation::nan_composite_over_stack(invalid_alpha, opaque);
    REQUIRE_FALSE(invalid_foreground);
    CHECK(invalid_foreground.error().kind == foundation::CompositionErrorKind::non_finite_color);
    const std::array<foundation::NanColor, 3> invalid_layer {
        color(0, 0, 0, 0.5F),
        invalid_alpha,
        color(1, 1, 1)
    };
    const auto invalid_background = foundation::nan_composite_background_stack(invalid_layer);
    REQUIRE_FALSE(invalid_background);
    CHECK(invalid_background.error().kind == foundation::CompositionErrorKind::non_finite_color);
    CHECK(invalid_background.error().index == 1);
}
