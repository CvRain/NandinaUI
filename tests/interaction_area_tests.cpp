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
    auto pointer_button(scene::MouseButtonEvent::Action action, float x = 10.0F)
        -> scene::MouseButtonEvent {
        return scene::MouseButtonEvent {
            scene::MouseButtonEvent::Button::left,
            action,
            foundation::NanPoint(x, 10.0F),
        };
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
