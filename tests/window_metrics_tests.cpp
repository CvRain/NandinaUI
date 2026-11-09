//
// Window and framebuffer metrics tests.
//

#include <nandina/app/window_metrics.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <stdexcept>

namespace
{
    using namespace nandina;

    void require_scale(const app::WindowMetrics& metrics, float expected) {
        REQUIRE(metrics.framebuffer_scale_x == Catch::Approx(expected));
        REQUIRE(metrics.framebuffer_scale_y == Catch::Approx(expected));
        REQUIRE(metrics.screen_to_physical == Catch::Approx(expected));
        REQUIRE(metrics.framebuffer_scale_is_uniform);
    }
} // namespace

TEST_CASE("window metrics preserve 1x and 2x coordinate spaces", "[app][window-metrics]") {
    const auto one_x = app::make_window_metrics(
        foundation::NanSize(800.0F, 600.0F),
        foundation::NanSize(800.0F, 600.0F)
    );
    const auto two_x = app::make_window_metrics(
        foundation::NanSize(800.0F, 600.0F),
        foundation::NanSize(1600.0F, 1200.0F)
    );

    require_scale(one_x, 1.0F);
    require_scale(two_x, 2.0F);
    REQUIRE(two_x.screen_size == foundation::NanSize(800.0F, 600.0F));
    REQUIRE(two_x.framebuffer_size == foundation::NanSize(1600.0F, 1200.0F));
}

TEST_CASE("window metrics recompute after screen and framebuffer resize", "[app][window-metrics]") {
    const auto before = app::make_window_metrics(
        foundation::NanSize(800.0F, 600.0F),
        foundation::NanSize(1600.0F, 1200.0F)
    );
    const auto after = app::make_window_metrics(
        foundation::NanSize(1000.0F, 700.0F),
        foundation::NanSize(2000.0F, 1400.0F)
    );

    require_scale(before, 2.0F);
    require_scale(after, 2.0F);
    REQUIRE(after.screen_size == foundation::NanSize(1000.0F, 700.0F));
    REQUIRE(after.framebuffer_size == foundation::NanSize(2000.0F, 1400.0F));
}

TEST_CASE("window metrics define non-uniform framebuffer fallback", "[app][window-metrics]") {
    const auto metrics = app::make_window_metrics(
        foundation::NanSize(800.0F, 600.0F),
        foundation::NanSize(1600.0F, 900.0F)
    );

    REQUIRE(metrics.framebuffer_scale_x == Catch::Approx(2.0F));
    REQUIRE(metrics.framebuffer_scale_y == Catch::Approx(1.5F));
    REQUIRE(metrics.screen_to_physical == Catch::Approx(1.5F));
    REQUIRE_FALSE(metrics.framebuffer_scale_is_uniform);
}

TEST_CASE("window metrics tolerate integer framebuffer rounding", "[app][window-metrics]") {
    const auto metrics = app::make_window_metrics(
        foundation::NanSize(801.0F, 601.0F),
        foundation::NanSize(1202.0F, 902.0F)
    );

    REQUIRE(metrics.framebuffer_scale_is_uniform);
    REQUIRE(metrics.screen_to_physical == Catch::Approx(1202.0F / 801.0F));
}

TEST_CASE("window metrics reject invalid coordinate spaces", "[app][window-metrics]") {
    REQUIRE_THROWS_AS(
        app::make_window_metrics(foundation::NanSize::zero(), foundation::NanSize(800.0F, 600.0F)),
        std::invalid_argument
    );
    REQUIRE_THROWS_AS(
        app::make_window_metrics(
            foundation::NanSize(800.0F, 600.0F),
            foundation::NanSize(std::numeric_limits<float>::infinity(), 600.0F)
        ),
        std::invalid_argument
    );
    REQUIRE_THROWS_AS(
        app::make_window_metrics(
            foundation::NanSize(std::numeric_limits<float>::denorm_min(), 600.0F),
            foundation::NanSize(std::numeric_limits<float>::max(), 600.0F)
        ),
        std::invalid_argument
    );
}

TEST_CASE("content scale separates OS scaling from pixel density", "[app][window-metrics]") {
    // Windows at 175%: window coordinates are physical pixels and the framebuffer equals the
    // window, so the content scale is the full display scale.
    REQUIRE(app::content_scale_from_display(1.75F, 1.0F) == Catch::Approx(1.75F));
    // Windows at 100%: no content scaling.
    REQUIRE(app::content_scale_from_display(1.0F, 1.0F) == Catch::Approx(1.0F));
    // Retina-like platform: window coordinates are points and pixel density carries the 2x; the
    // content scale is 1.0, so logical units are not scaled twice.
    REQUIRE(app::content_scale_from_display(2.0F, 2.0F) == Catch::Approx(1.0F));
    // Fractional Linux scale, window coordinates stay logical.
    REQUIRE(app::content_scale_from_display(1.5F, 1.5F) == Catch::Approx(1.0F));
    REQUIRE(app::content_scale_from_display(1.5F, 1.0F) == Catch::Approx(1.5F));
}

TEST_CASE("unavailable platform scale falls back to 1x", "[app][window-metrics]") {
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    const auto inf = std::numeric_limits<float>::infinity();
    REQUIRE(app::content_scale_from_display(nan, 1.0F) == Catch::Approx(1.0F));
    REQUIRE(app::content_scale_from_display(1.0F, nan) == Catch::Approx(1.0F));
    REQUIRE(app::content_scale_from_display(0.0F, 1.0F) == Catch::Approx(1.0F));
    REQUIRE(app::content_scale_from_display(inf, 1.0F) == Catch::Approx(1.0F));
    REQUIRE(app::content_scale_from_display(1.75F, -1.0F) == Catch::Approx(1.0F));
}

TEST_CASE("logical and native window sizes round-trip by content scale", "[app][window-metrics]") {
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    const auto native =
        app::window_size_from_logical_size(foundation::NanSize(1200.0F, 800.0F), 1.75F);
    REQUIRE(native == foundation::NanSize(2100.0F, 1400.0F));
    const auto logical = app::logical_size_from_window_size(native, 1.75F);
    REQUIRE(logical == foundation::NanSize(1200.0F, 800.0F));

    // A missing/invalid scale means "already logical" rather than a divide-by-zero.
    REQUIRE(
        app::window_size_from_logical_size(foundation::NanSize(800.0F, 600.0F), 0.0F)
        == foundation::NanSize(800.0F, 600.0F)
    );
    REQUIRE(
        app::logical_size_from_window_size(foundation::NanSize(800.0F, 600.0F), nan)
        == foundation::NanSize(800.0F, 600.0F)
    );
}

TEST_CASE("window size conversion rejects invalid sizes", "[app][window-metrics]") {
    REQUIRE_THROWS_AS(
        app::window_size_from_logical_size(foundation::NanSize::zero(), 1.75F),
        std::invalid_argument
    );
    REQUIRE_THROWS_AS(
        app::logical_size_from_window_size(
            foundation::NanSize(std::numeric_limits<float>::infinity(), 600.0F),
            1.75F
        ),
        std::invalid_argument
    );
}
