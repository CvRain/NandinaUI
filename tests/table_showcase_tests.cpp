#include "theme_test_support.hpp"
// Exercise the maintained Table page: sections build, layout converges, and table semantics exist.
#include "../showcase/pages/table_page.hpp"

#include <nandina/scene/scene_tree.hpp>

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
    "the maintained Table page builds its sections and keeps table semantics",
    "[table][showcase]"
) {
    Fixture fixture;
    showcase::TablePage page;
    auto root = page.build(fixture.context);
    fixture.tree.set_root(root);

    require_settled(fixture.tree, foundation::NanSize(1200, 800));

    REQUIRE(find_text(*root, "表格（Table）"));
    REQUIRE(find_text(*root, "列轨道"));
    REQUIRE(find_text(*root, "有限宽度与换行"));
    REQUIRE(find_text(*root, "空表"));
    REQUIRE(find_text(*root, "行数与自然行高"));

    // Four tables on the page: wide tracks, narrow tracks, empty, and natural-row-height.
    REQUIRE(count_role(*root, semantics::Role::table) == 4U);
    // Each table has three columns; the header cells expose column_header, data rows expose cell.
    REQUIRE(count_role(*root, semantics::Role::column_header) == 12U);
    REQUIRE(count_role(*root, semantics::Role::cell) == 27U);

    require_settled(fixture.tree, foundation::NanSize(640, 800));
}

TEST_CASE(
    "the maintained Table page keeps a bounded scroll root at narrow widths",
    "[table][showcase][resize]"
) {
    Fixture fixture;
    showcase::TablePage page;
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
