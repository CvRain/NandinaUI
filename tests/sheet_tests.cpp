#include "theme_test_support.hpp"
//
// Sheet (edge-docked modal drawer) tests.
//

#include <nandina/widget/sheet.hpp>

#include <nandina/foundation/geometry.hpp>
#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/render/render_device.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/overlay_host.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/semantics/semantics.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/builtin_component_traits.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/layout.hpp>
#include <nandina/widget/select.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace nandina;

namespace
{
    class RecordingDevice final: public render::IRenderDevice {
    public:
        struct BoxCall {
            foundation::NanRect rect;
            float radius = 0.0F;
            foundation::NanColor color;
        };
        std::vector<BoxCall> rects;

        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void
        draw_rect(const foundation::NanRect& rect, const foundation::NanColor& color) override {
            rects.push_back({rect, 0.0F, color});
        }
        void
        draw_rect_outline(const foundation::NanRect&, float, const foundation::NanColor&) override {
        }
        void draw_rounded_rect(
            const foundation::NanRect& rect,
            const float radius,
            const foundation::NanColor& color
        ) override {
            rects.push_back({rect, radius, color});
        }
        void draw_line(
            const foundation::NanPoint&,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_circle(const foundation::NanPoint&, float, const foundation::NanColor&) override {
        }
        void draw_text(
            std::string_view,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
    };

    /// 无窗口（detached）回退宿主：Sheet → DismissLayer → FocusScope → SheetPanel。
    struct DetachedSheetHarness {
        reactive::Graph graph;
        theme_test::Manager themes;
        std::shared_ptr<widget::Sheet> sheet = widget::Sheet::create();
        std::shared_ptr<scene::NanControl> root =
            std::make_shared<scene::NanControl>(foundation::NanSize(800.0F, 600.0F));
        scene::NanSceneTree tree;

        DetachedSheetHarness() {
            tree.set_theme_manager(themes);
            root->add_child(sheet);
            tree.set_root(root);
        }

        void layout() {
            (void)tree.layout_root(foundation::NanSize(800.0F, 600.0F));
        }

        [[nodiscard]] auto panel() const -> scene::NanControl* {
            if (sheet->child_count() == 0) {
                return nullptr;
            }
            auto* dismiss = sheet->get_child(0)->as_control();
            if (dismiss == nullptr || dismiss->child_count() == 0) {
                return nullptr;
            }
            auto* scope = dismiss->get_child(0)->as_control();
            return scope != nullptr && scope->child_count() > 0 ? scope->get_child(0)->as_control()
                                                                : nullptr;
        }
    };

    void tick(DetachedSheetHarness& harness, const float dt) {
        harness.sheet->on_process(dt);
        {
            auto phase = harness.tree.enter_phase(scene::FramePhase::animation);
            harness.tree.advance_animations(dt);
        }
    }

    /// 带窗口浮层的 Sheet 宿主：内容层可选挂在 120×80 裁剪容器内。
    struct OverlaySheetHarness {
        reactive::Graph graph;
        theme_test::Manager themes;
        std::shared_ptr<scene::OverlayHost> host = scene::OverlayHost::create();
        std::shared_ptr<scene::NanControl> body =
            std::make_shared<scene::NanControl>(foundation::NanSize(800.0F, 600.0F));
        std::shared_ptr<widget::Sheet> sheet = widget::Sheet::create();
        std::shared_ptr<scene::NanControl> clip;
        scene::NanSceneTree tree;

        explicit OverlaySheetHarness(const bool inside_clip = false) {
            tree.set_theme_manager(themes);
            host->set_content(body);
            tree.set_root(host);
            body->add_child(std::make_shared<scene::NanControl>(foundation::NanSize(1.0F, 1.0F)));
            if (inside_clip) {
                clip = std::make_shared<scene::NanControl>(foundation::NanSize(120.0F, 80.0F));
                clip->set_position(foundation::NanPoint(10.0F, 10.0F));
                clip->set_overflow(scene::ControlOverflow::clip);
                clip->add_child(sheet);
                body->add_child(clip);
            }
            else {
                body->add_child(sheet);
            }
        }

        void layout() {
            (void)tree.layout_root(foundation::NanSize(800.0F, 600.0F));
        }

        /// overlay layer → surface → DismissLayer → FocusScope → SheetPanel
        [[nodiscard]] auto panel() const -> scene::NanControl* {
            auto* surface = host->layer_at(1)->layout_root();
            if (surface == nullptr || surface->child_count() == 0) {
                return nullptr;
            }
            auto* dismiss = surface->get_child(0)->as_control();
            if (dismiss == nullptr || dismiss->child_count() == 0) {
                return nullptr;
            }
            auto* scope = dismiss->get_child(0)->as_control();
            return scope != nullptr && scope->child_count() > 0 ? scope->get_child(0)->as_control()
                                                                : nullptr;
        }
    };

    void settle(OverlaySheetHarness& harness) {
        for (int i = 0; i < 8; ++i) {
            harness.sheet->on_process(0.1F);
            {
                auto phase = harness.tree.enter_phase(scene::FramePhase::animation);
                harness.tree.advance_animations(0.1F);
            }
        }
    }
} // namespace

TEST_CASE(
    "Sheet reuses DialogRecipe with flush zero radius by default and honors overrides",
    "[sheet][theme]"
) {
    DetachedSheetHarness harness;
    const auto light = harness.sheet->resolved_style();
    CHECK(light.scrim.alpha() > 0.0F);
    CHECK(light.scrim.alpha() < 1.0F);
    CHECK(light.panel.radius == Catch::Approx(0.0F));
    CHECK(light.metrics.panel_width == Catch::Approx(360.0F));
    CHECK(harness.sheet->extent() == Catch::Approx(360.0F));
    CHECK_FALSE(harness.sheet->has_explicit_extent());

    harness.themes.set_preference(theme::ThemePreference::dark);
    const auto dark = harness.sheet->resolved_style();
    CHECK_FALSE(light.panel.fill == dark.panel.fill);
    CHECK(dark.panel.radius == Catch::Approx(0.0F));

    harness.sheet->set_override(
        theme::DialogRecipeRule {
            .panel_radius = theme::ThemeScalar::literal(12.0F),
            .metrics_panel_width = theme::ThemeScalar::literal(420.0F),
        }
    );
    CHECK(harness.sheet->resolved_style().panel.radius == Catch::Approx(12.0F));
    CHECK(harness.sheet->extent() == Catch::Approx(420.0F));

    // set_extent 优先于 recipe 的 metrics_panel_width，clear_extent 恢复到 recipe/override。
    harness.sheet->set_extent(480.0F);
    CHECK(harness.sheet->has_explicit_extent());
    CHECK(harness.sheet->extent() == Catch::Approx(480.0F));
    harness.sheet->clear_extent();
    CHECK_FALSE(harness.sheet->has_explicit_extent());
    CHECK(harness.sheet->extent() == Catch::Approx(420.0F));
}

TEST_CASE(
    "Sheet docks to all four viewport edges and respects extent",
    "[sheet][overlay][layout]"
) {
    OverlaySheetHarness harness;
    harness.sheet->set_title("抽屉面板");
    harness.sheet->set_description("四边停靠几何校验");
    (void)harness.sheet->set_content(
        std::make_shared<scene::NanControl>(foundation::NanSize(100.0F, 60.0F))
    );
    harness.sheet->open();
    harness.layout();

    auto* panel = harness.panel();
    REQUIRE(panel != nullptr);

    // 1. 默认 right：贴齐右边缘，铺满视口高度 600，宽度 360。
    CHECK(harness.sheet->side() == widget::SheetSide::right);
    CHECK(panel->global_bounds() == foundation::NanRect::from_xywh(440.0F, 0.0F, 360.0F, 600.0F));

    // 2. left：贴齐左边缘 (0, 0)，铺满视口高度 600，显式宽度 300。
    harness.sheet->set_side(widget::SheetSide::left);
    harness.sheet->set_extent(300.0F);
    harness.layout();
    CHECK(panel->global_bounds() == foundation::NanRect::from_xywh(0.0F, 0.0F, 300.0F, 600.0F));

    // 3. top：贴齐上边缘 (0, 0)，铺满视口宽度 800，显式高度 220。
    harness.sheet->set_side(widget::SheetSide::top);
    harness.sheet->set_extent(220.0F);
    harness.layout();
    CHECK(panel->global_bounds() == foundation::NanRect::from_xywh(0.0F, 0.0F, 800.0F, 220.0F));

    // 4. bottom：贴齐下边缘 (0, 600 - 240 = 360)，铺满视口宽度 800，显式高度 240。
    harness.sheet->set_side(widget::SheetSide::bottom);
    harness.sheet->set_extent(240.0F);
    harness.layout();
    CHECK(panel->global_bounds() == foundation::NanRect::from_xywh(0.0F, 360.0F, 800.0F, 240.0F));

    // 5. bottom 清除显式 extent 后回退到自然高度（>= min_height 140），仍贴齐底边。
    harness.sheet->clear_extent();
    harness.layout();
    const auto natural_bounds = panel->global_bounds();
    CHECK(natural_bounds.get_left() == Catch::Approx(0.0F));
    CHECK(natural_bounds.get_width() == Catch::Approx(800.0F));
    CHECK(natural_bounds.get_height() >= 140.0F);
    CHECK(natural_bounds.get_bottom() == Catch::Approx(600.0F));
}

TEST_CASE(
    "Sheet pins footer to the bottom and gives content the remaining middle height",
    "[sheet][layout][slots]"
) {
    OverlaySheetHarness harness;
    harness.sheet->set_title("属性详情");
    harness.sheet->set_description("头部说明文字");

    auto body = std::make_shared<scene::NanControl>();
    body->set_width(scene::fill);
    body->set_height(scene::fill);
    auto footer = std::make_shared<scene::NanControl>(foundation::NanSize(120.0F, 36.0F));

    (void)harness.sheet->set_content(body);
    (void)harness.sheet->set_footer(footer);
    harness.sheet->open();
    harness.layout();

    auto* panel = harness.panel();
    REQUIRE(panel != nullptr);
    const auto style = harness.sheet->resolved_style();
    const auto panel_bounds = panel->global_bounds();
    const auto body_bounds = body->global_bounds();
    const auto footer_bounds = footer->global_bounds();

    // footer 贴齐面板底部内边距。
    CHECK(
        panel_bounds.get_bottom() - footer_bounds.get_bottom()
        == Catch::Approx(style.metrics.padding_y)
    );
    // content (height=fill) 占据 header 下方到 footer 上方的全部中间高度。
    CHECK(footer_bounds.get_top() - body_bounds.get_bottom() == Catch::Approx(style.metrics.gap));
    CHECK(body_bounds.get_width() == Catch::Approx(360.0F - style.metrics.padding_x * 2.0F));
    CHECK(body_bounds.get_height() > 400.0F);

    // 自定义 header 与 set_title/set_description 互斥，后设置者生效。
    auto custom_header = std::make_shared<scene::NanControl>(foundation::NanSize(160.0F, 48.0F));
    (void)harness.sheet->set_header(custom_header);
    CHECK(harness.sheet->header() == custom_header.get());
    CHECK(harness.sheet->content() == body.get());
    CHECK(harness.sheet->footer() == footer.get());
    harness.layout();
    CHECK(
        custom_header->global_bounds().get_top() - panel_bounds.get_top()
        == Catch::Approx(style.metrics.padding_y)
    );

    harness.sheet->set_title("恢复内置标题");
    CHECK(harness.sheet->header() != custom_header.get());
    CHECK(custom_header->parent() == nullptr);
}

TEST_CASE("Sheet escapes clipping containers and blocks input below", "[sheet][overlay][modal]") {
    OverlaySheetHarness harness(true);
    auto behind = std::make_shared<widget::Button>("Behind");
    behind->set_position(foundation::NanPoint(20.0F, 20.0F));
    harness.body->add_child(behind);
    harness.layout();

    const auto behind_point = foundation::NanPoint(30.0F, 30.0F);
    REQUIRE(harness.tree.hit_test(behind_point) == behind.get());

    harness.sheet->open();
    harness.layout();

    auto* panel = harness.panel();
    REQUIRE(panel != nullptr);
    const auto bounds = panel->global_bounds();
    // 面板按 800×600 视口停靠在右侧 (440, 0, 360, 600)，不被 120×80 的父级裁剪容器限制。
    CHECK(bounds == foundation::NanRect::from_xywh(440.0F, 0.0F, 360.0F, 600.0F));
    CHECK_FALSE(harness.clip->global_bounds().contains_point(bounds.get_center()));

    auto* hit = harness.tree.hit_test(behind_point);
    REQUIRE(hit != nullptr);
    CHECK(hit != behind.get());
}

TEST_CASE(
    "Sheet dismisses on scrim click and escape and honors non-dismissible mode",
    "[sheet][overlay][input]"
) {
    OverlaySheetHarness harness;
    (void)harness.sheet->set_content(std::make_shared<widget::Button>("Inside"));
    harness.sheet->open();
    harness.layout();

    // 面板在右侧 [440, 800]：点击面板内部 (500, 300) 不关闭。
    harness.tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(500.0F, 300.0F)
        )
    );
    REQUIRE(harness.sheet->is_open());

    // 点击左侧遮罩区域 (50, 300) 关闭。
    harness.tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(50.0F, 300.0F)
        )
    );
    REQUIRE_FALSE(harness.sheet->is_open());

    // 重新打开后按 Escape 关闭。
    harness.sheet->open();
    harness.layout();
    REQUIRE(harness.sheet->is_open());
    harness.tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::escape, scene::KeyEvent::Action::press)
    );
    REQUIRE_FALSE(harness.sheet->is_open());

    // dismissible = false 时吞掉遮罩点击与 Escape，不关闭且不穿透到下层。
    int behind_clicks = 0;
    auto behind = std::make_shared<widget::Button>("Behind");
    behind->set_position(foundation::NanPoint(20.0F, 20.0F));
    behind->set_on_click([&behind_clicks] { ++behind_clicks; });
    harness.body->add_child(behind);

    harness.sheet->set_dismissible(false);
    harness.sheet->open();
    harness.layout();

    harness.tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::escape, scene::KeyEvent::Action::press)
    );
    CHECK(harness.sheet->is_open());

    harness.tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(25.0F, 25.0F)
        )
    );
    CHECK(harness.sheet->is_open());
    CHECK(behind_clicks == 0);
}

TEST_CASE(
    "Sheet traps Tab focus within its panel and restores focus on close",
    "[sheet][overlay][focus]"
) {
    OverlaySheetHarness harness;
    auto opener = std::make_shared<widget::Button>("Open Sheet");
    harness.body->add_child(opener);
    harness.layout();
    harness.tree.set_focus(opener.get());
    REQUIRE(harness.tree.focused_node() == opener.get());

    auto first = widget::Button::create("First");
    auto second = widget::Button::create("Second");
    auto col = widget::Column::create();
    col->add(first);
    col->add(second);
    (void)harness.sheet->set_content(col);

    harness.sheet->open();
    harness.layout();
    CHECK(harness.tree.focused_node() == first.get());

    harness.tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::tab, scene::KeyEvent::Action::press)
    );
    CHECK(harness.tree.focused_node() == second.get());

    harness.tree.dispatch_key(
        scene::KeyEvent(
            foundation::LogicalKey::tab,
            scene::KeyEvent::Action::press,
            {.shift = true}
        )
    );
    CHECK(harness.tree.focused_node() == first.get());

    harness.sheet->close();
    settle(harness);
    CHECK(harness.tree.focused_node() == opener.get());
    CHECK(harness.host->overlay_count() == 0);
}

TEST_CASE(
    "Sheet closed event and set_on_close survive owner release during notification",
    "[sheet][event][lifetime]"
) {
    DetachedSheetHarness harness;
    int callback_count = 0;
    int event_count = 0;
    harness.sheet->set_on_close([&callback_count] { ++callback_count; });
    auto sub = harness.sheet->closed().subscribe([&event_count] { ++event_count; });

    harness.sheet->open();
    harness.layout();
    harness.sheet->close();
    CHECK(callback_count == 0);
    tick(harness, 0.5F);
    tick(harness, 0.5F);
    CHECK(callback_count == 1);
    CHECK(event_count == 1);

    // 回调内释放最后一个外部 shared_ptr<Sheet> 不产生 use-after-free。
    auto owned = widget::Sheet::create("Owned");
    int trailing = 0;
    owned->set_on_close([&owned] { owned.reset(); });
    auto trailing_sub = owned->closed().subscribe([&trailing] { ++trailing; });
    owned->open();
    owned->close();
    owned->on_process(0.5F);
    CHECK(owned == nullptr);
    CHECK(trailing == 1);
}

TEST_CASE(
    "Sheet exposes dialog role on its visible panel and leaves no gap in parent layout",
    "[sheet][overlay][semantics][layout]"
) {
    auto host = scene::OverlayHost::create();
    auto column = widget::Column::create();
    column->set_gap(12.0F);
    auto above = std::make_shared<scene::NanControl>(foundation::NanSize(200.0F, 40.0F));
    auto below = std::make_shared<scene::NanControl>(foundation::NanSize(200.0F, 40.0F));
    auto sheet = widget::Sheet::create("配置检查器", widget::SheetSide::right);
    (void)sheet->set_content(std::make_shared<widget::Button>("保存"));
    column->add(above);
    column->add(sheet);
    column->add(below);
    host->set_content(column);

    scene::NanSceneTree tree;
    tree.set_root(host);
    REQUIRE(tree.layout_root(foundation::NanSize(800.0F, 600.0F)) >= 1);
    const auto before = below->position().get_y() - (above->position().get_y() + above->height());
    REQUIRE(before == Catch::Approx(12.0F));

    sheet->open();
    column->mark_layout_dirty();
    REQUIRE(tree.layout_root(foundation::NanSize(800.0F, 600.0F)) >= 1);
    const auto after = below->position().get_y() - (above->position().get_y() + above->height());
    CHECK(after == Catch::Approx(before));
    CHECK_FALSE(sheet->is_visible_in_tree());

    auto* surface = host->layer_at(1)->layout_root();
    REQUIRE(surface != nullptr);
    REQUIRE(surface->child_count() == 1);
    auto* panel = surface->get_child(0)->get_child(0)->get_child(0)->as_control();
    REQUIRE(panel != nullptr);
    REQUIRE(tree.update_semantics());
    const auto* sem = tree.semantics_tree().find(panel->semantics_id());
    REQUIRE(sem != nullptr);
    CHECK(sem->properties.role == semantics::Role::dialog);
    CHECK(sem->properties.label == "配置检查器");
}

TEST_CASE(
    "floating popup opened inside a Sheet stays above the scrim",
    "[sheet][overlay][nested]"
) {
    OverlaySheetHarness harness;
    auto select = widget::Select::create({"One", "Two", "Three"});
    (void)harness.sheet->set_content(select);
    harness.sheet->open();
    harness.layout();
    REQUIRE(harness.host->overlay_count() == 1);

    const auto field = foundation::NanPoint(
        select->global_bounds().get_left() + 10.0F,
        select->global_bounds().get_top() + 10.0F
    );
    harness.tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            field
        )
    );
    harness.layout();
    REQUIRE(select->is_open());
    REQUIRE(harness.host->overlay_count() == 2);

    auto* surface = harness.host->layer_at(1)->layout_root();
    REQUIRE(surface != nullptr);
    REQUIRE(surface->child_count() == 2);
    auto* popup = surface->get_child(1)->get_child(0)->as_control();
    REQUIRE(popup != nullptr);

    const auto row_height = popup->global_bounds().get_height() / 3.0F;
    const auto option_point = foundation::NanPoint(
        popup->global_bounds().get_left() + 10.0F,
        popup->global_bounds().get_top() + row_height * 1.5F
    );
    harness.tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            option_point
        )
    );
    CHECK(select->selected_index() == 1);
    CHECK_FALSE(select->is_open());
}

TEST_CASE(
    "Sheet ComponentTraits and validation reject invalid side extent and slot cycles",
    "[sheet][authoring][validation]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme_test::Manager themes;
    auto host = scene::OverlayHost::create();
    widget::BuildContext ui(graph, scope, themes, nullptr, host.get());

    auto body = ui.make<widget::Button>("Apply").build();
    auto sheet =
        ui.make<widget::Sheet>(widget::SheetSide::left, "导航抽屉", body).open(true).build();
    CHECK(sheet->side() == widget::SheetSide::left);
    CHECK(sheet->title() == "导航抽屉");
    CHECK(sheet->content() == body.get());
    CHECK(sheet->is_open());
    sheet->set_open(false);
    CHECK_FALSE(sheet->is_open());

    for (const float invalid_extent:
         {0.0F,
          -10.0F,
          std::numeric_limits<float>::infinity(),
          std::numeric_limits<float>::quiet_NaN()})
    {
        REQUIRE_THROWS_AS(sheet->set_extent(invalid_extent), std::invalid_argument);
    }

    REQUIRE_THROWS_AS(sheet->set_side(static_cast<widget::SheetSide>(99)), std::invalid_argument);
    REQUIRE_THROWS_AS(sheet->set_header(nullptr), std::invalid_argument);
    REQUIRE_THROWS_AS(sheet->set_content(nullptr), std::invalid_argument);
    REQUIRE_THROWS_AS(sheet->set_footer(nullptr), std::invalid_argument);

    // 拒绝自引用或祖先环。
    REQUIRE_THROWS_AS(sheet->set_content(sheet), std::invalid_argument);
    auto parent_box = widget::Column::create();
    parent_box->add(sheet);
    REQUIRE_THROWS_AS(sheet->set_content(parent_box), std::invalid_argument);

    // 拒绝已挂在其它父节点下的控件。
    auto other_parent = widget::Column::create();
    auto attached_child = widget::Button::create("Attached");
    other_parent->add(attached_child);
    REQUIRE_THROWS_AS(sheet->set_footer(attached_child), std::logic_error);

    // 一个控件只能占用一个 Sheet 槽位；跨槽复用必须事务性拒绝。
    auto header_sheet = widget::Sheet::create();
    auto header_node = widget::Button::create("Header");
    header_sheet->set_header(header_node);
    REQUIRE_THROWS_AS(header_sheet->set_content(header_node), std::invalid_argument);
    CHECK(header_sheet->header() == header_node.get());
    CHECK(header_sheet->content() == nullptr);
    REQUIRE_THROWS_AS(header_sheet->set_footer(header_node), std::invalid_argument);
    CHECK(header_sheet->header() == header_node.get());
    CHECK(header_sheet->footer() == nullptr);

    auto content_sheet = widget::Sheet::create();
    auto content_node = widget::Button::create("Content");
    content_sheet->set_content(content_node);
    REQUIRE_THROWS_AS(content_sheet->set_header(content_node), std::invalid_argument);
    CHECK(content_sheet->content() == content_node.get());
    CHECK(content_sheet->header() == nullptr);
    REQUIRE_THROWS_AS(content_sheet->set_footer(content_node), std::invalid_argument);
    CHECK(content_sheet->content() == content_node.get());
    CHECK(content_sheet->footer() == nullptr);

    auto footer_sheet = widget::Sheet::create();
    auto footer_node = widget::Button::create("Footer");
    footer_sheet->set_footer(footer_node);
    REQUIRE_THROWS_AS(footer_sheet->set_header(footer_node), std::invalid_argument);
    CHECK(footer_sheet->footer() == footer_node.get());
    CHECK(footer_sheet->header() == nullptr);
    REQUIRE_THROWS_AS(footer_sheet->set_content(footer_node), std::invalid_argument);
    CHECK(footer_sheet->footer() == footer_node.get());
    CHECK(footer_sheet->content() == nullptr);
}

TEST_CASE(
    "closed in-tree Sheet does not restore stale focus when later detached",
    "[sheet][focus][fallback]"
) {
    reactive::Graph graph;
    theme_test::Manager themes;
    auto root = std::make_shared<scene::NanControl>(foundation::NanSize(800.0F, 600.0F));
    auto opener = widget::Button::create("Open");
    auto later = widget::Button::create("Later");
    auto sheet = widget::Sheet::create();
    auto inside = widget::Button::create("Inside");
    (void)sheet->set_content(inside);
    root->add_child(opener);
    root->add_child(sheet);
    root->add_child(later);

    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(root);
    REQUIRE(tree.layout_root(foundation::NanSize(800.0F, 600.0F)) >= 1);
    tree.set_focus(opener.get());

    sheet->open();
    REQUIRE(tree.focused_node() == inside.get());
    sheet->close();
    for (int i = 0; i < 8; ++i) {
        sheet->on_process(0.1F);
        {
            auto phase = tree.enter_phase(scene::FramePhase::animation);
            tree.advance_animations(0.1F);
        }
    }
    REQUIRE(tree.focused_node() == opener.get());

    tree.set_focus(later.get());
    REQUIRE(tree.focused_node() == later.get());
    (void)root->remove_child(*sheet);
    CHECK(tree.focused_node() == later.get());
}
