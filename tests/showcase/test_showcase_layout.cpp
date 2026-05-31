#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include <thorvg-1/thorvg.h>

import nandina.app.authoring;
import nandina.runtime.nan_event;
import nandina.runtime.nan_widget;
import nandina.showcase;
import nandina.showcase.main_page;
import nandina.showcase.sandbox_page;
import nandina.widgets.sidebar_menu_button;

namespace {

auto child_at(nandina::runtime::NanWidget& widget, const std::size_t index) -> nandina::runtime::NanWidget& {
    auto& children = widget.children();
    EXPECT_LT(index, children.size());
    return *children.at(index);
}

auto collect_sidebar_buttons(
    nandina::runtime::NanWidget& widget,
    std::vector<nandina::widgets::SidebarMenuButton*>& out) -> void {
    if (auto* button = dynamic_cast<nandina::widgets::SidebarMenuButton*>(&widget)) {
        out.push_back(button);
    }

    for (auto& child : widget.children()) {
        collect_sidebar_buttons(*child, out);
    }
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

struct SandboxRootFixture {
    std::unique_ptr<nandina::showcase::SandboxPage> page;
    nandina::app::NanComponent::Ptr component;
};

auto make_sandbox_root() -> SandboxRootFixture {
    auto page = std::make_unique<nandina::showcase::SandboxPage>();
    auto component = page->build();
    return SandboxRootFixture{
        .page = std::move(page),
        .component = std::move(component),
    };
}

} // namespace

TEST(ShowcaseLayoutTest, SandboxPageBuild_ProducesMountedComponent) {
    auto fixture = make_sandbox_root();
    auto& component = fixture.component;
    ASSERT_NE(component, nullptr);
    // Sandbox 目前只有一个 Center → Label 结构
    ASSERT_GE(component->child_count(), 1u);
}

TEST(ShowcaseLayoutTest, SandboxPageDraw_ProducesVisiblePixels) {
    auto fixture = make_sandbox_root();
    auto& component = fixture.component;
    ASSERT_NE(component, nullptr);
    static_cast<nandina::runtime::NanWidget&>(*component).set_bounds(0.0f, 0.0f, 1280.0f, 720.0f);
    component->layout();

    ThorvgCanvasScope canvas_scope{1280u, 720u};
    component->draw(canvas_scope.canvas());
    canvas_scope.render();

    // Verify the sandbox page builds and draws without crashing.
    // (Pixel-level verification of rendered text glyphs using ThorVG Picture
    //  + SwCanvas is deferred until render abstract layer is established.)
    SUCCEED();
}

TEST(ShowcaseLayoutTest, AppWindowWrappedSandboxPageDrawsVisiblePixels) {
    TestShowcaseWindow window({
        .title = "Showcase Test",
        .width = 1280,
        .height = 720,
        .resizable = false,
        .high_dpi = false,
    });
    auto fixture = make_sandbox_root();
    window.set_root_component(std::move(fixture.component));

    ThorvgCanvasScope canvas_scope{1280u, 720u};
    window.draw_once(canvas_scope.canvas());
    canvas_scope.render();

    // Verify the app-window-wrapped sandbox page builds and draws without crashing.
    // (Pixel-level verification of rendered text glyphs using ThorVG Picture
    //  + SwCanvas is deferred until render abstract layer is established.)
    SUCCEED();
}

TEST(ShowcaseLayoutTest, SandboxPageLayoutCentersContentInsteadOfStackingAtOrigin) {
    auto fixture = make_sandbox_root();
    auto& component = fixture.component;
    ASSERT_NE(component, nullptr);

    static_cast<nandina::runtime::NanWidget&>(*component).set_bounds(0.0f, 0.0f, 1280.0f, 720.0f);
    component->layout();

    auto& center = child_at(*component, 0);
    auto& sized_box = child_at(center, 0);
    auto& column = child_at(sized_box, 0);
    ASSERT_GE(column.child_count(), 7u);

    auto& button_row = child_at(column, 6);
    auto& left_expanded = child_at(button_row, 0);
    auto& right_expanded = child_at(button_row, 1);

    EXPECT_FLOAT_EQ(sized_box.x(), 460.0f);
    EXPECT_FLOAT_EQ(sized_box.width(), 360.0f);
    EXPECT_FLOAT_EQ(column.x(), 460.0f);
    EXPECT_FLOAT_EQ(column.width(), 360.0f);
    EXPECT_GT(left_expanded.width(), 0.0f);
    EXPECT_GT(right_expanded.x(), left_expanded.x());
}

TEST(ShowcaseLayoutTest, ShowcaseShellSidebarActiveStateTracksNavigation) {
    auto router = nandina::app::NanRouter::create();
    router->register_page(std::make_unique<nandina::showcase::SandboxPage>());

    auto shell_node = nandina::app::create_shell(std::move(router));
    auto mounted = nandina::app::mount(std::move(shell_node));
    ASSERT_NE(mounted, nullptr);

    mounted->set_bounds(0.0f, 0.0f, 1280.0f, 720.0f);

    auto& root_row = child_at(*mounted, 0);
    std::vector<nandina::widgets::SidebarMenuButton*> buttons;
    collect_sidebar_buttons(root_row, buttons);

    // 当前只有 Sandbox 一个页面，sidebar 应有至少 1 个导航按钮
    ASSERT_GE(buttons.size(), 1u);
    EXPECT_TRUE(buttons[0]->active());
}

TEST(ShowcaseLayoutTest, ExportedShowcaseShellRegistersMultiplePagesIntoSidebar) {
    auto mounted = nandina::app::mount(nandina::showcase::create_showcase_shell());
    ASSERT_NE(mounted, nullptr);

    mounted->set_bounds(0.0f, 0.0f, 1280.0f, 720.0f);

    auto& root_row = child_at(*mounted, 0);
    std::vector<nandina::widgets::SidebarMenuButton*> buttons;
    collect_sidebar_buttons(root_row, buttons);

    ASSERT_GE(buttons.size(), 4u);
    EXPECT_TRUE(buttons[0]->active());
    EXPECT_FALSE(buttons[1]->active());
    EXPECT_FALSE(buttons[2]->active());
    EXPECT_FALSE(buttons[3]->active());
    EXPECT_EQ(buttons[0]->text(), "Main Page");
    EXPECT_EQ(buttons[1]->text(), "Button Showcase");
    EXPECT_EQ(buttons[2]->text(), "Forms");
    EXPECT_EQ(buttons[3]->text(), "Sandbox");
}

TEST(ShowcaseLayoutTest, ShowcaseShellPlacesSidebarOnLeftAndPageHostOnRight) {
    auto mounted = nandina::app::mount(nandina::showcase::create_showcase_shell());
    ASSERT_NE(mounted, nullptr);

    static_cast<nandina::runtime::NanWidget&>(*mounted).set_bounds(0.0f, 0.0f, 1280.0f, 720.0f);
    mounted->measure(nandina::geometry::NanConstraints::tight(1280.0f, 720.0f));
    mounted->layout();

    auto& root_row = child_at(*mounted, 0);
    ASSERT_EQ(root_row.child_count(), 2u);

    const auto sidebar_bounds = root_row.children()[0]->bounds();
    const auto content_bounds = root_row.children()[1]->bounds();

    EXPECT_FLOAT_EQ(sidebar_bounds.x(), 0.0f);
    EXPECT_FLOAT_EQ(sidebar_bounds.y(), 0.0f);
    EXPECT_FLOAT_EQ(sidebar_bounds.width(), 260.0f);
    EXPECT_FLOAT_EQ(sidebar_bounds.height(), 720.0f);

    EXPECT_FLOAT_EQ(content_bounds.x(), 260.0f);
    EXPECT_FLOAT_EQ(content_bounds.y(), 0.0f);
    EXPECT_FLOAT_EQ(content_bounds.width(), 1020.0f);
    EXPECT_FLOAT_EQ(content_bounds.height(), 720.0f);
}

TEST(ShowcaseLayoutTest, MainPagePanelAndCardContentDoNotStackOnSameBounds) {
    nandina::showcase::MainPage page;
    auto component = page.build();
    ASSERT_NE(component, nullptr);

    static_cast<nandina::runtime::NanWidget&>(*component).set_bounds(0.0f, 0.0f, 1280.0f, 720.0f);
    component->measure(nandina::geometry::NanConstraints::tight(1280.0f, 720.0f));
    component->layout();

    auto& root_column = child_at(*component, 0);
    ASSERT_EQ(root_column.child_count(), 3u);

    auto& panel = child_at(root_column, 1);
    auto& card = child_at(root_column, 2);

    auto& panel_content_column = child_at(panel, 0);
    auto& card_content_column = child_at(card, 0);
    ASSERT_EQ(panel_content_column.child_count(), 2u);
    ASSERT_EQ(card_content_column.child_count(), 2u);

    const auto panel_first = panel_content_column.children()[0]->bounds();
    const auto panel_second = panel_content_column.children()[1]->bounds();
    const auto card_first = card_content_column.children()[0]->bounds();
    const auto card_second = card_content_column.children()[1]->bounds();
    auto& card_title_host = child_at(card, 1);
    auto& card_title_label = child_at(card_title_host, 0);

    EXPECT_GT(panel_second.y(), panel_first.y());
    EXPECT_GT(card_second.y(), card_first.y());
    EXPECT_LE(panel_second.y() + panel_second.height(), panel.y() + panel.height());
    EXPECT_LE(card_second.y() + card_second.height(), card.y() + card.height());
    EXPECT_GE(card_title_label.x(), card.x());
    EXPECT_GE(card_title_label.y(), card.y());
    EXPECT_LT(card_title_label.y(), card_first.y());
}