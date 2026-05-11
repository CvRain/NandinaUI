#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include <thorvg-1/thorvg.h>

import nandina.app.authoring;
import nandina.runtime.nan_widget;
import nandina.showcase;

namespace {

auto child_at(nandina::runtime::NanWidget& widget, const std::size_t index) -> nandina::runtime::NanWidget& {
    auto& children = widget.children();
    EXPECT_LT(index, children.size());
    return *children.at(index);
}

class ThorvgCanvasScope {
public:
    ThorvgCanvasScope(const std::uint32_t width, const std::uint32_t height)
        : pixels_(width * height, 0u), width_(width) {
        if (tvg::Initializer::init(0u) != tvg::Result::Success) {
            throw std::runtime_error("tvg::Initializer::init failed");
        }
        canvas_.reset(tvg::SwCanvas::gen());
        if (!canvas_) {
            tvg::Initializer::term();
            throw std::runtime_error("tvg::SwCanvas::gen() returned nullptr");
        }
        if (canvas_->target(
                pixels_.data(),
                width,
                width,
                height,
                tvg::ColorSpace::ARGB8888) != tvg::Result::Success) {
            canvas_.reset();
            tvg::Initializer::term();
            throw std::runtime_error("tvg::SwCanvas::target failed");
        }
    }

    ~ThorvgCanvasScope() {
        canvas_.reset();
        tvg::Initializer::term();
    }

    auto canvas() const noexcept -> tvg::SwCanvas& {
        return *canvas_;
    }

    auto render() const -> void {
        ASSERT_EQ(canvas_->draw(true), tvg::Result::Success);
        ASSERT_EQ(canvas_->sync(), tvg::Result::Success);
    }

    [[nodiscard]] auto pixel_at(const std::uint32_t x, const std::uint32_t y) const noexcept -> std::uint32_t {
        return pixels_.at(static_cast<std::size_t>(y) * width_ + x);
    }

private:
    std::vector<std::uint32_t> pixels_;
    std::unique_ptr<tvg::SwCanvas> canvas_;
    std::uint32_t width_;
};

class TestShowcaseWindow final : public nandina::app::NanAppWindow {
public:
    explicit TestShowcaseWindow(const nandina::app::AppConfig& config)
        : nandina::app::NanAppWindow(config) {
    }

    auto draw_once(tvg::SwCanvas& canvas) -> void {
        on_draw(canvas);
    }
};

[[nodiscard]] auto alpha_of(const std::uint32_t pixel) noexcept -> std::uint8_t {
    return static_cast<std::uint8_t>((pixel >> 24u) & 0xffu);
}

} // namespace

TEST(ShowcaseLayoutTest, MainComponent_UsesSidebarAndCapabilityBoard) {
    MainComponent component;

    ASSERT_EQ(component.child_count(), 1u);

    auto& root_row = child_at(component, 0);
    ASSERT_EQ(root_row.child_count(), 2u);

    auto& sidebar_slot = child_at(root_row, 0);
    auto& main_expanded = child_at(root_row, 1);
    ASSERT_EQ(sidebar_slot.child_count(), 1u);
    ASSERT_EQ(main_expanded.child_count(), 1u);

    auto& main_padding = child_at(main_expanded, 0);
    ASSERT_EQ(main_padding.child_count(), 1u);

    auto& main_column = child_at(main_padding, 0);
    ASSERT_EQ(main_column.child_count(), 3u);
}

TEST(ShowcaseLayoutTest, MainComponent_DrawPassLaysOutPrimaryRegions) {
    MainComponent component;
    static_cast<nandina::runtime::NanWidget&>(component).set_bounds(0.0f, 0.0f, 1280.0f, 720.0f);

    ThorvgCanvasScope canvas_scope{1280u, 720u};
    component.draw(canvas_scope.canvas());

    auto& root_row = child_at(component, 0);
    auto& sidebar_slot = child_at(root_row, 0);
    auto& main_expanded = child_at(root_row, 1);
    auto& main_padding = child_at(main_expanded, 0);
    auto& main_column = child_at(main_padding, 0);

    const auto sidebar_bounds = sidebar_slot.bounds();
    EXPECT_FLOAT_EQ(sidebar_bounds.x(), 0.0f);
    EXPECT_FLOAT_EQ(sidebar_bounds.y(), 0.0f);
    EXPECT_FLOAT_EQ(sidebar_bounds.width(), 260.0f);
    EXPECT_FLOAT_EQ(sidebar_bounds.height(), 720.0f);

    const auto main_bounds = main_expanded.bounds();
    EXPECT_FLOAT_EQ(main_bounds.x(), 260.0f);
    EXPECT_FLOAT_EQ(main_bounds.y(), 0.0f);
    EXPECT_FLOAT_EQ(main_bounds.width(), 1020.0f);
    EXPECT_FLOAT_EQ(main_bounds.height(), 720.0f);

    const auto content_bounds = main_column.bounds();
    EXPECT_FLOAT_EQ(content_bounds.x(), 280.0f);
    EXPECT_FLOAT_EQ(content_bounds.y(), 20.0f);
    EXPECT_FLOAT_EQ(content_bounds.width(), 980.0f);
    EXPECT_FLOAT_EQ(content_bounds.height(), 680.0f);

    auto& hero_slot = child_at(main_column, 0);
    auto& split_row = child_at(main_column, 1);
    auto& bottom_slot = child_at(main_column, 2);

    EXPECT_FLOAT_EQ(hero_slot.bounds().x(), 280.0f);
    EXPECT_FLOAT_EQ(hero_slot.bounds().y(), 20.0f);
    EXPECT_FLOAT_EQ(hero_slot.bounds().width(), 980.0f);
    EXPECT_FLOAT_EQ(hero_slot.bounds().height(), 136.0f);

    EXPECT_FLOAT_EQ(split_row.bounds().x(), 280.0f);
    EXPECT_FLOAT_EQ(split_row.bounds().y(), 176.0f);
    EXPECT_FLOAT_EQ(split_row.bounds().width(), 980.0f);
    EXPECT_FLOAT_EQ(split_row.bounds().height(), 280.0f);

    EXPECT_FLOAT_EQ(bottom_slot.bounds().x(), 280.0f);
    EXPECT_FLOAT_EQ(bottom_slot.bounds().y(), 476.0f);
    EXPECT_FLOAT_EQ(bottom_slot.bounds().width(), 980.0f);
    EXPECT_FLOAT_EQ(bottom_slot.bounds().height(), 224.0f);
}

TEST(ShowcaseLayoutTest, MainComponent_DrawPassProducesDistinctRegions) {
    MainComponent component;
    static_cast<nandina::runtime::NanWidget&>(component).set_bounds(0.0f, 0.0f, 1280.0f, 720.0f);

    ThorvgCanvasScope canvas_scope{1280u, 720u};
    component.draw(canvas_scope.canvas());
    canvas_scope.render();

    const auto sidebar_pixel = canvas_scope.pixel_at(40u, 40u);
    const auto hero_pixel = canvas_scope.pixel_at(320u, 60u);
    const auto lower_card_pixel = canvas_scope.pixel_at(980u, 520u);

    EXPECT_GT(alpha_of(sidebar_pixel), 0u);
    EXPECT_GT(alpha_of(hero_pixel), 0u);
    EXPECT_GT(alpha_of(lower_card_pixel), 0u);

    EXPECT_NE(sidebar_pixel, hero_pixel);
    EXPECT_NE(hero_pixel, lower_card_pixel);
}

TEST(ShowcaseLayoutTest, AppWindowWrappedMainComponentStillDrawsShowcaseRegions) {
    TestShowcaseWindow window({
        .title = "Showcase Test",
        .width = 1280,
        .height = 720,
        .resizable = false,
        .high_dpi = false,
    });
    window.set_root(nandina::app::adopt(std::make_unique<MainComponent>()));

    ThorvgCanvasScope canvas_scope{1280u, 720u};
    window.draw_once(canvas_scope.canvas());
    canvas_scope.render();

    const auto sidebar_pixel = canvas_scope.pixel_at(40u, 40u);
    const auto hero_pixel = canvas_scope.pixel_at(320u, 60u);

    EXPECT_GT(alpha_of(sidebar_pixel), 0u);
    EXPECT_GT(alpha_of(hero_pixel), 0u);
    EXPECT_NE(sidebar_pixel, hero_pixel);
}