#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/internal/focus_scope.hpp>

#include <catch2/catch_test_macros.hpp>

#include <memory>

using namespace nandina;

namespace
{
    class FocusableControl final: public scene::NanControl {
    public:
        [[nodiscard]] auto is_focusable() const -> bool override {
            return true;
        }
    };
}

TEST_CASE("focus scope traps tab and restores previous focus", "[overlay][focus]") {
    auto root = std::make_shared<scene::NanControl>();
    auto outside = std::make_shared<FocusableControl>();
    root->add_child(outside);
    scene::NanSceneTree tree;
    tree.set_root(root);
    tree.set_focus(outside.get());

    auto scope = std::make_shared<widget::internal::FocusScope>();
    auto content = std::make_shared<scene::NanControl>();
    auto first = std::make_shared<FocusableControl>();
    auto second = std::make_shared<FocusableControl>();
    content->add_child(first);
    content->add_child(second);
    scope->set_content(content);
    root->add_child(scope);

    REQUIRE(tree.focused_node() == first.get());

    tree.dispatch_key(scene::KeyEvent(258, scene::KeyEvent::Action::press));
    REQUIRE(tree.focused_node() == second.get());
    tree.dispatch_key(scene::KeyEvent(258, scene::KeyEvent::Action::press));
    REQUIRE(tree.focused_node() == first.get());
    tree.dispatch_key(scene::KeyEvent(
        258,
        scene::KeyEvent::Action::press,
        {.shift = true}
    ));
    REQUIRE(tree.focused_node() == second.get());

    root->remove_and_delete(*scope);
    REQUIRE(tree.focused_node() == outside.get());
}

TEST_CASE("focus scope leaves focus unchanged without focusable descendants", "[overlay][focus]") {
    auto root = std::make_shared<scene::NanControl>();
    auto outside = std::make_shared<FocusableControl>();
    root->add_child(outside);
    scene::NanSceneTree tree;
    tree.set_root(root);
    tree.set_focus(outside.get());

    auto scope = std::make_shared<widget::internal::FocusScope>();
    scope->set_content(std::make_shared<scene::NanControl>());
    root->add_child(scope);

    REQUIRE(tree.focused_node() == outside.get());
}
