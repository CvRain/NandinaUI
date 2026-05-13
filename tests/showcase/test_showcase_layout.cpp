#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include <thorvg-1/thorvg.h>

import nandina.app.authoring;
import nandina.runtime.nan_widget;
import nandina.showcase.overview_page;

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

TEST(ShowcaseLayoutTest, OverviewContent_HasPaddingRootAndThreeContentSections) {
    OverviewContent component;

    // OverviewContent 结构：root_padding → main_column(3子)
    ASSERT_EQ(component.child_count(), 1u);

    auto& root_padding = child_at(component, 0);
    ASSERT_EQ(root_padding.child_count(), 1u);

    auto& main_column = child_at(root_padding, 0);
    // hero_slot / split / bottom_slot
    ASSERT_EQ(main_column.child_count(), 3u);
}

TEST(ShowcaseLayoutTest, OverviewContent_DrawPassLaysOutContentRegions) {
    OverviewContent component;
    static_cast<nandina::runtime::NanWidget&>(component).set_bounds(0.0f, 0.0f, 1280.0f, 720.0f);

    ThorvgCanvasScope canvas_scope{1280u, 720u};
    component.draw(canvas_scope.canvas());

    // root_padding fills entire bounds
    auto& root_padding = child_at(component, 0);
    EXPECT_FLOAT_EQ(root_padding.bounds().x(), 0.0f);
    EXPECT_FLOAT_EQ(root_padding.bounds().y(), 0.0f);
    EXPECT_FLOAT_EQ(root_padding.bounds().width(), 1280.0f);
    EXPECT_FLOAT_EQ(root_padding.bounds().height(), 720.0f);

    // main_column inset by padding=20 on all sides
    auto& main_column = child_at(root_padding, 0);
    EXPECT_FLOAT_EQ(main_column.bounds().x(), 20.0f);
    EXPECT_FLOAT_EQ(main_column.bounds().y(), 20.0f);
    EXPECT_FLOAT_EQ(main_column.bounds().width(), 1240.0f);
    EXPECT_FLOAT_EQ(main_column.bounds().height(), 680.0f);

    // hero_slot: height=136, starts at column top
    auto& hero_slot   = child_at(main_column, 0);
    auto& split_row   = child_at(main_column, 1);
    auto& bottom_slot = child_at(main_column, 2);

    EXPECT_FLOAT_EQ(hero_slot.bounds().x(), 20.0f);
    EXPECT_FLOAT_EQ(hero_slot.bounds().y(), 20.0f);
    EXPECT_FLOAT_EQ(hero_slot.bounds().width(), 1240.0f);
    EXPECT_FLOAT_EQ(hero_slot.bounds().height(), 136.0f);

    // split_row: height=280, gap=20 after hero
    EXPECT_FLOAT_EQ(split_row.bounds().x(), 20.0f);
    EXPECT_FLOAT_EQ(split_row.bounds().y(), 176.0f); // 20+136+20
    EXPECT_FLOAT_EQ(split_row.bounds().width(), 1240.0f);
    EXPECT_FLOAT_EQ(split_row.bounds().height(), 280.0f);

    // bottom_slot: height=224, gap=20 after split
    EXPECT_FLOAT_EQ(bottom_slot.bounds().x(), 20.0f);
    EXPECT_FLOAT_EQ(bottom_slot.bounds().y(), 476.0f); // 176+280+20
    EXPECT_FLOAT_EQ(bottom_slot.bounds().width(), 1240.0f);
    EXPECT_FLOAT_EQ(bottom_slot.bounds().height(), 224.0f);
}

TEST(ShowcaseLayoutTest, OverviewContent_DrawPassProducesVisiblePixels) {
    OverviewContent component;
    static_cast<nandina::runtime::NanWidget&>(component).set_bounds(0.0f, 0.0f, 1280.0f, 720.0f);

    ThorvgCanvasScope canvas_scope{1280u, 720u};
    component.draw(canvas_scope.canvas());
    canvas_scope.render();

    // hero 区域内的像素（位于 padding=20 内的英雄卡片）
    const auto hero_pixel       = canvas_scope.pixel_at(320u, 60u);
    // split 区域（y≈200）
    const auto split_pixel      = canvas_scope.pixel_at(320u, 200u);
    // bottom 卡片区域（y≈520）
    const auto lower_card_pixel = canvas_scope.pixel_at(980u, 520u);

    EXPECT_GT(alpha_of(hero_pixel), 0u);
    EXPECT_GT(alpha_of(split_pixel), 0u);
    EXPECT_GT(alpha_of(lower_card_pixel), 0u);

    EXPECT_NE(hero_pixel, lower_card_pixel);
}

TEST(ShowcaseLayoutTest, AppWindowWrappedOverviewContentDrawsVisiblePixels) {
    TestShowcaseWindow window({
        .title = "Showcase Test",
        .width = 1280,
        .height = 720,
        .resizable = false,
        .high_dpi = false,
    });
    window.set_root(nandina::app::adopt(std::make_unique<OverviewContent>()));

    ThorvgCanvasScope canvas_scope{1280u, 720u};
    window.draw_once(canvas_scope.canvas());
    canvas_scope.render();

    const auto hero_pixel    = canvas_scope.pixel_at(320u, 60u);
    const auto content_pixel = canvas_scope.pixel_at(320u, 200u);

    EXPECT_GT(alpha_of(hero_pixel), 0u);
    EXPECT_GT(alpha_of(content_pixel), 0u);
    EXPECT_NE(hero_pixel, content_pixel);
}