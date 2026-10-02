#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/gesture_area.hpp>
#include <nandina/widget/pointer_area.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/controls.hpp>
#include <nandina/widget/primitives/pressable.hpp>

#include <catch2/catch_test_macros.hpp>

#include <memory>

using namespace nandina;

namespace
{
    auto button_event(
        const scene::MouseButtonEvent::Button button,
        const scene::MouseButtonEvent::Action action,
        const float x = 10.0F
    ) -> scene::MouseButtonEvent {
        return scene::MouseButtonEvent {button, action, foundation::NanPoint(x, 10.0F)};
    }

    auto pointer_button(scene::MouseButtonEvent::Action action, float x = 10.0F)
        -> scene::MouseButtonEvent {
        return button_event(scene::MouseButtonEvent::Button::left, action, x);
    }

}

TEST_CASE("pressable reports press release cancel hover and focus", "[interaction][pressable]") {
    widget::primitives::Pressable pressable {foundation::NanSize(100.0F, 40.0F)};
    int presses = 0;
    int releases = 0;
    int cancels = 0;
    int hover_changes = 0;
    int focus_changes = 0;
    pressable.set_on_press([&] { ++presses; });
    pressable.set_on_release([&] { ++releases; });
    pressable.set_on_cancel([&] { ++cancels; });
    pressable.set_on_hover_changed([&](bool) { ++hover_changes; });
    pressable.set_on_focus_changed([&](bool) { ++focus_changes; });

    scene::MouseEnterEvent enter {foundation::NanPoint(10.0F, 10.0F)};
    pressable.on_input(enter);
    auto down = pointer_button(scene::MouseButtonEvent::Action::press);
    pressable.on_input(down);
    scene::MouseLeaveEvent leave {foundation::NanPoint(120.0F, 10.0F)};
    pressable.on_input(leave);
    scene::FocusEnterEvent focus;
    pressable.on_input(focus);
    scene::FocusLeaveEvent blur;
    pressable.on_input(blur);

    REQUIRE(presses == 1);
    REQUIRE(releases == 0);
    REQUIRE(cancels == 1);
    REQUIRE(hover_changes == 2);
    REQUIRE(focus_changes == 2);
}

TEST_CASE("pointer area observes bubbled events and captures a press", "[interaction][pointer-area]") {
    scene::NanSceneTree tree;
    auto area = std::make_shared<widget::PointerArea>();
    area->set_size(foundation::NanSize(100.0F, 40.0F));
    area->set_child(std::make_shared<scene::NanControl>(foundation::NanSize(100.0F, 40.0F)));
    tree.set_root(area);
    int downs = 0;
    int moves = 0;
    int ups = 0;
    area->set_on_pointer_down([&](const auto&) { ++downs; });
    area->set_on_pointer_move([&](const auto&) { ++moves; });
    area->set_on_pointer_up([&](const auto&) { ++ups; });

    tree.dispatch_mouse_move(scene::MouseMoveEvent {
        foundation::NanPoint(10.0F, 10.0F), foundation::NanPoint::zero()
    });
    tree.dispatch_mouse_button(pointer_button(scene::MouseButtonEvent::Action::press));
    REQUIRE(tree.pointer_capture() != nullptr);
    tree.dispatch_mouse_move(scene::MouseMoveEvent {
        foundation::NanPoint(140.0F, 10.0F), foundation::NanPoint(130.0F, 0.0F)
    });
    tree.dispatch_mouse_button(pointer_button(scene::MouseButtonEvent::Action::release, 140.0F));

    REQUIRE(downs == 1);
    REQUIRE(moves >= 1);
    REQUIRE(ups == 1);
    REQUIRE(tree.pointer_capture() == nullptr);
}

TEST_CASE("gesture area keeps single and double click mutually exclusive", "[interaction][gesture-area]") {
    scene::NanSceneTree tree;
    auto area = std::make_shared<widget::GestureArea>();
    area->set_size(foundation::NanSize(100.0F, 40.0F));
    tree.set_root(area);
    int clicks = 0;
    int doubles = 0;
    area->set_on_click([&](const auto&) { ++clicks; });
    area->set_on_double_click([&](const auto&) { ++doubles; });
    tree.dispatch_mouse_move(scene::MouseMoveEvent {
        foundation::NanPoint(10.0F, 10.0F), foundation::NanPoint::zero()
    });

    tree.dispatch_mouse_button(pointer_button(scene::MouseButtonEvent::Action::press));
    tree.dispatch_mouse_button(pointer_button(scene::MouseButtonEvent::Action::release));
    tree.process(0.10F);
    tree.dispatch_mouse_button(pointer_button(scene::MouseButtonEvent::Action::press));
    tree.dispatch_mouse_button(pointer_button(scene::MouseButtonEvent::Action::release));
    tree.process(0.40F);

    REQUIRE(doubles == 1);
    REQUIRE(clicks == 0);
}

TEST_CASE("gesture area observes an interactive child during capture", "[interaction][gesture-area]") {
    scene::NanSceneTree tree;
    auto area = std::make_shared<widget::GestureArea>();
    auto button = std::make_shared<widget::Button>("Open");
    area->set_size(foundation::NanSize(100.0F, 40.0F));
    area->set_child(button);
    tree.set_root(area);
    int button_clicks = 0;
    int gesture_clicks = 0;
    button->set_on_click([&] { ++button_clicks; });
    area->set_on_click([&](const auto&) { ++gesture_clicks; });
    tree.dispatch_mouse_move(scene::MouseMoveEvent {
        foundation::NanPoint(10.0F, 10.0F), foundation::NanPoint::zero()
    });
    tree.dispatch_mouse_button(pointer_button(scene::MouseButtonEvent::Action::press));
    tree.dispatch_mouse_button(pointer_button(scene::MouseButtonEvent::Action::release));

    REQUIRE(button_clicks == 1);
    REQUIRE(gesture_clicks == 1);
}

TEST_CASE("drag and long press suppress click recognition", "[interaction][gesture-area]") {
    scene::NanSceneTree tree;
    auto area = std::make_shared<widget::GestureArea>();
    area->set_size(foundation::NanSize(100.0F, 40.0F));
    tree.set_root(area);
    int clicks = 0;
    int drag_starts = 0;
    int drag_moves = 0;
    int drag_ends = 0;
    int long_presses = 0;
    area->set_on_click([&](const auto&) { ++clicks; });
    area->set_on_drag_start([&](const auto&) { ++drag_starts; });
    area->set_on_drag_move([&](const auto&) { ++drag_moves; });
    area->set_on_drag_end([&](const auto&) { ++drag_ends; });
    area->set_on_long_press([&](const auto&) { ++long_presses; });
    tree.dispatch_mouse_move(scene::MouseMoveEvent {
        foundation::NanPoint(10.0F, 10.0F), foundation::NanPoint::zero()
    });

    tree.dispatch_mouse_button(pointer_button(scene::MouseButtonEvent::Action::press));
    tree.dispatch_mouse_move(scene::MouseMoveEvent {
        foundation::NanPoint(30.0F, 10.0F), foundation::NanPoint(20.0F, 0.0F)
    });
    tree.dispatch_mouse_button(pointer_button(scene::MouseButtonEvent::Action::release, 30.0F));
    tree.process(0.40F);
    REQUIRE(drag_starts == 1);
    REQUIRE(drag_moves == 1);
    REQUIRE(drag_ends == 1);
    REQUIRE(clicks == 0);

    tree.dispatch_mouse_move(scene::MouseMoveEvent {
        foundation::NanPoint(10.0F, 10.0F), foundation::NanPoint::zero()
    });
    tree.dispatch_mouse_button(pointer_button(scene::MouseButtonEvent::Action::press));
    tree.process(0.60F);
    tree.dispatch_mouse_button(pointer_button(scene::MouseButtonEvent::Action::release));
    tree.process(0.40F);
    REQUIRE(long_presses == 1);
    REQUIRE(clicks == 0);
}

TEST_CASE(
    "a compound pointer gesture is observable above an interactive child",
    "[interaction][gesture-area]"
) {
    // 场景：左键按住不松手，期间用右键点一下，再松开左键。
    // 这是“需要设备细节的复杂交互”——框架的答案是组合 GestureArea，而不是给 Button 堆事件。
    scene::NanSceneTree tree;
    auto area = std::make_shared<widget::GestureArea>();
    auto button = std::make_shared<widget::Button>("Hold");
    area->set_size(foundation::NanSize(100.0F, 40.0F));
    area->set_child(button);
    tree.set_root(area);

    // 原始层（PointerArea 继承下来的）能看到**每个**按钮的 down/up，
    // 包括 GestureArea 自己不识别的右键。
    int left_downs = 0;
    int right_downs = 0;
    int right_ups = 0;
    int left_ups = 0;
    area->set_on_pointer_down([&](const scene::MouseButtonEvent& event) {
        if (event.button() == scene::MouseButtonEvent::Button::left) {
            ++left_downs;
        }
        else if (event.button() == scene::MouseButtonEvent::Button::right) {
            ++right_downs;
        }
    });
    area->set_on_pointer_up([&](const scene::MouseButtonEvent& event) {
        if (event.button() == scene::MouseButtonEvent::Button::left) {
            ++left_ups;
        }
        else if (event.button() == scene::MouseButtonEvent::Button::right) {
            ++right_ups;
        }
    });
    int button_clicks = 0;
    int button_cancels = 0;
    button->set_on_click([&] { ++button_clicks; });
    button->set_on_cancel([&] { ++button_cancels; });

    tree.dispatch_mouse_move(scene::MouseMoveEvent {
        foundation::NanPoint(10.0F, 10.0F), foundation::NanPoint::zero()
    });
    tree.dispatch_mouse_button(button_event(scene::MouseButtonEvent::Button::left, scene::MouseButtonEvent::Action::press));
    tree.dispatch_mouse_button(button_event(scene::MouseButtonEvent::Button::right, scene::MouseButtonEvent::Action::press));
    tree.dispatch_mouse_button(button_event(scene::MouseButtonEvent::Button::right, scene::MouseButtonEvent::Action::release));
    tree.dispatch_mouse_button(button_event(scene::MouseButtonEvent::Button::left, scene::MouseButtonEvent::Action::release));

    // 四次转换全部可观察：右键的 down/up 也拿得到，而且在按住左键期间右键
    // **不会打乱** GestureArea 自己那份左键状态机（它显式地对非主键早退）。
    REQUIRE(left_downs == 1);
    REQUIRE(right_downs == 1);
    REQUIRE(right_ups == 1);
    REQUIRE(left_ups == 1);
    REQUIRE(button_cancels == 0);
    // 代价：左键那一次对 Button 来说仍是一次合法点击，语义激活照常发生。
    // 想让复合手势“吸收”掉这次点击，框架目前没有直接的开关。
    REQUIRE(button_clicks == 1);
}

TEST_CASE("BuildContext authors gesture areas around ordinary controls", "[interaction][authoring]") {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};
    int double_clicks = 0;

    auto area = ui.make<widget::GestureArea>()
                    .on_double_click([&](const scene::MouseButtonEvent&) { ++double_clicks; })
                    .child(ui.make<widget::Label>("Double-click me"))
                    .build();

    REQUIRE(area->child_count() == 1);
}
