//
// Fixed-design viewport scaling policy tests.
//

#include "app/viewport_scaling.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <stdexcept>

namespace
{
    using namespace nandina;

    void require_point(foundation::NanPoint actual, float x, float y) {
        REQUIRE(actual.get_x() == Catch::Approx(x));
        REQUIRE(actual.get_y() == Catch::Approx(y));
    }
} // namespace

TEST_CASE("contain preserves the design aspect ratio and anchors letterboxing", "[app][viewport]") {
    const auto mapping = app::make_viewport_mapping(
        foundation::NanSize(800.0F, 600.0F),
        app::ViewportScalePolicy {.design_size = foundation::NanSize(400.0F, 400.0F)}
    );

    REQUIRE(mapping.scale == Catch::Approx(1.5F));
    require_point(mapping.offset, 100.0F, 0.0F);
    REQUIRE(mapping.logical_size == foundation::NanSize(400.0F, 400.0F));
    REQUIRE(mapping.content_bounds() == foundation::NanRect::from_xywh(100.0F, 0.0F, 600.0F, 600.0F));
}

TEST_CASE("cover fills the screen and exposes the cropped logical region", "[app][viewport]") {
    const auto mapping = app::make_viewport_mapping(
        foundation::NanSize(800.0F, 600.0F),
        app::ViewportScalePolicy {
            .design_size = foundation::NanSize(400.0F, 400.0F),
            .fit = app::ViewportFit::cover,
        }
    );

    REQUIRE(mapping.scale == Catch::Approx(2.0F));
    require_point(mapping.offset, 0.0F, -100.0F);
    REQUIRE(mapping.content_bounds() == foundation::NanRect::from_xywh(0.0F, -100.0F, 800.0F, 800.0F));
}

TEST_CASE("viewport mapping round-trips input through the render transform", "[app][viewport][input]") {
    const auto mapping = app::make_viewport_mapping(
        foundation::NanSize(900.0F, 600.0F),
        app::ViewportScalePolicy {
            .design_size = foundation::NanSize(400.0F, 300.0F),
            .horizontal_anchor = app::ViewportAnchor::end,
            .vertical_anchor = app::ViewportAnchor::start,
        }
    );
    const foundation::NanPoint logical(40.0F, 75.0F);
    const auto screen = mapping.logical_to_screen(logical);

    REQUIRE(mapping.scale == Catch::Approx(2.0F));
    require_point(mapping.offset, 100.0F, 0.0F);
    require_point(screen, 180.0F, 150.0F);
    require_point(mapping.screen_to_logical(screen), 40.0F, 75.0F);
}

TEST_CASE("viewport mapping rejects invalid coordinate spaces", "[app][viewport]") {
    REQUIRE_THROWS_AS(
        app::make_viewport_mapping(
            foundation::NanSize::zero(),
            app::ViewportScalePolicy {.design_size = foundation::NanSize(400.0F, 300.0F)}
        ),
        std::invalid_argument
    );
    REQUIRE_THROWS_AS(
        app::make_viewport_mapping(
            foundation::NanSize(800.0F, 600.0F),
            app::ViewportScalePolicy {
                .design_size = foundation::NanSize(
                    std::numeric_limits<float>::infinity(), 300.0F
                ),
            }
        ),
        std::invalid_argument
    );
}
