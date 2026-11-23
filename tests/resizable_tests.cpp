#include "theme_test_support.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <nandina/reactive/graph.hpp>
#include <nandina/reactive/scope.hpp>
#include <nandina/render/render_device.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/authoring.hpp>
#include <nandina/widget/build_context.hpp>
#include <nandina/widget/builtin_component_traits.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/resizable.hpp>

#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace
{
    using namespace nandina;

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

    class WidthDependentBox final: public scene::NanControl {
    public:
        [[nodiscard]] auto on_measure(const foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override {
            const float width =
                std::isfinite(constraints.max_width) ? constraints.max_width : 20.0F;
            return constraints.constrain(foundation::NanSize(width, width));
        }
    };

    class HeightDependentBox final: public scene::NanControl {
    public:
        [[nodiscard]] auto on_measure(const foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override {
            const float height =
                std::isfinite(constraints.max_height) ? constraints.max_height : 20.0F;
            return constraints.constrain(foundation::NanSize(height, height));
        }
    };

    void click_left(scene::NanSceneTree& tree, const foundation::NanPoint pos) {
        tree.dispatch_mouse_button(
            scene::MouseButtonEvent(
                scene::MouseButtonEvent::Button::left,
                scene::MouseButtonEvent::Action::press,
                pos
            )
        );
        tree.dispatch_mouse_button(
            scene::MouseButtonEvent(
                scene::MouseButtonEvent::Button::left,
                scene::MouseButtonEvent::Action::release,
                pos
            )
        );
    }
} // namespace

TEST_CASE(
    "Resizable splits horizontal and vertical panes and preserves ratio on resize",
    "[widget][resizable]"
) {
    auto first = std::make_shared<scene::NanControl>();
    auto second = std::make_shared<scene::NanControl>();
    auto split = widget::Resizable::create(widget::LayoutAxis::horizontal);
    split->set_handle_thickness(8.0F);
    split->set_first_child(first);
    split->set_second_child(second);

    scene::NanSceneTree tree;
    tree.set_root(split);
    (void)tree.layout_root(foundation::NanSize(408.0F, 200.0F));

    CHECK(split->ratio() == Catch::Approx(0.5F));
    CHECK(split->effective_ratio() == Catch::Approx(0.5F));
    CHECK(first->width() == Catch::Approx(200.0F));
    CHECK(first->height() == Catch::Approx(200.0F));
    CHECK(split->handle_rect().get_x() == Catch::Approx(200.0F));
    CHECK(split->handle_rect().get_width() == Catch::Approx(8.0F));
    CHECK(second->position().get_x() == Catch::Approx(208.0F));
    CHECK(second->width() == Catch::Approx(200.0F));

    // Programmatic ratio update is silent and updates pane geometry immediately.
    int ratio_events = 0;
    split->set_on_ratio_changed([&ratio_events](float) { ++ratio_events; });
    split->set_ratio(0.25F);
    CHECK(ratio_events == 0);
    CHECK(first->width() == Catch::Approx(100.0F));
    CHECK(second->position().get_x() == Catch::Approx(108.0F));
    CHECK(second->width() == Catch::Approx(300.0F));

    // Outer resize preserves the 0.25 split ratio.
    (void)tree.layout_root(foundation::NanSize(808.0F, 240.0F));
    CHECK(first->width() == Catch::Approx(200.0F));
    CHECK(second->width() == Catch::Approx(600.0F));
    CHECK(first->height() == Catch::Approx(240.0F));

    // Switch to vertical split.
    split->set_axis(widget::LayoutAxis::vertical);
    (void)tree.layout_root(foundation::NanSize(300.0F, 408.0F));
    CHECK(first->width() == Catch::Approx(300.0F));
    CHECK(first->height() == Catch::Approx(100.0F));
    CHECK(split->handle_rect().get_y() == Catch::Approx(100.0F));
    CHECK(split->handle_rect().get_height() == Catch::Approx(8.0F));
    CHECK(second->position().get_y() == Catch::Approx(108.0F));
    CHECK(second->height() == Catch::Approx(300.0F));
}

TEST_CASE(
    "Resizable measures children at allocated pane main extents when cross axis is unbounded",
    "[widget][resizable][measure]"
) {
    auto first_h = std::make_shared<WidthDependentBox>();
    auto second_h = std::make_shared<WidthDependentBox>();
    auto horizontal = widget::Resizable::create(widget::LayoutAxis::horizontal);
    horizontal->set_handle_thickness(8.0F);
    horizontal->set_first_child(first_h);
    horizontal->set_second_child(second_h);

    const auto horizontal_size = horizontal->measure_layout(
        foundation::NanLayoutConstraints {
            .min_width = 0.0F,
            .max_width = 400.0F,
            .min_height = 0.0F,
            .max_height = std::numeric_limits<float>::infinity(),
        }
    );
    CHECK(horizontal_size.get_width() == Catch::Approx(400.0F));
    CHECK(horizontal_size.get_height() == Catch::Approx(196.0F));
    CHECK(first_h->measured_size().get_height() == Catch::Approx(196.0F));
    CHECK(second_h->measured_size().get_height() == Catch::Approx(196.0F));

    auto first_v = std::make_shared<HeightDependentBox>();
    auto second_v = std::make_shared<HeightDependentBox>();
    auto vertical = widget::Resizable::create(widget::LayoutAxis::vertical);
    vertical->set_handle_thickness(8.0F);
    vertical->set_first_child(first_v);
    vertical->set_second_child(second_v);

    const auto vertical_size = vertical->measure_layout(
        foundation::NanLayoutConstraints {
            .min_width = 0.0F,
            .max_width = std::numeric_limits<float>::infinity(),
            .min_height = 0.0F,
            .max_height = 400.0F,
        }
    );
    CHECK(vertical_size.get_width() == Catch::Approx(196.0F));
    CHECK(vertical_size.get_height() == Catch::Approx(400.0F));
    CHECK(first_v->measured_size().get_width() == Catch::Approx(196.0F));
    CHECK(second_v->measured_size().get_width() == Catch::Approx(196.0F));
}

TEST_CASE(
    "Resizable clamps minimum pane extents and degrades gracefully in tiny containers",
    "[widget][resizable]"
) {
    auto first = std::make_shared<scene::NanControl>();
    auto second = std::make_shared<scene::NanControl>();
    auto split = widget::Resizable::create(widget::LayoutAxis::horizontal);
    split->set_handle_thickness(8.0F);
    split->set_min_panes_extent(100.0F, 120.0F);
    split->set_first_child(first);
    split->set_second_child(second);

    scene::NanSceneTree tree;
    tree.set_root(split);
    (void)tree.layout_root(foundation::NanSize(408.0F, 160.0F));

    // Requesting 5% clamps first pane to min_first_extent (100 / 400 = 0.25).
    split->set_ratio(0.05F);
    CHECK(split->ratio() == Catch::Approx(0.05F));
    CHECK(split->effective_ratio() == Catch::Approx(0.25F));
    CHECK(first->width() == Catch::Approx(100.0F));
    CHECK(second->width() == Catch::Approx(300.0F));

    // Requesting 95% clamps second pane to min_second_extent (120 -> first = 280 / 400 = 0.70).
    split->set_ratio(0.95F);
    CHECK(split->effective_ratio() == Catch::Approx(0.70F));
    CHECK(first->width() == Catch::Approx(280.0F));
    CHECK(second->width() == Catch::Approx(120.0F));

    // Container shrinks below min_first + min_second + handle (100 + 120 + 8 = 228):
    // panes share available space proportionally without negative extents or overflow.
    (void)tree.layout_root(foundation::NanSize(118.0F, 160.0F));
    CHECK(first->width() >= 0.0F);
    CHECK(second->width() >= 0.0F);
    CHECK(first->width() == Catch::Approx(50.0F));
    CHECK(second->width() == Catch::Approx(60.0F));
    CHECK(
        first->width() + split->handle_rect().get_width() + second->width() == Catch::Approx(118.0F)
    );
}

TEST_CASE(
    "Resizable supports pointer dragging and double-click reset on the divider handle",
    "[widget][resizable]"
) {
    reactive::Graph graph;
    auto first = widget::Label::create(graph, "Left pane");
    auto second = widget::Label::create(graph, "Right pane");
    auto split = widget::Resizable::create(widget::LayoutAxis::horizontal);
    split->set_handle_thickness(8.0F);
    split->set_min_panes_extent(40.0F, 40.0F);
    split->set_default_ratio(0.40F);
    split->set_ratio(0.50F);
    split->set_first_child(first);
    split->set_second_child(second);

    std::vector<float> changes;
    split->set_on_ratio_changed([&changes](const float next) { changes.push_back(next); });

    scene::NanSceneTree tree;
    tree.set_root(split);
    (void)tree.layout_root(foundation::NanSize(408.0F, 200.0F));

    // Clicking inside the left pane does NOT focus the handle or start dragging.
    click_left(tree, foundation::NanPoint(50.0F, 100.0F));
    CHECK_FALSE(split->is_dragging());
    CHECK_FALSE(split->is_handle_focused());
    CHECK(changes.empty());

    // Pressing on the handle (at x = 203, inside [200, 208]) starts drag without jumping.
    tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::press,
            foundation::NanPoint(203.0F, 100.0F)
        )
    );
    CHECK(split->is_dragging());
    CHECK(split->is_handle_focused());
    CHECK(first->width() == Catch::Approx(200.0F));
    CHECK(changes.empty());

    // Drag right by +100 DIP (from 203 to 303) -> first pane becomes 300 DIP (ratio 0.75).
    tree.dispatch_mouse_move(
        scene::MouseMoveEvent(
            foundation::NanPoint(303.0F, 100.0F),
            foundation::NanPoint(100.0F, 0.0F)
        )
    );
    CHECK(first->width() == Catch::Approx(300.0F));
    CHECK(second->width() == Catch::Approx(100.0F));
    CHECK(split->effective_ratio() == Catch::Approx(0.75F));
    REQUIRE(changes.size() == 1);
    CHECK(changes.back() == Catch::Approx(0.75F));

    tree.dispatch_mouse_button(
        scene::MouseButtonEvent(
            scene::MouseButtonEvent::Button::left,
            scene::MouseButtonEvent::Action::release,
            foundation::NanPoint(303.0F, 100.0F)
        )
    );
    CHECK_FALSE(split->is_dragging());

    // Advance time past double-click window, then double-click handle at new position (x = 304).
    tree.process(0.50F);
    click_left(tree, foundation::NanPoint(304.0F, 100.0F));
    click_left(tree, foundation::NanPoint(304.0F, 100.0F));

    CHECK(split->effective_ratio() == Catch::Approx(0.40F));
    CHECK(first->width() == Catch::Approx(160.0F));
    CHECK(second->width() == Catch::Approx(240.0F));
    REQUIRE(changes.size() == 2);
    CHECK(changes.back() == Catch::Approx(0.40F));
}

TEST_CASE(
    "Resizable keyboard navigation, Tab order, semantics and disabled state",
    "[widget][resizable]"
) {
    auto left_btn = widget::Button::create("Left");
    auto right_btn = widget::Button::create("Right");
    auto split = widget::Resizable::create(widget::LayoutAxis::horizontal);
    split->set_handle_thickness(8.0F);
    split->set_min_panes_extent(40.0F, 40.0F);
    split->set_keyboard_step(0.10F);
    split->set_first_child(left_btn);
    split->set_second_child(right_btn);

    scene::NanSceneTree tree;
    tree.set_root(split);
    (void)tree.layout_root(foundation::NanSize(408.0F, 200.0F));

    // Pointer focus follows the hit pane; the handle must not steal clicks inside a pane.
    click_left(tree, foundation::NanPoint(100.0F, 100.0F));
    CHECK(tree.focused_node() == left_btn.get());
    CHECK_FALSE(split->is_handle_focused());

    // Tab follows child order [first, handle, second] without trapping focus in the split.
    tree.set_focus(left_btn.get());
    tree.dispatch_key(scene::KeyEvent(foundation::LogicalKey::tab, scene::KeyEvent::Action::press));
    CHECK(tree.focused_node() == &split->handle());
    tree.dispatch_key(scene::KeyEvent(foundation::LogicalKey::tab, scene::KeyEvent::Action::press));
    CHECK(tree.focused_node() == right_btn.get());
    tree.dispatch_key(
        scene::KeyEvent(
            foundation::LogicalKey::tab,
            scene::KeyEvent::Action::press,
            {.shift = true}
        )
    );
    CHECK(tree.focused_node() == &split->handle());

    // Focus handle directly and step with Left / Right / Home / End.
    REQUIRE(split->focus_handle());
    CHECK(tree.focused_node() == &split->handle());

    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::right, scene::KeyEvent::Action::press)
    );
    CHECK(split->effective_ratio() == Catch::Approx(0.60F));
    CHECK(left_btn->width() == Catch::Approx(240.0F));

    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::left, scene::KeyEvent::Action::press)
    );
    CHECK(split->effective_ratio() == Catch::Approx(0.50F));

    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::home, scene::KeyEvent::Action::press)
    );
    CHECK(split->effective_ratio() == Catch::Approx(0.10F));
    CHECK(left_btn->width() == Catch::Approx(40.0F));

    tree.dispatch_key(scene::KeyEvent(foundation::LogicalKey::end, scene::KeyEvent::Action::press));
    CHECK(split->effective_ratio() == Catch::Approx(0.90F));
    CHECK(left_btn->width() == Catch::Approx(360.0F));

    // Semantics actions on handle.
    (void)tree.update_semantics();
    const auto* handle_sem = tree.semantics_tree().find(split->handle().semantics_id());
    REQUIRE(handle_sem != nullptr);
    CHECK(handle_sem->properties.role == semantics::Role::slider);
    CHECK(handle_sem->properties.state.focusable);

    REQUIRE(tree.perform_semantics_action(
        split->handle().semantics_id(),
        semantics::ActionRequest {.action = semantics::Action::set_value, .value = "0.5"}
    ));
    CHECK(split->effective_ratio() == Catch::Approx(0.50F));

    REQUIRE(tree.perform_semantics_action(
        split->handle().semantics_id(),
        semantics::ActionRequest {.action = semantics::Action::decrement}
    ));
    CHECK(split->effective_ratio() == Catch::Approx(0.40F));

    const auto* before_thickness = tree.semantics_tree().find(split->handle().semantics_id());
    REQUIRE(before_thickness != nullptr);
    const auto previous_hint = before_thickness->properties.hint;
    split->set_handle_thickness(108.0F);
    REQUIRE(tree.update_semantics());
    const auto* resized_handle_sem = tree.semantics_tree().find(split->handle().semantics_id());
    REQUIRE(resized_handle_sem != nullptr);
    CHECK(resized_handle_sem->properties.hint != previous_hint);

    // Disabling clears focus and blocks keyboard/semantics resize.
    split->set_disabled(true);
    CHECK(tree.focused_node() == nullptr);
    CHECK_FALSE(split->focus_handle());
    CHECK_FALSE(tree.perform_semantics_action(
        split->handle().semantics_id(),
        semantics::ActionRequest {.action = semantics::Action::increment}
    ));
    CHECK(split->effective_ratio() == Catch::Approx(0.40F));
}

TEST_CASE(
    "Resizable callbacks are owner-release safe and theme drawing records primitives",
    "[widget][resizable]"
) {
    auto tree = std::make_unique<scene::NanSceneTree>();
    theme_test::Manager themes;
    tree->set_theme_manager(themes);

    auto split = widget::Resizable::create(widget::LayoutAxis::horizontal);
    split->set_first_child(std::make_shared<scene::NanControl>());
    split->set_second_child(std::make_shared<scene::NanControl>());
    tree->set_root(split);
    (void)tree->layout_root(foundation::NanSize(408.0F, 200.0F));

    // Draw in light and dark themes via RecordingDevice.
    RecordingDevice recorder;
    tree->draw(recorder);
    CHECK(!recorder.rects.empty());

    themes.set_preference(theme::ThemePreference::dark);
    recorder.rects.clear();
    tree->draw(recorder);
    CHECK(!recorder.rects.empty());

    // Owner-release safety: callback drops both tree root and the last external shared_ptr.
    REQUIRE(split->focus_handle());
    bool callback_ran = false;
    split->set_on_ratio_changed([&callback_ran, &tree, &split](float) {
        callback_ran = true;
        tree->set_root(nullptr);
        split.reset();
    });
    int event_after_release = 0;
    auto ratio_subscription =
        split->ratio_changed().subscribe([&event_after_release](float) { ++event_after_release; });
    tree->dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::right, scene::KeyEvent::Action::press)
    );
    CHECK(callback_ran);
    CHECK(split == nullptr);
    CHECK(event_after_release == 1);
}

TEST_CASE(
    "Resizable validates arguments, rejects cross-slot reuse and supports 3-pane DSL nesting",
    "[widget][resizable]"
) {
    auto split = widget::Resizable::create();
    CHECK_THROWS_AS(split->set_axis(static_cast<widget::LayoutAxis>(99)), std::invalid_argument);
    CHECK_THROWS_AS(
        split->set_ratio(std::numeric_limits<float>::quiet_NaN()),
        std::invalid_argument
    );
    CHECK_THROWS_AS(split->set_min_first_extent(-1.0F), std::invalid_argument);
    CHECK_THROWS_AS(split->set_min_second_extent(-1.0F), std::invalid_argument);
    CHECK_THROWS_AS(split->set_handle_thickness(0.5F), std::invalid_argument);
    CHECK_THROWS_AS(split->set_keyboard_step(0.0F), std::invalid_argument);
    CHECK_THROWS_AS(split->set_keyboard_step(1.5F), std::invalid_argument);
    CHECK_THROWS_AS(split->set_first_child(nullptr), std::invalid_argument);
    CHECK_THROWS_AS(split->set_first_child(split), std::invalid_argument);

    auto a = std::make_shared<scene::NanControl>();
    auto b = std::make_shared<scene::NanControl>();
    auto c = std::make_shared<scene::NanControl>();
    split->set_first_child(a);
    split->set_second_child(b);

    // Cross-slot reuse must fail transactionally without disturbing existing slots.
    CHECK_THROWS_AS(split->set_second_child(a), std::invalid_argument);
    CHECK(split->first_child() == a.get());
    CHECK(split->second_child() == b.get());

    CHECK_THROWS_AS(split->set_first_child(b), std::invalid_argument);
    CHECK(split->first_child() == a.get());
    CHECK(split->second_child() == b.get());

    // Adding a third child via add() throws std::logic_error.
    CHECK_THROWS_AS(split->add(c), std::logic_error);

    // Nested 3-pane DSL composition + reactive Signal<float> two-way binding.
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemeManager themes;
    widget::BuildContext ui(graph, scope, themes);

    auto& ratio_sig = ui.signal_value(0.30F);
    auto bound_split = ui.make<widget::Resizable>(ratio_sig, widget::LayoutAxis::horizontal)
                           .min_panes_extent(20.0F, 20.0F)
                           .first_child(ui.make<widget::Label>("Left"))
                           .second_child(ui.resizable(
                                               widget::LayoutAxis::vertical,
                                               ui.make<widget::Label>("Top right"),
                                               ui.make<widget::Label>("Bottom right")
                           )
                                             .ratio(0.40F))
                           .build();

    scene::NanSceneTree tree;
    tree.set_root(bound_split);
    (void)tree.layout_root(foundation::NanSize(508.0F, 308.0F));
    CHECK(bound_split->effective_ratio() == Catch::Approx(0.30F));
    CHECK(bound_split->first_child()->width() == Catch::Approx(150.0F));

    // Signal -> widget binding.
    ratio_sig.set(0.50F);
    CHECK(bound_split->ratio() == Catch::Approx(0.50F));
    CHECK(bound_split->first_child()->width() == Catch::Approx(250.0F));

    // Widget user interaction -> Signal binding.
    REQUIRE(bound_split->focus_handle());
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::right, scene::KeyEvent::Action::press)
    );
    CHECK(ratio_sig.get() == Catch::Approx(0.55F));
}
