#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include <thorvg-1/thorvg.h>

import nandina.app.authoring;
import nandina.foundation.nan_insets;
import nandina.runtime.nan_widget;
import nandina.layout.core;
import nandina.foundation.color;
import nandina.widgets.button;
import nandina.widgets.card;
import nandina.widgets.icon;
import nandina.widgets.label;
import nandina.widgets.panel;
import nandina.widgets.pressable;
import nandina.widgets.progressbar;
import nandina.widgets.surface;

class TestWidget final : public nandina::runtime::NanWidget {
public:
    static auto create() -> std::unique_ptr<TestWidget> {
        return std::make_unique<TestWidget>();
    }

    [[nodiscard]] auto preferred_size() const noexcept -> nandina::geometry::NanSize override {
        return {24.0f, 12.0f};
    }
};

namespace {

class ProbeWidget final : public nandina::runtime::NanWidget {
public:
    static auto create() -> std::unique_ptr<ProbeWidget> {
        return std::make_unique<ProbeWidget>();
    }

    auto measure(const nandina::geometry::NanConstraints& constraints) -> void override {
        ++measure_count;
        nandina::runtime::NanWidget::measure(constraints);
    }

    auto layout() -> void override {
        ++layout_count;
        nandina::runtime::NanWidget::layout();
    }

    [[nodiscard]] auto preferred_size() const noexcept -> nandina::geometry::NanSize override {
        return {24.0f, 12.0f};
    }

    int measure_count{0};
    int layout_count{0};
};

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

private:
    std::vector<std::uint32_t> pixels_;
    std::unique_ptr<tvg::SwCanvas> canvas_;
    std::uint32_t width_;
};

class TestAppWindow final : public nandina::app::NanAppWindow {
public:
    explicit TestAppWindow(const nandina::app::AppConfig& config)
        : nandina::app::NanAppWindow(config) {
    }

    auto draw_once(tvg::SwCanvas& canvas) -> void {
        on_draw(canvas);
    }
};

} // namespace

TEST(AppAuthoringTest, MountBindsAndResetsRootRef) {
    nandina::app::Ref<TestWidget> widget_ref;

    auto root = nandina::app::adopt(TestWidget::create()).bind(widget_ref);
    auto mounted = nandina::app::mount(std::move(root));

    ASSERT_NE(mounted, nullptr);
    ASSERT_TRUE(widget_ref);
    EXPECT_EQ(widget_ref.get(), mounted->children().front().get());

    mounted.reset();
    EXPECT_FALSE(widget_ref);
}

TEST(AppAuthoringTest, MountPreservesNestedRefsAcrossRowComposition) {
    nandina::app::Ref<TestWidget> left_ref;
    nandina::app::Ref<TestWidget> right_ref;

    auto root = nandina::app::row(nandina::app::children(
        nandina::app::adopt(TestWidget::create()).bind(left_ref),
        nandina::app::adopt(TestWidget::create()).bind(right_ref)));

    auto mounted = nandina::app::mount(std::move(root));

    ASSERT_NE(mounted, nullptr);
    ASSERT_EQ(mounted->child_count(), 1u);

    auto* row = mounted->children().front().get();
    ASSERT_NE(row, nullptr);
    ASSERT_EQ(row->child_count(), 2u);

    EXPECT_EQ(left_ref.get(), row->children()[0].get());
    EXPECT_EQ(right_ref.get(), row->children()[1].get());

    mounted.reset();
    EXPECT_FALSE(left_ref);
    EXPECT_FALSE(right_ref);
}

TEST(AppAuthoringTest, MountedNodeComponentPropagatesBoundsToRootWidget) {
    auto mounted = nandina::app::mount(nandina::app::adopt(TestWidget::create()));

    ASSERT_NE(mounted, nullptr);
    mounted->set_bounds(10.0f, 20.0f, 200.0f, 80.0f);

    ASSERT_EQ(mounted->child_count(), 1u);
    const auto bounds = mounted->children().front()->bounds();

    EXPECT_FLOAT_EQ(bounds.x(), 10.0f);
    EXPECT_FLOAT_EQ(bounds.y(), 20.0f);
    EXPECT_FLOAT_EQ(bounds.width(), 200.0f);
    EXPECT_FLOAT_EQ(bounds.height(), 80.0f);
}

TEST(AppAuthoringTest, MountedNodeComponentSetBoundsTriggersRootMeasureAndLayout) {
    nandina::app::Ref<ProbeWidget> probe_ref;

    auto mounted = nandina::app::mount(
        nandina::app::adopt(ProbeWidget::create()).bind(probe_ref));

    ASSERT_NE(mounted, nullptr);
    ASSERT_TRUE(probe_ref);

    mounted->set_bounds(0.0f, 0.0f, 120.0f, 40.0f);

    EXPECT_GE(probe_ref->measure_count, 1);
    EXPECT_GE(probe_ref->layout_count, 1);
}

TEST(AppAuthoringTest, PaddingWrapsSingleChildAndPreservesRefBinding) {
    nandina::app::Ref<TestWidget> widget_ref;

    auto mounted = nandina::app::mount(
        nandina::app::padding(
            nandina::app::adopt(TestWidget::create()).bind(widget_ref)));

    ASSERT_NE(mounted, nullptr);
    ASSERT_EQ(mounted->child_count(), 1u);

    auto* padding = mounted->children().front().get();
    ASSERT_NE(padding, nullptr);
    ASSERT_EQ(padding->child_count(), 1u);
    EXPECT_EQ(widget_ref.get(), padding->children().front().get());

    mounted.reset();
    EXPECT_FALSE(widget_ref);
}

TEST(AppAuthoringTest, CenterAndSizedBoxPropagateNestedBounds) {
    auto mounted = nandina::app::mount(
        nandina::app::center(
            nandina::app::sized_box(
                nandina::app::adopt(TestWidget::create()))));

    ASSERT_NE(mounted, nullptr);
    mounted->set_bounds(0.0f, 0.0f, 100.0f, 60.0f);

    ASSERT_EQ(mounted->child_count(), 1u);
    auto* center = mounted->children().front().get();
    ASSERT_NE(center, nullptr);
    ASSERT_EQ(center->child_count(), 1u);

    auto* sized_box = center->children().front().get();
    ASSERT_NE(sized_box, nullptr);
    ASSERT_EQ(sized_box->child_count(), 1u);

    const auto sized_box_bounds = sized_box->bounds();
    EXPECT_FLOAT_EQ(sized_box_bounds.x(), 38.0f);
    EXPECT_FLOAT_EQ(sized_box_bounds.y(), 24.0f);
    EXPECT_FLOAT_EQ(sized_box_bounds.width(), 24.0f);
    EXPECT_FLOAT_EQ(sized_box_bounds.height(), 12.0f);

    const auto child_bounds = sized_box->children().front()->bounds();
    EXPECT_FLOAT_EQ(child_bounds.x(), 38.0f);
    EXPECT_FLOAT_EQ(child_bounds.y(), 24.0f);
    EXPECT_FLOAT_EQ(child_bounds.width(), 24.0f);
    EXPECT_FLOAT_EQ(child_bounds.height(), 12.0f);
}

TEST(AppAuthoringTest, StackMountsMultipleChildrenAndPropagatesRefs) {
    nandina::app::Ref<TestWidget> front_ref;
    nandina::app::Ref<TestWidget> back_ref;

    auto mounted = nandina::app::mount(nandina::app::stack(nandina::app::children(
        nandina::app::adopt(TestWidget::create()).bind(back_ref),
        nandina::app::adopt(TestWidget::create()).bind(front_ref))));

    ASSERT_NE(mounted, nullptr);
    ASSERT_EQ(mounted->child_count(), 1u);

    auto* stack = mounted->children().front().get();
    ASSERT_NE(stack, nullptr);
    ASSERT_EQ(stack->child_count(), 2u);
    EXPECT_EQ(back_ref.get(), stack->children()[0].get());
    EXPECT_EQ(front_ref.get(), stack->children()[1].get());

    mounted->set_bounds(5.0f, 8.0f, 90.0f, 40.0f);
    const auto first_bounds = stack->children()[0]->bounds();
    const auto second_bounds = stack->children()[1]->bounds();

    EXPECT_FLOAT_EQ(first_bounds.x(), 5.0f);
    EXPECT_FLOAT_EQ(first_bounds.y(), 8.0f);
    EXPECT_FLOAT_EQ(first_bounds.width(), 90.0f);
    EXPECT_FLOAT_EQ(first_bounds.height(), 40.0f);
    EXPECT_FLOAT_EQ(second_bounds.x(), 5.0f);
    EXPECT_FLOAT_EQ(second_bounds.y(), 8.0f);
    EXPECT_FLOAT_EQ(second_bounds.width(), 90.0f);
    EXPECT_FLOAT_EQ(second_bounds.height(), 40.0f);

    mounted.reset();
    EXPECT_FALSE(front_ref);
    EXPECT_FALSE(back_ref);

}

TEST(AppAuthoringTest, PaddingNodeSupportsChainedInsetsConfiguration) {
    auto mounted = nandina::app::mount(
        nandina::app::padding(
            nandina::app::adopt(TestWidget::create()))
            .padding(3.0f, 5.0f, 7.0f, 11.0f));

    ASSERT_NE(mounted, nullptr);
    mounted->set_bounds(10.0f, 20.0f, 100.0f, 60.0f);

    ASSERT_EQ(mounted->child_count(), 1u);
    auto* padding = mounted->children().front().get();
    ASSERT_NE(padding, nullptr);
    ASSERT_EQ(padding->child_count(), 1u);

    const auto child_bounds = padding->children().front()->bounds();
    EXPECT_FLOAT_EQ(child_bounds.x(), 13.0f);
    EXPECT_FLOAT_EQ(child_bounds.y(), 25.0f);
    EXPECT_FLOAT_EQ(child_bounds.width(), 90.0f);
    EXPECT_FLOAT_EQ(child_bounds.height(), 44.0f);
}

TEST(AppAuthoringTest, SizedBoxNodeSupportsChainedFixedDimensions) {
    auto mounted = nandina::app::mount(
        nandina::app::center(
            nandina::app::sized_box(
                nandina::app::adopt(TestWidget::create()))
                .width(48.0f)
                .height(18.0f)));

    ASSERT_NE(mounted, nullptr);
    mounted->set_bounds(0.0f, 0.0f, 100.0f, 60.0f);

    ASSERT_EQ(mounted->child_count(), 1u);
    auto* center = mounted->children().front().get();
    ASSERT_NE(center, nullptr);
    ASSERT_EQ(center->child_count(), 1u);

    auto* sized_box = center->children().front().get();
    ASSERT_NE(sized_box, nullptr);
    ASSERT_EQ(sized_box->child_count(), 1u);

    const auto sized_box_bounds = sized_box->bounds();
    EXPECT_FLOAT_EQ(sized_box_bounds.x(), 26.0f);
    EXPECT_FLOAT_EQ(sized_box_bounds.y(), 21.0f);
    EXPECT_FLOAT_EQ(sized_box_bounds.width(), 48.0f);
    EXPECT_FLOAT_EQ(sized_box_bounds.height(), 18.0f);

    const auto child_bounds = sized_box->children().front()->bounds();
    EXPECT_FLOAT_EQ(child_bounds.x(), 26.0f);
    EXPECT_FLOAT_EQ(child_bounds.y(), 21.0f);
    EXPECT_FLOAT_EQ(child_bounds.width(), 48.0f);
    EXPECT_FLOAT_EQ(child_bounds.height(), 18.0f);
}

TEST(AppAuthoringTest, ColumnNodeSupportsChainedLayoutContainerConfiguration) {
    auto mounted = nandina::app::mount(
        nandina::app::column(nandina::app::children(
            nandina::app::adopt(TestWidget::create()),
            nandina::app::adopt(TestWidget::create())))
            .padding(4.0f, 6.0f, 8.0f, 10.0f)
            .gap(5.0f)
            .align_items(nandina::layout::LayoutAlignment::stretch));

    ASSERT_NE(mounted, nullptr);
    mounted->set_bounds(0.0f, 0.0f, 80.0f, 60.0f);

    ASSERT_EQ(mounted->child_count(), 1u);
    auto* column = mounted->children().front().get();
    ASSERT_NE(column, nullptr);
    ASSERT_EQ(column->child_count(), 2u);

    const auto first_bounds = column->children()[0]->bounds();
    const auto second_bounds = column->children()[1]->bounds();

    EXPECT_FLOAT_EQ(first_bounds.x(), 4.0f);
    EXPECT_FLOAT_EQ(first_bounds.y(), 6.0f);
    EXPECT_FLOAT_EQ(first_bounds.width(), 68.0f);
    EXPECT_FLOAT_EQ(first_bounds.height(), 12.0f);

    EXPECT_FLOAT_EQ(second_bounds.x(), 4.0f);
    EXPECT_FLOAT_EQ(second_bounds.y(), 23.0f);
    EXPECT_FLOAT_EQ(second_bounds.width(), 68.0f);
    EXPECT_FLOAT_EQ(second_bounds.height(), 12.0f);
}

TEST(AppAuthoringTest, StackNodeSupportsChainedAlignmentConfiguration) {
    auto mounted = nandina::app::mount(
        nandina::app::stack(nandina::app::children(
            nandina::app::adopt(TestWidget::create()),
            nandina::app::adopt(TestWidget::create())))
            .align_items(nandina::layout::LayoutAlignment::center)
            .justify_content(nandina::layout::LayoutAlignment::center));

    ASSERT_NE(mounted, nullptr);
    mounted->set_bounds(0.0f, 0.0f, 100.0f, 60.0f);

    ASSERT_EQ(mounted->child_count(), 1u);
    auto* stack = mounted->children().front().get();
    ASSERT_NE(stack, nullptr);
    ASSERT_EQ(stack->child_count(), 2u);

    const auto first_bounds = stack->children()[0]->bounds();
    const auto second_bounds = stack->children()[1]->bounds();

    EXPECT_FLOAT_EQ(first_bounds.x(), 38.0f);
    EXPECT_FLOAT_EQ(first_bounds.y(), 24.0f);
    EXPECT_FLOAT_EQ(first_bounds.width(), 24.0f);
    EXPECT_FLOAT_EQ(first_bounds.height(), 12.0f);

    EXPECT_FLOAT_EQ(second_bounds.x(), 38.0f);
    EXPECT_FLOAT_EQ(second_bounds.y(), 24.0f);
    EXPECT_FLOAT_EQ(second_bounds.width(), 24.0f);
    EXPECT_FLOAT_EQ(second_bounds.height(), 12.0f);
}

TEST(AppAuthoringTest, LabelFactorySupportsChainedTextStylingAndRefBinding) {
    nandina::app::Ref<nandina::widgets::Label> label_ref;

    auto mounted = nandina::app::mount(
        nandina::app::label("Overview")
            .font_size(10.0f)
            .align(nandina::widgets::TextAlign::Center)
            .vertical_align(nandina::widgets::TextVerticalAlign::Center)
            .color(nandina::NanColor::from(nandina::NanRgb{1, 2, 3}))
            .bind(label_ref));

    ASSERT_NE(mounted, nullptr);
    ASSERT_TRUE(label_ref);
    EXPECT_EQ(label_ref->text(), "Overview");
    EXPECT_FLOAT_EQ(label_ref->font_size(), 10.0f);
    const auto color = label_ref->color().to<nandina::NanRgb>();
    EXPECT_EQ(color.red(), 1u);
    EXPECT_EQ(color.green(), 2u);
    EXPECT_EQ(color.blue(), 3u);
}

TEST(AppAuthoringTest, ButtonFactorySupportsTextColorsAndRefBinding) {
    nandina::app::Ref<nandina::widgets::Button> button_ref;

    nandina::widgets::ButtonColors colors;
    colors.bg = nandina::NanColor::from(nandina::NanRgb{9, 8, 7});
    colors.text = nandina::NanColor::from(nandina::NanRgb{6, 5, 4});
    colors.corner_radius = 11.0f;
    colors.padding = nandina::geometry::NanInsets{20.0f, 9.0f, 18.0f, 7.0f};

    auto mounted = nandina::app::mount(
        nandina::app::button("Run")
            .colors(colors)
            .bind(button_ref));

    ASSERT_NE(mounted, nullptr);
    ASSERT_TRUE(button_ref);
    EXPECT_EQ(button_ref->text(), "Run");
    const auto bg = button_ref->colors().bg.to<nandina::NanRgb>();
    const auto text = button_ref->colors().text.to<nandina::NanRgb>();
    EXPECT_EQ(bg.red(), 9u);
    EXPECT_EQ(bg.green(), 8u);
    EXPECT_EQ(bg.blue(), 7u);
    EXPECT_EQ(text.red(), 6u);
    EXPECT_EQ(text.green(), 5u);
    EXPECT_EQ(text.blue(), 4u);
    EXPECT_FLOAT_EQ(button_ref->corner_radius(), 11.0f);
    EXPECT_FLOAT_EQ(button_ref->padding().left(), 20.0f);
    EXPECT_FLOAT_EQ(button_ref->padding().top(), 9.0f);
    EXPECT_FLOAT_EQ(button_ref->padding().right(), 18.0f);
    EXPECT_FLOAT_EQ(button_ref->padding().bottom(), 7.0f);
}

TEST(AppAuthoringTest, ButtonSetColorsAppliesLayoutAffectingStyle) {
    auto button = nandina::widgets::Button::create();
    button->set_bounds(0.0f, 0.0f, 120.0f, 36.0f);
    EXPECT_FALSE(button->is_layout_dirty());

    nandina::widgets::ButtonColors colors;
    colors.corner_radius = 10.0f;
    colors.padding = nandina::geometry::NanInsets{16.0f, 6.0f, 14.0f, 8.0f};

    button->set_colors(colors);

    EXPECT_TRUE(button->is_layout_dirty());
    EXPECT_FLOAT_EQ(button->corner_radius(), 10.0f);
    EXPECT_FLOAT_EQ(button->padding().left(), 16.0f);
    EXPECT_FLOAT_EQ(button->padding().top(), 6.0f);
    EXPECT_FLOAT_EQ(button->padding().right(), 14.0f);
    EXPECT_FLOAT_EQ(button->padding().bottom(), 8.0f);
}

TEST(AppAuthoringTest, CardFactorySupportsTitleStylingAndChildRefs) {
    nandina::app::Ref<nandina::widgets::Card> card_ref;
    nandina::app::Ref<TestWidget> child_ref;

    auto mounted = nandina::app::mount(
        nandina::app::card(nandina::app::children(
            nandina::app::adopt(TestWidget::create()).bind(child_ref)))
            .title("Stats")
            .bg_color(nandina::NanColor::from(nandina::NanRgb{50, 60, 70}))
            .corner_radius(12.0f)
            .bind(card_ref));

    ASSERT_NE(mounted, nullptr);
    ASSERT_TRUE(card_ref);
    ASSERT_TRUE(child_ref);
    EXPECT_EQ(card_ref->title(), "Stats");
    EXPECT_EQ(child_ref.get(), card_ref->children().front().get());
    EXPECT_FLOAT_EQ(card_ref->corner_radius(), 12.0f);
    const auto bg = card_ref->bg_color().to<nandina::NanRgb>();
    EXPECT_EQ(bg.red(), 50u);
    EXPECT_EQ(bg.green(), 60u);
    EXPECT_EQ(bg.blue(), 70u);
}

TEST(AppAuthoringTest, PanelFactorySupportsTitleStylingAndChildRefs) {
    nandina::app::Ref<nandina::widgets::Panel> panel_ref;
    nandina::app::Ref<TestWidget> child_ref;

    auto mounted = nandina::app::mount(
        nandina::app::panel(nandina::app::children(
            nandina::app::adopt(TestWidget::create()).bind(child_ref)))
            .title("Settings")
            .bg_color(nandina::NanColor::from(nandina::NanRgb{42, 44, 62}))
            .corner_radius(9.0f)
            .bind(panel_ref));

    ASSERT_NE(mounted, nullptr);
    ASSERT_TRUE(panel_ref);
    ASSERT_TRUE(child_ref);
    EXPECT_EQ(panel_ref->title(), "Settings");
    EXPECT_EQ(child_ref.get(), panel_ref->children().front().get());
    EXPECT_FLOAT_EQ(panel_ref->corner_radius(), 9.0f);
    const auto bg = panel_ref->bg_color().to<nandina::NanRgb>();
    EXPECT_EQ(bg.red(), 42u);
    EXPECT_EQ(bg.green(), 44u);
    EXPECT_EQ(bg.blue(), 62u);
}

TEST(AppAuthoringTest, CardLayoutAffectingSettersMarkLayoutDirty) {
    auto card = nandina::widgets::Card::create();

    card->set_bounds(0.0f, 0.0f, 160.0f, 96.0f);
    EXPECT_FALSE(card->is_layout_dirty());

    card->set_title("Stats");
    EXPECT_TRUE(card->is_layout_dirty());

    card->set_bounds(0.0f, 0.0f, 160.0f, 96.0f);
    EXPECT_FALSE(card->is_layout_dirty());

    card->set_title_font_size(14.0f);
    EXPECT_TRUE(card->is_layout_dirty());

    card->set_bounds(0.0f, 0.0f, 160.0f, 96.0f);
    EXPECT_FALSE(card->is_layout_dirty());

    card->set_show_accent(true);
    EXPECT_TRUE(card->is_layout_dirty());
}

TEST(AppAuthoringTest, PanelHeaderHeightMarksLayoutDirty) {
    auto panel = nandina::widgets::Panel::create();

    panel->set_bounds(0.0f, 0.0f, 160.0f, 96.0f);
    EXPECT_FALSE(panel->is_layout_dirty());

    panel->set_header_height(36.0f);
    EXPECT_TRUE(panel->is_layout_dirty());
}

TEST(AppAuthoringTest, LabelLayoutAffectingSettersMarkLayoutDirty) {
    auto label = nandina::widgets::Label::create();

    label->set_bounds(0.0f, 0.0f, 120.0f, 32.0f);
    EXPECT_FALSE(label->is_layout_dirty());

    label->set_text("Hello");
    EXPECT_TRUE(label->is_layout_dirty());

    label->set_bounds(0.0f, 0.0f, 120.0f, 32.0f);
    EXPECT_FALSE(label->is_layout_dirty());

    label->set_font_size(18.0f);
    EXPECT_TRUE(label->is_layout_dirty());
}

TEST(AppAuthoringTest, LabelLazyFontLoadRequestsRelayoutForNextFrame) {
    auto label = nandina::widgets::Label::create();
    label->set_text("Hello, Nandina");
    label->set_bounds(0.0f, 0.0f, 160.0f, 32.0f);
    EXPECT_FALSE(label->is_layout_dirty());

    ThorvgCanvasScope canvas_scope{160u, 32u};
    label->draw(canvas_scope.canvas());

    EXPECT_TRUE(label->font() != nullptr);
    EXPECT_TRUE(label->is_layout_dirty());
    EXPECT_TRUE(label->dirty());

    label->clear_dirty_recursive();
    EXPECT_TRUE(label->dirty());
}

TEST(AppAuthoringTest, LabelMeasureUsesConstraintWidthForWrappedHeight) {
    auto label = nandina::widgets::Label::create();
    label->set_text("Nandina UI layout wrapping verifies measured height");
    label->set_font_size(9.0f);

    label->measure(nandina::geometry::NanConstraints{0.0f, 240.0f, 0.0f, nandina::geometry::NanConstraints::k_infinity});
    const auto wide_size = label->measured_size();

    label->measure(nandina::geometry::NanConstraints{0.0f, 72.0f, 0.0f, nandina::geometry::NanConstraints::k_infinity});
    const auto narrow_size = label->measured_size();

    EXPECT_GT(wide_size.width(), 0.0f);
    EXPECT_GT(wide_size.height(), 0.0f);
    EXPECT_GT(narrow_size.height(), wide_size.height());
}

TEST(AppAuthoringTest, SurfacePaddingMarksLayoutDirty) {
    auto surface = nandina::widgets::Surface::create();

    surface->set_bounds(0.0f, 0.0f, 160.0f, 96.0f);
    EXPECT_FALSE(surface->is_layout_dirty());

    surface->set_padding(nandina::geometry::NanInsets{12.0f});
    EXPECT_TRUE(surface->is_layout_dirty());
}

TEST(AppAuthoringTest, SurfaceBackgroundColorMarksDirty) {
    auto surface = nandina::widgets::Surface::create();
    surface->clear_dirty();

    surface->set_bg_color(nandina::NanColor::from(nandina::NanRgb{10, 20, 30}));

    EXPECT_TRUE(surface->dirty());
}

TEST(AppAuthoringTest, IconAndProgressBarSizeSettersMarkLayoutDirty) {
    auto icon = nandina::widgets::Icon::create();
    icon->set_bounds(0.0f, 0.0f, 24.0f, 24.0f);
    EXPECT_FALSE(icon->is_layout_dirty());

    icon->set_size(20.0f);
    EXPECT_TRUE(icon->is_layout_dirty());

    auto progress = nandina::widgets::ProgressBar::create();
    progress->set_bounds(0.0f, 0.0f, 160.0f, 12.0f);
    EXPECT_FALSE(progress->is_layout_dirty());

    progress->set_bar_height(8.0f);
    EXPECT_TRUE(progress->is_layout_dirty());
}

TEST(AppAuthoringTest, ButtonTextSetterMarksLayoutDirty) {
    auto button = nandina::widgets::Button::create();

    button->set_bounds(0.0f, 0.0f, 120.0f, 36.0f);
    EXPECT_FALSE(button->is_layout_dirty());

    button->set_text("Run");
    EXPECT_TRUE(button->is_layout_dirty());
}

TEST(AppAuthoringTest, PressableDisableClearsInteractiveStateAndMarksDirty) {
    auto pressable = nandina::widgets::Pressable::create();
    pressable->clear_dirty();

    nandina::runtime::Event move_event = nandina::runtime::PointerMoveEvent{.x = 8.0, .y = 6.0, .delta_x = 0.0, .delta_y = 0.0};
    EXPECT_TRUE(pressable->dispatch_event(move_event));
    EXPECT_TRUE(pressable->state().hovered);

    pressable->clear_dirty();
    pressable->set_disabled(true);

    const auto state = pressable->state();
    EXPECT_TRUE(state.disabled);
    EXPECT_FALSE(state.hovered);
    EXPECT_FALSE(state.pressed);
    EXPECT_FALSE(state.focused);
    EXPECT_TRUE(pressable->dirty());
}

TEST(AppAuthoringTest, AppWindowDrawConsumesRootLayoutDirtyAndReflowsTree) {
    nandina::app::Ref<nandina::runtime::NanWidget> root_ref;
    nandina::app::Ref<TestWidget> first_ref;
    nandina::app::Ref<TestWidget> second_ref;

    TestAppWindow window({
        .title = "Test",
        .width = 80,
        .height = 60,
        .resizable = false,
        .high_dpi = false,
    });

    window.set_root(
        nandina::app::column(nandina::app::children(
            nandina::app::adopt(TestWidget::create()).bind(first_ref),
            nandina::app::adopt(TestWidget::create()).bind(second_ref)))
            .padding(4.0f, 6.0f, 8.0f, 10.0f)
            .gap(5.0f)
            .align_items(nandina::layout::LayoutAlignment::stretch)
            .bind(root_ref));

    ASSERT_TRUE(root_ref);
    ASSERT_TRUE(first_ref);
    ASSERT_TRUE(second_ref);

    first_ref->set_bounds(55.0f, 44.0f, 1.0f, 1.0f);
    second_ref->set_bounds(66.0f, 33.0f, 2.0f, 2.0f);
    root_ref->mark_layout_dirty();

    ThorvgCanvasScope canvas_scope{80u, 60u};
    window.draw_once(canvas_scope.canvas());

    EXPECT_FALSE(root_ref->dirty());
    EXPECT_FALSE(first_ref->dirty());
    EXPECT_FALSE(second_ref->dirty());

    const auto first_bounds = first_ref->bounds();
    const auto second_bounds = second_ref->bounds();

    EXPECT_FLOAT_EQ(first_bounds.x(), 4.0f);
    EXPECT_FLOAT_EQ(first_bounds.y(), 6.0f);
    EXPECT_FLOAT_EQ(first_bounds.width(), 68.0f);
    EXPECT_FLOAT_EQ(first_bounds.height(), 12.0f);

    EXPECT_FLOAT_EQ(second_bounds.x(), 4.0f);
    EXPECT_FLOAT_EQ(second_bounds.y(), 23.0f);
    EXPECT_FLOAT_EQ(second_bounds.width(), 68.0f);
    EXPECT_FLOAT_EQ(second_bounds.height(), 12.0f);
}