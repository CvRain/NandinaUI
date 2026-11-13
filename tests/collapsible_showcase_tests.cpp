#include "theme_test_support.hpp"
// Exercise the maintained Collapsible page: sections build, nested instances exist, layout settles.
#include "../showcase/pages/collapsible_page.hpp"

#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/collapsible.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string_view>

using namespace nandina;

namespace
{
    struct Fixture {
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
        scene::NanSceneTree tree;
    };

    auto find_text(scene::NanNode& node, std::string_view prefix) -> scene::NanControl* {
        const auto properties = node.resolved_semantics_properties();
        if (properties.role == semantics::Role::static_text && properties.label.starts_with(prefix))
        {
            return node.as_control();
        }
        for (std::size_t i = 0; i < node.child_count(); ++i) {
            if (auto* found = find_text(*node.get_child(i), prefix))
                return found;
        }
        return nullptr;
    }

    auto find_named(scene::NanNode& node, std::string_view name) -> scene::NanNode* {
        if (node.name() == name) {
            return &node;
        }
        for (std::size_t i = 0; i < node.child_count(); ++i) {
            if (auto* found = find_named(*node.get_child(i), name))
                return found;
        }
        return nullptr;
    }

    auto count_role(scene::NanNode& node, semantics::Role role) -> std::size_t {
        std::size_t total = node.resolved_semantics_properties().role == role ? 1U : 0U;
        for (std::size_t i = 0; i < node.child_count(); ++i) {
            total += count_role(*node.get_child(i), role);
        }
        return total;
    }

    void require_settled(scene::NanSceneTree& tree, foundation::NanSize viewport) {
        REQUIRE(tree.layout_root(viewport) >= 1);
        REQUIRE(tree.layout_root(viewport) == 0);
        REQUIRE(tree.layout_root(viewport) == 0);
    }
} // namespace

TEST_CASE(
    "the maintained Collapsible page builds its sections and nested instances",
    "[collapsible][showcase]"
) {
    Fixture fixture;
    showcase::CollapsiblePage page;
    auto root = page.build(fixture.context);
    fixture.tree.set_root(root);

    require_settled(fixture.tree, foundation::NanSize(1200, 800));

    REQUIRE(find_text(*root, "折叠内容（Collapsible）"));
    REQUIRE(find_text(*root, "收起与展开"));
    REQUIRE(find_text(*root, "嵌套"));
    REQUIRE(find_text(*root, "Signal 与禁用"));

    // Five instances: closed, open, inner, outer, signal trigger, disabled -> six disclosure
    // buttons in total (outer + inner + the four standalone sections above).
    REQUIRE(count_role(*root, semantics::Role::button) >= 6U);

    require_settled(fixture.tree, foundation::NanSize(720, 800));
}

TEST_CASE(
    "the maintained Collapsible page keeps expanded content inside the disclosure",
    "[collapsible][showcase][layout]"
) {
    Fixture fixture;
    showcase::CollapsiblePage page;
    auto root = page.build(fixture.context);
    fixture.tree.set_root(root);

    const auto require_open_content_inside = [&root](const std::string_view node_name) {
        auto* node = find_named(*root, node_name);
        REQUIRE(node != nullptr);
        auto* collapsible = static_cast<widget::Collapsible*>(node->as_control());
        REQUIRE(collapsible != nullptr);
        REQUIRE(collapsible->open());
        auto* content = collapsible->child();
        REQUIRE(content != nullptr);
        REQUIRE(content->visible());
        // The disclosure must reserve the height its expanded content needs; otherwise the
        // content paints past the box (and past the surrounding example Card).
        CHECK(
            content->global_bounds().get_bottom()
            <= collapsible->global_bounds().get_bottom() + foundation::nan_epsilon
        );
    };

    require_settled(fixture.tree, foundation::NanSize(1200, 800));
    require_open_content_inside("collapsible-open");

    // 900 px is where the right column text rewraps and exposes the stale flex height.
    require_settled(fixture.tree, foundation::NanSize(900, 800));
    require_open_content_inside("collapsible-open");

    // A user toggle must also reserve the newly visible content.
    auto* signal_node = find_named(*root, "collapsible-signal");
    REQUIRE(signal_node != nullptr);
    auto* signal_collapsible = static_cast<widget::Collapsible*>(signal_node->as_control());
    REQUIRE(signal_collapsible != nullptr);
    REQUIRE_FALSE(signal_collapsible->open());
    signal_collapsible->toggle();
    require_settled(fixture.tree, foundation::NanSize(900, 800));
    REQUIRE(signal_collapsible->open());
    auto* signal_content = signal_collapsible->child();
    REQUIRE(signal_content != nullptr);
    REQUIRE(signal_content->visible());
    CHECK(
        signal_content->global_bounds().get_bottom()
        <= signal_collapsible->global_bounds().get_bottom() + foundation::nan_epsilon
    );

    // The disabled disclosure keeps its slot empty and ignores user toggles: nothing changes,
    // so the tree must not request another layout pass.
    auto* disabled_node = find_named(*root, "collapsible-disabled");
    REQUIRE(disabled_node != nullptr);
    auto* disabled = static_cast<widget::Collapsible*>(disabled_node->as_control());
    REQUIRE(disabled != nullptr);
    REQUIRE(disabled->disabled());
    REQUIRE_FALSE(disabled->open());
    disabled->toggle();
    REQUIRE(fixture.tree.layout_root(foundation::NanSize(900, 800)) == 0);
    REQUIRE_FALSE(disabled->open());
    REQUIRE(disabled->child() == nullptr);
}

TEST_CASE(
    "the maintained Collapsible page keeps a bounded scroll root at narrow widths",
    "[collapsible][showcase][resize]"
) {
    Fixture fixture;
    showcase::CollapsiblePage page;
    auto root = page.build(fixture.context);
    fixture.tree.set_root(root);
    auto* root_control = root->as_control();
    REQUIRE(root_control != nullptr);

    require_settled(fixture.tree, foundation::NanSize(1200, 800));
    REQUIRE(root_control->width() <= 1200.0F + foundation::nan_epsilon);
    REQUIRE(root_control->height() <= 800.0F + foundation::nan_epsilon);

    require_settled(fixture.tree, foundation::NanSize(640, 800));
    REQUIRE(root_control->width() <= 640.0F + foundation::nan_epsilon);
    REQUIRE(root_control->height() <= 800.0F + foundation::nan_epsilon);
}
