#include <nandina/widget/internal/anchored_positioner.hpp>

#include <catch2/catch_test_macros.hpp>

#include <limits>

using namespace nandina;

TEST_CASE("anchored overlays support direction and cross-axis alignment", "[overlay][position]") {
    const auto anchor = foundation::NanRect::from_xywh(100.0F, 80.0F, 40.0F, 20.0F);
    const auto viewport = foundation::NanRect::from_xywh(0.0F, 0.0F, 400.0F, 300.0F);
    const foundation::NanSize overlay_size(60.0F, 30.0F);

    const auto top = widget::internal::position_anchored_overlay(
        anchor,
        overlay_size,
        viewport,
        {.placement = widget::internal::OverlayPlacement::top, .gap = 8.0F}
    );
    REQUIRE(top.rect == foundation::NanRect::from_xywh(90.0F, 42.0F, 60.0F, 30.0F));

    const auto right_end = widget::internal::position_anchored_overlay(
        anchor,
        overlay_size,
        viewport,
        {
            .placement = widget::internal::OverlayPlacement::right,
            .alignment = widget::internal::OverlayAlignment::end,
            .gap = 5.0F,
        }
    );
    REQUIRE(right_end.rect == foundation::NanRect::from_xywh(145.0F, 70.0F, 60.0F, 30.0F));
}

TEST_CASE("anchored overlays flip to the side with less overflow", "[overlay][position][flip]") {
    const auto result = widget::internal::position_anchored_overlay(
        foundation::NanRect::from_xywh(80.0F, 170.0F, 40.0F, 20.0F),
        foundation::NanSize(80.0F, 50.0F),
        foundation::NanRect::from_xywh(0.0F, 0.0F, 200.0F, 200.0F),
        {.gap = 6.0F}
    );

    REQUIRE(result.placement == widget::internal::OverlayPlacement::top);
    REQUIRE(result.flipped);
    REQUIRE_FALSE(result.shifted);
    REQUIRE(result.rect == foundation::NanRect::from_xywh(60.0F, 114.0F, 80.0F, 50.0F));
}

TEST_CASE("anchored overlays shift inside padded viewport bounds", "[overlay][position][shift]") {
    const auto result = widget::internal::position_anchored_overlay(
        foundation::NanRect::from_xywh(4.0F, 50.0F, 20.0F, 20.0F),
        foundation::NanSize(80.0F, 40.0F),
        foundation::NanRect::from_xywh(0.0F, 0.0F, 180.0F, 120.0F),
        {
            .alignment = widget::internal::OverlayAlignment::start,
            .offset = foundation::NanPoint(-20.0F, 0.0F),
            .viewport_padding = 8.0F,
            .flip = false,
        }
    );

    REQUIRE_FALSE(result.flipped);
    REQUIRE(result.shifted);
    REQUIRE(result.rect == foundation::NanRect::from_xywh(8.0F, 70.0F, 80.0F, 40.0F));
}

TEST_CASE("anchored positioning can preserve intentional overflow", "[overlay][position]") {
    const auto result = widget::internal::position_anchored_overlay(
        foundation::NanRect::from_xywh(5.0F, 5.0F, 10.0F, 10.0F),
        foundation::NanSize(100.0F, 80.0F),
        foundation::NanRect::from_xywh(0.0F, 0.0F, 60.0F, 50.0F),
        {.placement = widget::internal::OverlayPlacement::top, .flip = false, .shift = false}
    );

    REQUIRE(result.placement == widget::internal::OverlayPlacement::top);
    REQUIRE_FALSE(result.flipped);
    REQUIRE_FALSE(result.shifted);
    REQUIRE(result.rect == foundation::NanRect::from_xywh(-40.0F, -75.0F, 100.0F, 80.0F));
}

TEST_CASE("anchored positioning rejects invalid geometry", "[overlay][position][contract]") {
    const auto anchor = foundation::NanRect::from_xywh(10.0F, 10.0F, 20.0F, 20.0F);
    const auto viewport = foundation::NanRect::from_xywh(0.0F, 0.0F, 100.0F, 100.0F);

    REQUIRE_THROWS_AS(
        widget::internal::position_anchored_overlay(
            anchor,
            foundation::NanSize::zero(),
            viewport
        ),
        std::invalid_argument
    );
    REQUIRE_THROWS_AS(
        widget::internal::position_anchored_overlay(
            anchor,
            foundation::NanSize(20.0F, 20.0F),
            viewport,
            {.gap = std::numeric_limits<float>::infinity()}
        ),
        std::invalid_argument
    );
}
