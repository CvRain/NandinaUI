// Exercise the shipped page, not a similar tree recreated in a test.
#include "../showcase/pages/anchors_page.hpp"

#include <nandina/scene/input_event.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/key_codes.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>

using namespace nandina;

namespace
{
    void activate(widget::Button& button) {
        scene::KeyEvent event(widget::keys::enter, scene::KeyEvent::Action::press);
        REQUIRE(button.on_input(event));
    }
    auto find_named(scene::NanNode& node, std::string_view name) -> scene::NanNode* {
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
        theme::ThemeManager themes;
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
        showcase::AnchorsPage page;
        widget::View root = page.build(context);
    };
} // namespace

TEST_CASE(
    "the anchors showcase resizes and switches its real page",
    "[anchors][showcase][resize]"
) {
    PageFixture fixture;
    auto* side_node = find_named(*fixture.root, "anchors-sidebar");
    auto* edit_node = find_named(*fixture.root, "anchors-editor");
    auto* toggle_node = find_named(*fixture.root, "anchors-toggle");
    auto* relation_node = find_named(*fixture.root, "anchors-relation");
    REQUIRE(side_node);
    REQUIRE(edit_node);
    REQUIRE(toggle_node);
    REQUIRE(relation_node);
    auto* side = side_node->as_control();
    auto* edit = edit_node->as_control();
    auto* toggle = static_cast<widget::Button*>(toggle_node);
    auto* relation = static_cast<widget::Label*>(relation_node);
    scene::NanSceneTree tree;
    tree.set_root(fixture.root);
    const auto* first_child = fixture.root->get_child(0);
    const std::array viewports {
        foundation::NanSize(800.0F, 500.0F),
        foundation::NanSize(200.0F, 120.0F),
        foundation::NanSize(0.0F, 0.0F),
        foundation::NanSize(800.0F, 500.0F)
    };
    for (const auto size: viewports) {
        REQUIRE_NOTHROW(tree.layout_root(size));
        REQUIRE(side->width() <= size.get_width() * 0.3F);
        REQUIRE(edit->width() >= 0.0F);
        REQUIRE(edit->height() >= 0.0F);
        for (int i = 0; i < 4; ++i) {
            const auto before = side->position();
            activate(*toggle);
            REQUIRE(side->position() == before); // Description, not geometry, was committed.
            REQUIRE_NOTHROW(tree.layout_root(size));
            const bool right = i % 2 == 0;
            REQUIRE(
                side->position().get_x()
                == Catch::Approx(right ? size.get_width() - side->width() : 0.0F)
            );
            REQUIRE(edit->position().get_x() == Catch::Approx(right ? 0.0F : side->width()));
            REQUIRE(
                relation->text()
                == (right ? "右边缘锚在侧边栏左边缘上" : "左边缘锚在侧边栏右边缘上")
            );
            REQUIRE(fixture.root->get_child(0) == first_child);
        }
    }
    // Restore fixed 220px + 12px gaps: the 200px/zero viewport throws on a negative span.
}

TEST_CASE(
    "a retained showcase button does not access an expired canvas",
    "[anchors][showcase][expired]"
) {
    PageFixture fixture;
    auto* raw = find_named(*fixture.root, "anchors-toggle");
    REQUIRE(raw);
    const auto retained = std::static_pointer_cast<widget::Button>(raw->shared_from_this());
    fixture.root.reset();
    // Scope is deliberately still alive: callback guard alone cannot protect expired refs.
    REQUIRE_NOTHROW(activate(*retained));
    fixture.scope.clear();
    REQUIRE_NOTHROW(activate(*retained));
    // Resolve side.lock() before checking canvas.lock(): the first activation throws "expired".
}
