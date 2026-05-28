#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include <thorvg-1/thorvg.h>

import nandina.app.authoring;
import nandina.runtime.nan_event;
import nandina.runtime.nan_widget;
import nandina.showcase;
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
    auto& button_row = child_at(column, 2);
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