// Pure layout values must remain usable without scene or widget headers.

#include <nandina/foundation/layout_constraints.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <type_traits>

using nandina::foundation::NanInsets;
using nandina::foundation::NanLayoutConstraints;
using nandina::foundation::NanSize;

static_assert(std::is_aggregate_v<NanLayoutConstraints>);

TEST_CASE(
    "loose constraints preserve natural content and floor negative extents",
    "[foundation][layout]"
) {
    const auto loose = NanLayoutConstraints::loose();
    REQUIRE(loose.min_width == 0.0F);
    REQUIRE(loose.min_height == 0.0F);
    REQUIRE(std::isinf(loose.max_width));
    REQUIRE_FALSE(std::signbit(loose.max_width));
    REQUIRE(std::isinf(loose.max_height));
    REQUIRE_FALSE(std::signbit(loose.max_height));

    const auto natural = loose.constrain(NanSize(96.0F, 24.0F));
    REQUIRE(natural.get_width() == Catch::Approx(96.0F));
    REQUIRE(natural.get_height() == Catch::Approx(24.0F));
    const auto empty = loose.constrain(NanSize(-12.0F, -8.0F));
    REQUIRE(empty.get_width() == 0.0F);
    REQUIRE(empty.get_height() == 0.0F);
}

TEST_CASE(
    "tight constraints reserve the allocated viewport rather than natural size",
    "[foundation][layout]"
) {
    const auto viewport = NanLayoutConstraints::tight(NanSize(160.0F, 44.0F));
    REQUIRE(viewport.min_width == Catch::Approx(160.0F));
    REQUIRE(viewport.max_width == Catch::Approx(160.0F));
    REQUIRE(viewport.min_height == Catch::Approx(44.0F));
    REQUIRE(viewport.max_height == Catch::Approx(44.0F));
    for (const auto content: std::array {NanSize(96.0F, 18.0F), NanSize(300.0F, 100.0F)}) {
        const auto measured = viewport.constrain(content);
        REQUIRE(measured.get_width() == Catch::Approx(160.0F));
        REQUIRE(measured.get_height() == Catch::Approx(44.0F));
    }
}

TEST_CASE(
    "bounded content grows to its minimum and caps an oversized image",
    "[foundation][layout]"
) {
    const NanLayoutConstraints
        bounds {.min_width = 48.0F, .max_width = 160.0F, .min_height = 24.0F, .max_height = 72.0F};
    const auto small_label = bounds.constrain(NanSize(12.0F, 20.0F));
    REQUIRE(small_label.get_width() == Catch::Approx(48.0F));
    REQUIRE(small_label.get_height() == Catch::Approx(24.0F));
    const auto large_image = bounds.constrain(NanSize(240.0F, 90.0F));
    REQUIRE(large_image.get_width() == Catch::Approx(160.0F));
    REQUIRE(large_image.get_height() == Catch::Approx(72.0F));
    const auto fitting_content = bounds.constrain(NanSize(96.0F, 40.0F));
    REQUIRE(fitting_content.get_width() == Catch::Approx(96.0F));
    REQUIRE(fitting_content.get_height() == Catch::Approx(40.0F));
}

TEST_CASE(
    "an unbounded width keeps long text while height stays constrained",
    "[foundation][layout]"
) {
    const NanLayoutConstraints row {.min_width = 24.0F, .min_height = 18.0F, .max_height = 40.0F};
    const auto short_line = row.constrain(NanSize(640.0F, 12.0F));
    REQUIRE(short_line.get_width() == Catch::Approx(640.0F));
    REQUIRE(short_line.get_height() == Catch::Approx(18.0F));
    const auto tall_line = row.constrain(NanSize(640.0F, 80.0F));
    REQUIRE(tall_line.get_width() == Catch::Approx(640.0F));
    REQUIRE(tall_line.get_height() == Catch::Approx(40.0F));
}

TEST_CASE(
    "nonfinite maxima retain the existing uncapped content behavior",
    "[foundation][layout]"
) {
    // This extraction keeps the old fallback for every nonfinite maximum.
    for (const float maximum:
         std::array {
             std::numeric_limits<float>::infinity(),
             -std::numeric_limits<float>::infinity(),
             std::numeric_limits<float>::quiet_NaN()
         })
    {
        const NanLayoutConstraints bounds {
            .min_width = 24.0F,
            .max_width = maximum,
            .min_height = 10.0F,
            .max_height = maximum
        };
        const auto content = bounds.constrain(NanSize(80.0F, 60.0F));
        REQUIRE(content.get_width() == Catch::Approx(80.0F));
        REQUIRE(content.get_height() == Catch::Approx(60.0F));
        const auto small = bounds.constrain(NanSize(8.0F, 4.0F));
        REQUIRE(small.get_width() == Catch::Approx(24.0F));
        REQUIRE(small.get_height() == Catch::Approx(10.0F));
    }
}

TEST_CASE(
    "conflicting bounds preserve the required minimum without rewriting fields",
    "[foundation][layout]"
) {
    const NanLayoutConstraints
        required {.min_width = 90.0F, .max_width = 60.0F, .min_height = 36.0F, .max_height = 12.0F};
    for (const auto content: std::array {NanSize(40.0F, 100.0F), NanSize(500.0F, 0.0F)}) {
        const auto measured = required.constrain(content);
        REQUIRE(measured.get_width() == Catch::Approx(90.0F));
        REQUIRE(measured.get_height() == Catch::Approx(36.0F));
    }
    REQUIRE(required.max_width == Catch::Approx(60.0F));
    REQUIRE(required.max_height == Catch::Approx(12.0F));
}

TEST_CASE("special measured sizes keep existing float behavior", "[foundation][layout]") {
    const NanLayoutConstraints
        bounded {.min_width = 24.0F, .max_width = 200.0F, .min_height = 10.0F, .max_height = 70.0F};
    const auto infinite = bounded.constrain(
        NanSize(std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity())
    );
    REQUIRE(infinite.get_width() == Catch::Approx(200.0F));
    REQUIRE(infinite.get_height() == Catch::Approx(10.0F));
    const auto unknown_width =
        bounded.constrain(NanSize(std::numeric_limits<float>::quiet_NaN(), 30.0F));
    REQUIRE(std::isnan(unknown_width.get_width()));
    REQUIRE(unknown_width.get_height() == Catch::Approx(30.0F));

    const auto negative_zero = NanLayoutConstraints::loose().constrain(NanSize(-0.0F, 5.0F));
    REQUIRE(negative_zero.get_width() == 0.0F);
    REQUIRE(std::signbit(negative_zero.get_width()));
    const auto signed_allocation = NanLayoutConstraints::tight(NanSize(-4.0F, 8.0F));
    REQUIRE(signed_allocation.min_width == Catch::Approx(-4.0F));
    REQUIRE(signed_allocation.max_width == Catch::Approx(-4.0F));
    REQUIRE(signed_allocation.constrain(NanSize(40.0F, 40.0F)).get_width() == Catch::Approx(-4.0F));
}

TEST_CASE(
    "asymmetric panel padding reserves the correct horizontal and vertical space",
    "[foundation][layout]"
) {
    const NanLayoutConstraints
        panel {.min_width = 64.0F, .max_width = 240.0F, .min_height = 40.0F, .max_height = 120.0F};
    const auto content = panel.deflated(NanInsets(7.0F, 13.0F, 5.0F, 11.0F));
    REQUIRE(content.min_width == Catch::Approx(44.0F));
    REQUIRE(content.max_width == Catch::Approx(220.0F));
    REQUIRE(content.min_height == Catch::Approx(24.0F));
    REQUIRE(content.max_height == Catch::Approx(104.0F));
    REQUIRE(panel.min_width == Catch::Approx(64.0F));
    REQUIRE(panel.max_height == Catch::Approx(120.0F));
}

TEST_CASE(
    "padding larger than its panel collapses available content to zero",
    "[foundation][layout]"
) {
    const NanLayoutConstraints
        panel {.min_width = 12.0F, .max_width = 20.0F, .min_height = 8.0F, .max_height = 10.0F};
    const auto content = panel.deflated(NanInsets(6.0F, 17.0F, 4.0F, 11.0F));
    REQUIRE(content.min_width == 0.0F);
    REQUIRE(content.max_width == 0.0F);
    REQUIRE(content.min_height == 0.0F);
    REQUIRE(content.max_height == 0.0F);
    const auto measured = content.constrain(NanSize(60.0F, 10.0F));
    REQUIRE(measured.get_width() == 0.0F);
    REQUIRE(measured.get_height() == 0.0F);
}

TEST_CASE(
    "deflation preserves nonfinite maximum markers and finite minima",
    "[foundation][layout]"
) {
    for (const float maximum:
         std::array {
             std::numeric_limits<float>::infinity(),
             -std::numeric_limits<float>::infinity(),
             std::numeric_limits<float>::quiet_NaN()
         })
    {
        const NanLayoutConstraints panel {
            .min_width = 40.0F,
            .max_width = maximum,
            .min_height = 20.0F,
            .max_height = maximum
        };
        const auto content = panel.deflated(NanInsets(4.0F, 6.0F, 3.0F, 5.0F));
        REQUIRE(content.min_width == Catch::Approx(30.0F));
        REQUIRE(content.min_height == Catch::Approx(12.0F));
        if (std::isnan(maximum)) {
            REQUIRE(std::isnan(content.max_width));
            REQUIRE(std::isnan(content.max_height));
        }
        else {
            REQUIRE(content.max_width == maximum);
            REQUIRE(content.max_height == maximum);
        }
    }
}
