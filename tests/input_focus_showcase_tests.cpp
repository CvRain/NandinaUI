#include "theme_test_support.hpp"
//
// input_focus_showcase_tests — 验证随仓库发布的 D1 输入与焦点验收页能真实构建与布局。
//

#include "../showcase/pages/input_focus_page.hpp"

#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/checkbox.hpp>
#include <nandina/widget/dialog.hpp>
#include <nandina/widget/dropdown_menu.hpp>
#include <nandina/widget/tabs.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string_view>

using namespace nandina;

namespace
{
    auto find_named(scene::NanNode& node, const std::string_view name) -> scene::NanNode* {
        if (node.name() == name) {
            return &node;
        }
        for (std::size_t i = 0; i < node.child_count(); ++i) {
            if (auto* found = find_named(*node.get_child(i), name)) {
                return found;
            }
        }
        return nullptr;
    }

    struct PageFixture {
        reactive::Graph graph;
        reactive::ReactiveScope scope {graph};
        theme_test::Manager themes;
        app::PageContext context {
            graph,
            scope,
            themes.theme(),
            nullptr,
            {},
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            &themes
        };
        showcase::InputFocusPage page;
        widget::View root = page.build(context);
    };
} // namespace

TEST_CASE(
    "the input and focus showcase builds and lays out its real page",
    "[showcase][focus][hint]"
) {
    PageFixture fixture;
    auto* preference_node = find_named(*fixture.root, "focus-preference");
    auto* tabs_node = find_named(*fixture.root, "focus-tabs");
    auto* menu_node = find_named(*fixture.root, "focus-menu");
    auto* dialog_node = find_named(*fixture.root, "focus-dialog");
    REQUIRE(preference_node != nullptr);
    REQUIRE(tabs_node != nullptr);
    REQUIRE(menu_node != nullptr);
    REQUIRE(dialog_node != nullptr);

    auto* preference = static_cast<widget::Checkbox*>(preference_node);
    auto* dialog = static_cast<widget::Dialog*>(dialog_node);

    scene::NanSceneTree tree;
    tree.set_root(fixture.root);
    REQUIRE(tree.layout_root(foundation::NanSize(1000.0F, 700.0F)) >= 1);
    REQUIRE_FALSE(tree.focus_hint_always_visible());

    // The visible preference switch is wired to the scene policy seam.
    tree.set_focus(preference);
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::enter, scene::KeyEvent::Action::press)
    );
    REQUIRE(tree.focus_hint_always_visible());

    // Dialog entry/exit remains usable inside the page tree (no overlay host in this fixture).
    REQUIRE_NOTHROW(dialog->open());
    REQUIRE(dialog->is_open());
    REQUIRE_NOTHROW(tree.layout_root(foundation::NanSize(1000.0F, 700.0F)));
    tree.dispatch_key(
        scene::KeyEvent(foundation::LogicalKey::escape, scene::KeyEvent::Action::press)
    );
    REQUIRE_FALSE(dialog->is_open());
}
