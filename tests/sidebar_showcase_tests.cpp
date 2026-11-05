// Exercise real Sidebar grouping, separators and navigation through public semantics.
#include "../showcase/components/sidebar.hpp"

#include <nandina/scene/scene_tree.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace app = nandina::app;
namespace foundation = nandina::foundation;
namespace reactive = nandina::reactive;
namespace scene = nandina::scene;
namespace semantics = nandina::semantics;
namespace showcase = nandina::showcase;
namespace theme = nandina::theme;
namespace widget = nandina::widget;

namespace
{
    template<int Index>
    class TestPage final: public app::Page<> {
    public:
        auto build(app::PageContext&) -> widget::View override {
            return std::make_shared<scene::NanControl>();
        }
    };

    void collect(const semantics::Node& node, std::vector<const semantics::Node*>& nodes) {
        const auto role = node.properties.role;
        if (role == semantics::Role::switch_control) {
            return;
        }
        if (role == semantics::Role::static_text || role == semantics::Role::button
            || role == semantics::Role::separator)
        {
            nodes.push_back(&node);
        }
        if (role == semantics::Role::button) {
            return;
        }
        for (const auto& child: node.children) {
            collect(child, nodes);
        }
    }

    struct Fixture {
        reactive::Graph graph;
        reactive::ReactiveScope scope {graph};
        theme::ThemeManager themes;
        app::NanRouter router {graph, themes};
        scene::NanSceneTree tree;
        widget::View root;

        explicit Fixture(const app::Routes& routes) {
            REQUIRE(router.configure(routes));
            const app::ShellContext context {
                graph,
                scope,
                themes,
                nullptr,
                nullptr,
                nullptr,
                router.navigation(),
                router.outlet(),
                router.routes(),
                router.current_page()
            };
            root = showcase::Sidebar(context).build_shell();
            tree.set_root(root);
            (void)tree.layout_root(foundation::NanSize(1200, 900));
            REQUIRE(tree.update_semantics());
        }

        auto navigation_nodes() const -> std::vector<const semantics::Node*> {
            std::vector<const semantics::Node*> nodes;
            for (const auto& root_node: tree.semantics_tree().roots) {
                collect(root_node, nodes);
            }
            // The sidebar brackets navigation with separators above and below it.
            const auto separator = [](const semantics::Node* node) {
                return node->properties.role == semantics::Role::separator;
            };
            const auto first = std::ranges::find_if(nodes, separator);
            const auto last = std::find_if(nodes.rbegin(), nodes.rend(), separator);
            REQUIRE(first != nodes.end());
            REQUIRE(last != nodes.rend());
            REQUIRE(first < last.base() - 1);
            return {first + 1, last.base() - 1};
        }

        auto navigation_items() const -> std::vector<std::string> {
            std::vector<std::string> result;
            for (const auto* node: navigation_nodes()) {
                switch (node->properties.role) {
                    case semantics::Role::separator:
                        result.emplace_back("|");
                        break;
                    case semantics::Role::static_text:
                        result.push_back("heading:" + node->properties.label);
                        break;
                    case semantics::Role::button:
                        result.push_back("item:" + node->properties.label);
                        break;
                    default:
                        break;
                }
            }
            return result;
        }
    };
} // namespace

TEST_CASE("sidebar merges ungrouped routes without an empty heading", "[showcase][sidebar][nav]") {
    Fixture fixture(
        app::Routes {
            app::route<TestPage<0>>({.title = "First"}),
            app::route<TestPage<1>>({.title = "Named", .nav_group = "Tools"}),
            app::route<TestPage<2>>({.title = "Second", .nav_group = ""}),
            app::route<TestPage<3>>({.title = "Named again", .nav_group = "Tools"}),
            app::route<TestPage<4>>(
                {.title = "Hidden", .nav_group = "Hidden", .show_in_nav = false}
            ),
        }
    );
    CHECK(
        fixture.navigation_items()
        == std::vector<std::string> {
            "item:First",
            "item:Second",
            "|",
            "heading:Tools",
            "item:Named",
            "item:Named again"
        }
    );

    // Identical group values never replace the typed page key used by activation.
    for (const auto* node: fixture.navigation_nodes()) {
        if (node->properties.role == semantics::Role::button && node->properties.label == "Second")
        {
            REQUIRE(fixture.tree.perform_semantics_action(node->id, {semantics::Action::activate}));
            CHECK(fixture.router.current_page_key() == app::page_key<TestPage<2>>());
            REQUIRE(fixture.router.navigation().navigate<TestPage<1>>());
            CHECK(fixture.router.current_page_key() == app::page_key<TestPage<1>>());
            return;
        }
    }
    FAIL("Ungrouped route has no accessible navigation button");
}

TEST_CASE(
    "sidebar keeps separators on both sides of an ungrouped section",
    "[showcase][sidebar][nav]"
) {
    Fixture fixture(
        app::Routes {
            app::route<TestPage<0>>({.title = "Before", .nav_group = "A"}),
            app::route<TestPage<1>>({.title = "Middle"}),
            app::route<TestPage<2>>({.title = "After", .nav_group = "default"}),
            app::route<TestPage<3>>({.title = "Middle again", .nav_group = ""}),
        }
    );
    CHECK(
        fixture.navigation_items()
        == std::vector<std::string> {
            "heading:A",
            "item:Before",
            "|",
            "item:Middle",
            "item:Middle again",
            "|",
            "heading:default",
            "item:After"
        }
    );
}

TEST_CASE(
    "sidebar adds no section separators for a single ungrouped section",
    "[showcase][sidebar][nav]"
) {
    Fixture fixture(
        app::Routes {
            app::route<TestPage<0>>({.title = "First"}),
            app::route<TestPage<1>>({.title = "Second", .nav_group = ""}),
        }
    );
    CHECK(fixture.navigation_items() == std::vector<std::string> {"item:First", "item:Second"});
}

TEST_CASE("sidebar adds no empty section for hidden routes", "[showcase][sidebar][nav]") {
    Fixture fixture(
        app::Routes {
            app::route<TestPage<0>>({.title = "Hidden", .show_in_nav = false}),
        }
    );
    CHECK(fixture.navigation_items().empty());
}
