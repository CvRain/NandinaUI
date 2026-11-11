#include "theme_test_support.hpp"
// Exercise the maintained Label page and its real metadata table across widths.
#include "../showcase/components/component_info_table.hpp"
#include "../showcase/pages/label_page.hpp"

#include <nandina/scene/scene_tree.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>
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

    void require_settled(scene::NanSceneTree& tree, foundation::NanSize viewport) {
        REQUIRE(tree.layout_root(viewport) >= 1);
        REQUIRE(tree.layout_root(viewport) == 0);
        REQUIRE(tree.layout_root(viewport) == 0);
    }
} // namespace

TEST_CASE(
    "the maintained Label page reflows and keeps its metadata table",
    "[label][showcase][resize]"
) {
    Fixture fixture;
    showcase::LabelPage page;
    auto root = page.build(fixture.context);
    auto* paragraph = find_text(*root, "父级给出有限宽度时");
    auto* location = find_text(*root, "nandina/widget/label.hpp");
    REQUIRE(paragraph);
    REQUIRE(location);
    fixture.tree.set_root(root);
    require_settled(fixture.tree, foundation::NanSize(1200, 800));
    const auto wide_width = paragraph->width();
    const auto wide_height = paragraph->height();
    require_settled(fixture.tree, foundation::NanSize(640, 800));
    CHECK(paragraph->width() < wide_width);
    CHECK(paragraph->height() > wide_height);
    CHECK(location->height() > 0);
    CHECK(std::isfinite(location->height()));
    require_settled(fixture.tree, foundation::NanSize(1200, 800));
    CHECK(paragraph->width() == Catch::Approx(wide_width));
    CHECK(paragraph->height() == Catch::Approx(wide_height));
    CHECK(find_text(*root, "nandina/widget/label.hpp") == location);
}

TEST_CASE(
    "ComponentInfoTable wraps real long values within its assigned width",
    "[label][showcase][metadata]"
) {
    Fixture fixture;
    const std::string path =
        "nandina/widget/an_extended_component_reference_for_layout_acceptance.hpp";
    auto table =
        showcase::ComponentInfoTable(fixture.context)
            .set_file_location(path)
            .set_inherits("primitives::Text")
            .set_tips(
                {"Metadata follows the same finite content width as labels elsewhere in the page."}
            )
            .build()
            .build();
    auto* value = find_text(*table, path);
    REQUIRE(value);
    fixture.tree.set_root(table);
    require_settled(fixture.tree, foundation::NanSize(700, 300));
    const auto wide_height = value->height();
    const auto wide_width = value->width();
    require_settled(fixture.tree, foundation::NanSize(340, 300));
    CHECK(value->width() < wide_width);
    CHECK(value->height() > wide_height);
    CHECK(value->global_bounds().get_right() <= table->global_bounds().get_right());
    CHECK(value->resolved_semantics_properties().label == path);
}
