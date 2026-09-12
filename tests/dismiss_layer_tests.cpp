#include <nandina/widget/internal/dismiss_layer.hpp>
#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace nandina;

TEST_CASE("dismiss layer reports left pointer dismissal", "[overlay][dismiss]") {
    widget::internal::DismissReason reason {};
    int calls = 0;
    widget::internal::DismissLayer layer([&](const auto value) {
        reason = value;
        ++calls;
    });
    scene::MouseButtonEvent event(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(10.0F, 20.0F)
    );

    REQUIRE(layer.on_input(event));
    REQUIRE(event.is_accepted());
    REQUIRE(calls == 1);
    REQUIRE(reason == widget::internal::DismissReason::pointer);
}

TEST_CASE("dismiss layer reports escape dismissal", "[overlay][dismiss]") {
    widget::internal::DismissReason reason {};
    widget::internal::DismissLayer layer([&](const auto value) { reason = value; });
    scene::KeyEvent event(256, scene::KeyEvent::Action::press);

    REQUIRE(layer.on_input(event));
    REQUIRE(event.is_accepted());
    REQUIRE(reason == widget::internal::DismissReason::escape);
}

TEST_CASE("dismiss layer ignores non-dismiss input", "[overlay][dismiss]") {
    int calls = 0;
    widget::internal::DismissLayer layer([&](auto) { ++calls; });
    scene::MouseButtonEvent release(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::release,
        foundation::NanPoint(10.0F, 20.0F)
    );
    scene::MouseButtonEvent right(
        scene::MouseButtonEvent::Button::right,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(10.0F, 20.0F)
    );
    scene::KeyEvent other(65, scene::KeyEvent::Action::press);

    REQUIRE_FALSE(layer.on_input(release));
    REQUIRE_FALSE(layer.on_input(right));
    REQUIRE_FALSE(layer.on_input(other));
    REQUIRE(calls == 0);
}

TEST_CASE("dismiss layer distinguishes content clicks and receives bubbled escape", "[overlay][dismiss]") {
    int calls = 0;
    widget::internal::DismissReason reason {};
    auto layer = std::make_shared<widget::internal::DismissLayer>([&](const auto value) {
        reason = value;
        ++calls;
    });
    auto content = std::make_shared<scene::NanControl>(foundation::NanSize(40.0F, 30.0F));
    content->set_position(foundation::NanPoint(20.0F, 10.0F));
    layer->set_content(content);
    scene::NanSceneTree tree;
    tree.set_root(layer);
    (void)tree.layout_root(foundation::NanSize(100.0F, 80.0F));

    tree.dispatch_mouse_button(scene::MouseButtonEvent(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(30.0F, 20.0F)
    ));
    REQUIRE(calls == 0);

    tree.dispatch_mouse_button(scene::MouseButtonEvent(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(90.0F, 70.0F)
    ));
    REQUIRE(calls == 1);
    REQUIRE(reason == widget::internal::DismissReason::pointer);

    class Focusable final: public scene::NanControl {
        [[nodiscard]] auto is_focusable() const -> bool override { return true; }
    };
    auto focusable = std::make_shared<Focusable>();
    content->add_child(focusable);
    tree.set_focus(focusable.get());
    tree.dispatch_key(scene::KeyEvent(256, scene::KeyEvent::Action::press));
    REQUIRE(calls == 2);
    REQUIRE(reason == widget::internal::DismissReason::escape);
}
