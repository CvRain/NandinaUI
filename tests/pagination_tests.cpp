//
// Theme / Pagination tests.
//

#include <nandina/render/render_device.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/key_codes.hpp>
#include <nandina/widget/pagination.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <memory>
#include <vector>

using namespace nandina;

namespace
{
    class RecordingDevice final: public render::IRenderDevice {
    public:
        int rounded_rects = 0;
        int text_draws = 0;

        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect&, const foundation::NanColor&) override {}
        void draw_rect_outline(
            const foundation::NanRect&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_rounded_rect(
            const foundation::NanRect&,
            float,
            const foundation::NanColor&
        ) override {
            ++rounded_rects;
        }
        void draw_line(
            const foundation::NanPoint&,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_circle(const foundation::NanPoint&, float, const foundation::NanColor&) override {}
        void draw_text(
            std::string_view,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {
            ++text_draws;
        }
    };

    [[nodiscard]] auto pages_of(const widget::Pagination& pagination) -> std::vector<int> {
        std::vector<int> pages;
        for (std::size_t index = 0; index < pagination.visible_item_count(); ++index) {
            if (pagination.item_is_page(index)) {
                pages.push_back(pagination.item_page(index));
            }
        }
        return pages;
    }

    [[nodiscard]] auto ellipsis_count(const widget::Pagination& pagination) -> int {
        int count = 0;
        const auto total = pagination.visible_item_count();
        for (std::size_t index = 0; index < total; ++index) {
            if (!pagination.item_is_page(index) && index != 0 && index + 1 != total) {
                ++count;
            }
        }
        return count;
    }
} // namespace

TEST_CASE("pagination resolves slot, label and ellipsis tokens", "[pagination][theme]") {
    const auto design = theme::default_design_system();
    const auto style = theme::resolve_pagination(
        design,
        theme::ColorAppearance::light,
        theme::PaginationVisualState::normal
    );

    REQUIRE(style.container.fill.alpha() == Catch::Approx(0.0F));
    REQUIRE(style.item.fill.alpha() == Catch::Approx(0.0F));
    REQUIRE(
        style.item_active.fill.oklch().light == Catch::Approx(design.light.primary.oklch().light)
    );
    REQUIRE(
        style.label.color.oklch().light == Catch::Approx(design.light.foreground.oklch().light)
    );
    REQUIRE(
        style.label_active.color.oklch().light
        == Catch::Approx(design.light.primary_foreground.oklch().light)
    );
    REQUIRE(
        style.ellipsis.oklch().light
        == Catch::Approx(design.light.muted_foreground.oklch().light)
    );
    REQUIRE(style.focus.width == Catch::Approx(0.0F)); // focused 规则按需开启
    REQUIRE(style.metrics.box_size == Catch::Approx(32.0F));
    REQUIRE(style.metrics.gap == Catch::Approx(design.tokens.spacing.xs));
}

TEST_CASE("pagination disabled state scales slot and label alpha", "[pagination][theme]") {
    const auto design = theme::default_design_system();
    const auto normal = theme::resolve_pagination(
        design,
        theme::ColorAppearance::light,
        theme::PaginationVisualState::normal
    );
    const auto disabled = theme::resolve_pagination(
        design,
        theme::ColorAppearance::light,
        theme::PaginationVisualState::disabled
    );

    const float expected = design.tokens.opacity.disabled;
    REQUIRE(
        disabled.item_active.fill.alpha()
        == Catch::Approx(normal.item_active.fill.alpha() * expected)
    );
    REQUIRE(disabled.label.color.alpha() == Catch::Approx(normal.label.color.alpha() * expected));
    REQUIRE(disabled.ellipsis.alpha() == Catch::Approx(normal.ellipsis.alpha() * expected));
    REQUIRE(disabled.focus.color.alpha() == Catch::Approx(0.0F));
}

TEST_CASE("pagination shows every page for small counts and skips ellipsis", "[pagination][window]") {
    auto pagination = widget::Pagination::create();
    pagination->set_page_count(5);
    pagination->set_current_page(3);

    REQUIRE(pagination->page_count() == 5);
    REQUIRE(pagination->current_page() == 3);
    REQUIRE(pages_of(*pagination) == std::vector<int> {1, 2, 3, 4, 5});
    REQUIRE(ellipsis_count(*pagination) == 0);
    // prev + 5 pages + next
    REQUIRE(pagination->visible_item_count() == 7);
    REQUIRE(pagination->roving_member_count() == pagination->visible_item_count());
}

TEST_CASE("pagination collapses a long range around the current page", "[pagination][window]") {
    auto pagination = widget::Pagination::create();
    pagination->set_page_count(10);
    pagination->set_current_page(5);

    REQUIRE(pages_of(*pagination) == std::vector<int> {1, 4, 5, 6, 10});
    REQUIRE(ellipsis_count(*pagination) == 2);
    // prev + 1 + … + 4 5 6 + … + 10 + next
    REQUIRE(pagination->visible_item_count() == 9);
}

TEST_CASE("pagination windows clamp correctly at both edges", "[pagination][window]") {
    auto pagination = widget::Pagination::create();
    pagination->set_page_count(10);

    pagination->set_current_page(1);
    REQUIRE(pages_of(*pagination) == std::vector<int> {1, 2, 10});
    REQUIRE(ellipsis_count(*pagination) == 1);

    pagination->set_current_page(10);
    REQUIRE(pages_of(*pagination) == std::vector<int> {1, 9, 10});
    REQUIRE(ellipsis_count(*pagination) == 1);
}

TEST_CASE("pagination sibling count zero keeps only boundaries and current", "[pagination][window]") {
    auto pagination = widget::Pagination::create();
    pagination->set_page_count(7);
    pagination->set_sibling_count(0);
    pagination->set_current_page(4);

    REQUIRE(pagination->sibling_count() == 0);
    REQUIRE(pages_of(*pagination) == std::vector<int> {1, 4, 7});
    REQUIRE(ellipsis_count(*pagination) == 2);

    pagination->set_sibling_count(-3);
    REQUIRE(pagination->sibling_count() == 0);
}

TEST_CASE("pagination clamps page count and current page", "[pagination][boundary]") {
    auto pagination = widget::Pagination::create();
    pagination->set_page_count(5);
    REQUIRE(pagination->current_page() == 1);

    pagination->set_current_page(99);
    REQUIRE(pagination->current_page() == 5);
    pagination->set_current_page(-4);
    REQUIRE(pagination->current_page() == 1);
    pagination->set_current_page(0);
    REQUIRE(pagination->current_page() == 1);
}

TEST_CASE("pagination with no pages measures to zero and is not focusable", "[pagination][boundary]") {
    auto pagination = widget::Pagination::create();
    REQUIRE(pagination->page_count() == 0);
    REQUIRE(pagination->current_page() == 0);
    REQUIRE(pagination->visible_item_count() == 0);
    REQUIRE_FALSE(pagination->is_focusable());
    const auto measured = pagination->measure_layout(scene::LayoutConstraints::loose());
    REQUIRE(measured.get_width() == Catch::Approx(0.0F));
    REQUIRE(measured.get_height() == Catch::Approx(0.0F));

    pagination->set_page_count(-5);
    REQUIRE(pagination->page_count() == 0);
    pagination->set_page_count(3);
    REQUIRE(pagination->page_count() == 3);
    REQUIRE(pagination->current_page() == 1);
    REQUIRE(pagination->is_focusable());
}

TEST_CASE("pagination notifies only on user page changes", "[pagination][event]") {
    auto pagination = widget::Pagination::create();
    pagination->set_page_count(10);
    int callback_count = 0;
    int last_page = 0;
    pagination->set_on_page_change([&](const int page) {
        ++callback_count;
        last_page = page;
    });
    int event_count = 0;
    const auto subscription = pagination->page_changed().subscribe([&](const int) {
        ++event_count;
    });

    // 程序化 setter 静默。
    pagination->set_current_page(4);
    REQUIRE(pagination->current_page() == 4);
    REQUIRE(callback_count == 0);
    REQUIRE(event_count == 0);

    // 用户路径：回调 + 事件各一次。
    pagination->go_to_page(6);
    REQUIRE(pagination->current_page() == 6);
    REQUIRE(callback_count == 1);
    REQUIRE(event_count == 1);
    REQUIRE(last_page == 6);

    // 落在当前页是 no-op。
    pagination->go_to_page(6);
    REQUIRE(callback_count == 1);
    REQUIRE(event_count == 1);

    // 越界钳制后仍算一次变更。
    pagination->go_to_page(99);
    REQUIRE(pagination->current_page() == 10);
    REQUIRE(callback_count == 2);
    REQUIRE(event_count == 2);
}

TEST_CASE("pagination disabled blocks activation and dims the style", "[pagination][disabled]") {
    auto pagination = widget::Pagination::create();
    pagination->set_page_count(10);
    pagination->set_disabled(true);
    REQUIRE(pagination->disabled());
    REQUIRE_FALSE(pagination->is_focusable());
    REQUIRE(pagination->visual_state() == theme::PaginationVisualState::disabled);

    pagination->go_to_page(5);
    REQUIRE(pagination->current_page() == 1);
    REQUIRE(
        pagination->resolved_style().item_active.fill.alpha()
        == Catch::Approx(theme::default_theme().tokens.opacity.disabled)
    );

    pagination->set_disabled(false);
    REQUIRE(pagination->is_focusable());
    REQUIRE(pagination->visual_state() == theme::PaginationVisualState::normal);
}

TEST_CASE("pagination roams page slots through the real key path", "[pagination][keyboard]") {
    auto pagination = widget::Pagination::create();
    pagination->set_page_count(10);
    pagination->set_current_page(5);

    scene::NanSceneTree tree;
    tree.set_root(pagination);
    REQUIRE(tree.layout_root(foundation::NanSize(480.0F, 80.0F)) >= 1);
    tree.set_focus(pagination.get());

    // 槽位：0 prev / 1 page1 / 2 … / 3 page4 / 4 page5 / 5 page6 / 6 … / 7 page10 / 8 next
    REQUIRE(pagination->focused_item_index() == 4);
    REQUIRE(pagination->roving_member_count() == pagination->visible_item_count());

    const auto press = [&](const int keycode) {
        tree.dispatch_key(scene::KeyEvent(keycode, scene::KeyEvent::Action::press));
    };

    press(widget::keys::right);
    REQUIRE(pagination->focused_item_index() == 5);
    // 跳过省略号槽位（6）。
    press(widget::keys::right);
    REQUIRE(pagination->focused_item_index() == 7);
    press(widget::keys::right);
    REQUIRE(pagination->focused_item_index() == 8);
    // 复用 RovingFocus 的环绕语义。
    press(widget::keys::right);
    REQUIRE(pagination->focused_item_index() == 0);

    press(widget::keys::end);
    REQUIRE(pagination->focused_item_index() == 8);
    press(widget::keys::home);
    REQUIRE(pagination->focused_item_index() == 0);
}

TEST_CASE("pagination arrow roaming never lands on a disabled edge slot", "[pagination][keyboard]") {
    auto pagination = widget::Pagination::create();
    pagination->set_page_count(10);
    pagination->set_current_page(10);

    scene::NanSceneTree tree;
    tree.set_root(pagination);
    REQUIRE(tree.layout_root(foundation::NanSize(480.0F, 80.0F)) >= 1);
    tree.set_focus(pagination.get());

    // 当前页在末尾：槽位 0 prev / 1 page1 / 2 … / 3 page9 / 4 page10 / 5 next(禁用)
    REQUIRE(pagination->focused_item_index() == 4);
    tree.dispatch_key(scene::KeyEvent(widget::keys::end, scene::KeyEvent::Action::press));
    REQUIRE(pagination->focused_item_index() == 4);
    const int before = pagination->focused_item_index();
    tree.dispatch_key(scene::KeyEvent(widget::keys::right, scene::KeyEvent::Action::press));
    REQUIRE(pagination->focused_item_index() != before);
    REQUIRE(pagination->focused_item_index() != 5); // next 禁用，不会被选中
}

TEST_CASE("pagination Enter activates the focused slot", "[pagination][keyboard]") {
    auto pagination = widget::Pagination::create();
    pagination->set_page_count(10);
    pagination->set_current_page(5);
    int event_count = 0;
    const auto subscription = pagination->page_changed().subscribe([&](const int) {
        ++event_count;
    });

    scene::NanSceneTree tree;
    tree.set_root(pagination);
    REQUIRE(tree.layout_root(foundation::NanSize(480.0F, 80.0F)) >= 1);
    tree.set_focus(pagination.get());

    tree.dispatch_key(scene::KeyEvent(widget::keys::right, scene::KeyEvent::Action::press));
    REQUIRE(pagination->focused_item_index() == 5); // page 6
    tree.dispatch_key(scene::KeyEvent(widget::keys::enter, scene::KeyEvent::Action::press));
    REQUIRE(pagination->current_page() == 6);
    REQUIRE(event_count == 1);

    // 当前页变化后窗口重建，漫游焦点回到当前页槽位。
    REQUIRE(pagination->focused_item_index() == 4);
    tree.dispatch_key(scene::KeyEvent(widget::keys::space, scene::KeyEvent::Action::press));
    REQUIRE(pagination->current_page() == 6); // 激活当前页是 no-op
    REQUIRE(event_count == 1);
}

TEST_CASE("pagination pointer press activates a page slot", "[pagination][pointer]") {
    auto pagination = widget::Pagination::create();
    pagination->set_page_count(5);
    REQUIRE(pagination->current_page() == 1);

    scene::NanSceneTree tree;
    tree.set_root(pagination);
    REQUIRE(tree.layout_root(foundation::NanSize(480.0F, 80.0F)) >= 1);

    const auto style = pagination->resolved_style();
    // 槽位 3 = page 3（0 = prev，1 = page1，2 = page2）。
    const float x = style.metrics.padding_x + 3.0F * (style.metrics.box_size + style.metrics.gap)
        + style.metrics.box_size * 0.5F;
    const float y = pagination->height() * 0.5F;
    const auto point = foundation::NanPoint(x, y);
    tree.dispatch_mouse_move(scene::MouseMoveEvent(point, foundation::NanPoint(0.0F, 0.0F)));
    tree.dispatch_mouse_button(scene::MouseButtonEvent(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        point
    ));
    REQUIRE(pagination->current_page() == 3);
    REQUIRE(pagination->focused_item_index() >= 0);
}

TEST_CASE("pagination override patches fields and survives a system apply", "[pagination][override]") {
    theme::ThemeManager themes;
    auto pagination = widget::Pagination::create();
    pagination->set_page_count(10);
    pagination->set_current_page(3);
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(pagination);
    REQUIRE(tree.layout_root(foundation::NanSize(480.0F, 80.0F)) >= 1);

    pagination->set_override(theme::PaginationRecipeRule {
        .item_active_fill = theme::ThemeColor::token(theme::ColorToken::error),
        .metrics_box_size = theme::ThemeScalar::literal(40.0F),
    });
    REQUIRE(
        pagination->resolved_style().item_active.fill.oklch().light
        == Catch::Approx(themes.design_system().light.error.oklch().light)
    );
    REQUIRE(pagination->resolved_style().metrics.box_size == Catch::Approx(40.0F));

    auto design = theme::default_design_system();
    design.light.error = theme::nan_color(0.60F, 0.20F, 20.0F);
    themes.apply(std::make_shared<const theme::DesignSystem>(std::move(design)));
    REQUIRE(
        pagination->resolved_style().item_active.fill.oklch().light == Catch::Approx(0.60F)
    );
    REQUIRE(pagination->resolved_style().metrics.box_size == Catch::Approx(40.0F));
}

TEST_CASE("pagination re-resolves after an appearance switch", "[pagination][theme]") {
    theme::ThemeManager themes;
    auto pagination = widget::Pagination::create();
    pagination->set_page_count(5);
    pagination->set_override(theme::PaginationRecipeRule {
        .label_color = theme::ThemeColor::token(theme::ColorToken::foreground),
    });
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(pagination);
    REQUIRE(tree.layout_root(foundation::NanSize(480.0F, 80.0F)) >= 1);

    const float light = pagination->resolved_style().label.color.oklch().light;
    themes.set_system_appearance(theme::ColorAppearance::dark);
    REQUIRE(
        pagination->resolved_style().label.color.oklch().light
        == Catch::Approx(themes.design_system().dark.foreground.oklch().light)
    );
    REQUIRE(pagination->resolved_style().label.color.oklch().light != Catch::Approx(light));
}

TEST_CASE("pagination exposes a generic page-position semantics node", "[pagination][semantics]") {
    auto pagination = widget::Pagination::create();
    pagination->set_page_count(10);
    pagination->set_current_page(3);

    scene::NanSceneTree tree;
    tree.set_root(pagination);
    REQUIRE(tree.layout_root(foundation::NanSize(480.0F, 80.0F)) >= 1);
    REQUIRE(tree.update_semantics());

    const auto* node = tree.semantics_tree().find(pagination->semantics_id());
    REQUIRE(node != nullptr);
    REQUIRE(node->properties.role == semantics::Role::generic);
    REQUIRE(node->properties.label == "Page 3 of 10");
    REQUIRE(node->properties.value == "3");
    REQUIRE(node->properties.state.focusable);
    REQUIRE(semantics::supports(node->properties.actions, semantics::Action::focus));
}

TEST_CASE("pagination paints one glyph per visible slot", "[pagination][paint]") {
    auto pagination = widget::Pagination::create();
    pagination->set_page_count(5);
    scene::NanSceneTree tree;
    tree.set_root(pagination);
    REQUIRE(tree.layout_root(foundation::NanSize(480.0F, 80.0F)) >= 1);

    RecordingDevice device;
    tree.draw(device);
    REQUIRE(device.text_draws == 7); // prev + 5 pages + next
    REQUIRE(device.rounded_rects >= 1); // 当前页实色槽位
}

TEST_CASE("pagination is buildable through BuildContext", "[pagination][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    auto pagination = ui.make<widget::Pagination>(10, 3).build();
    REQUIRE(pagination != nullptr);
    REQUIRE(pagination->page_count() == 10);
    REQUIRE(pagination->current_page() == 3);

    auto& page = ui.signal<int>(1);
    auto bound = ui.make<widget::Pagination>(page, 8).build();
    REQUIRE(bound->current_page() == 1);
    bound->go_to_page(4);
    REQUIRE(page.peek() == 4);
    page.set(2);
    REQUIRE(bound->current_page() == 2);
}
