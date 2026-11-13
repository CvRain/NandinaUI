#include "theme_test_support.hpp"
// Exercise the maintained Surface/Card page: inset variants, box styles and nested boundary.
#include "../showcase/pages/surface_page.hpp"

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

    auto find_named(scene::NanNode& node, std::string_view name) -> scene::NanControl* {
        if (node.name() == name)
            return node.as_control();
        for (std::size_t i = 0; i < node.child_count(); ++i) {
            if (auto* found = find_named(*node.get_child(i), name))
                return found;
        }
        return nullptr;
    }

    void require_settled(scene::NanSceneTree& tree, foundation::NanSize viewport) {
        REQUIRE(tree.layout_root(viewport) >= 1);
        REQUIRE(tree.layout_root(viewport) == 0);
        REQUIRE(tree.layout_root(viewport) == 0);
    }
} // namespace

TEST_CASE(
    "the maintained Surface page builds inset variants and its nested card",
    "[surface][showcase]"
) {
    Fixture fixture;
    showcase::SurfacePage page;
    auto root = page.build(fixture.context);
    fixture.tree.set_root(root);

    require_settled(fixture.tree, foundation::NanSize(1200, 800));

    REQUIRE(find_text(*root, "表面与卡片（Surface / Card）"));
    REQUIRE(find_text(*root, "卡片内容边距"));
    REQUIRE(find_text(*root, "Surface 盒样式"));
    REQUIRE(find_text(*root, "嵌套与圆角边界"));

    REQUIRE(find_named(*root, "card-inset-default"));
    REQUIRE(find_named(*root, "card-inset-8"));
    REQUIRE(find_named(*root, "card-inset-24"));
    REQUIRE(find_named(*root, "card-nested"));

    require_settled(fixture.tree, foundation::NanSize(640, 800));
}

TEST_CASE(
    "the maintained Surface page keeps a bounded scroll root at narrow widths",
    "[surface][showcase][resize]"
) {
    Fixture fixture;
    showcase::SurfacePage page;
    auto root = page.build(fixture.context);
    fixture.tree.set_root(root);
    auto* root_control = root->as_control();
    REQUIRE(root_control != nullptr);

    require_settled(fixture.tree, foundation::NanSize(1200, 800));
    REQUIRE(root_control->width() <= 1200.0F + foundation::nan_epsilon);
    REQUIRE(root_control->height() <= 800.0F + foundation::nan_epsilon);

    require_settled(fixture.tree, foundation::NanSize(520, 800));
    REQUIRE(root_control->width() <= 520.0F + foundation::nan_epsilon);
    REQUIRE(root_control->height() <= 800.0F + foundation::nan_epsilon);
}
