#include <nandina/scene/scene_tree.hpp>
#include <nandina/scene/overlay_host.hpp>
#include <nandina/widget/internal/dismiss_layer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <utility>

using namespace nandina;

namespace
{
    class HitControl final: public scene::NanControl {
    public:
        explicit HitControl(foundation::NanSize size): scene::NanControl(size) {}

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override {
            return constraints.constrain(size());
        }
    };
}

TEST_CASE("overlay host lays out content and portal surface to the viewport", "[overlay][layout]") {
    auto host = scene::OverlayHost::create();
    auto content = std::make_shared<HitControl>(foundation::NanSize(40.0F, 30.0F));
    host->set_content(content);
    scene::NanSceneTree tree;
    tree.set_root(host);

    REQUIRE(tree.layout_root(foundation::NanSize(320.0F, 180.0F)) == 1);
    REQUIRE(content->size() == foundation::NanSize(320.0F, 180.0F));
    REQUIRE(host->layer_count() == 2);
    REQUIRE(host->layer_at(1)->layout_root()->size() == foundation::NanSize(320.0F, 180.0F));
}

TEST_CASE("presented content is hit above clipped application content", "[overlay][portal][input]") {
    auto host = scene::OverlayHost::create();
    auto content = std::make_shared<HitControl>(foundation::NanSize(100.0F, 100.0F));
    content->set_overflow(scene::ControlOverflow::clip);
    host->set_content(content);
    auto overlay = std::make_shared<HitControl>(foundation::NanSize(40.0F, 30.0F));
    overlay->set_position(foundation::NanPoint(140.0F, 20.0F));
    auto handle = host->present(overlay);
    scene::NanSceneTree tree;
    tree.set_root(host);
    (void)tree.layout_root(foundation::NanSize(240.0F, 120.0F));

    REQUIRE(tree.hit_test(foundation::NanPoint(150.0F, 25.0F)) == overlay.get());
    REQUIRE(handle.mounted());
    REQUIRE(host->overlay_count() == 1);

    handle.close();
    REQUIRE_FALSE(handle.mounted());
    REQUIRE(host->overlay_count() == 0);
    REQUIRE_FALSE(overlay->is_inside_tree());
    REQUIRE(tree.hit_test(foundation::NanPoint(150.0F, 25.0F)) == content.get());
}

TEST_CASE("overlay handles own presentation lifetime and preserve order", "[overlay][lifetime]") {
    auto host = scene::OverlayHost::create();
    host->set_content(std::make_shared<HitControl>(foundation::NanSize(200.0F, 100.0F)));
    auto modal = std::make_shared<HitControl>(foundation::NanSize(80.0F, 50.0F));
    auto lower = std::make_shared<HitControl>(foundation::NanSize(80.0F, 50.0F));
    auto upper = std::make_shared<HitControl>(foundation::NanSize(80.0F, 50.0F));
    // 模态层最先加入，仍必须压在后加入的 popup 之上：层级优先于加入顺序。
    auto modal_handle = host->present(modal, {.level = scene::OverlayLevel::modal});
    auto lower_handle = host->present(lower);
    auto upper_handle = host->present(upper);
    scene::NanSceneTree tree;
    tree.set_root(host);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    // 模态内部再开的浮层压过模态本身。
    auto nested = std::make_shared<HitControl>(foundation::NanSize(80.0F, 50.0F));
    auto nested_handle = host->present(nested, {.level = scene::OverlayLevel::nested_popup});

    REQUIRE(host->overlay_count() == 4);
    const auto probe = foundation::NanPoint(10.0F, 10.0F);
    REQUIRE(tree.hit_test(probe) == nested.get());

    nested_handle.close();
    REQUIRE(tree.hit_test(probe) == modal.get());

    // 同一层内按加入顺序叠放。
    modal_handle.close();
    REQUIRE(tree.hit_test(probe) == upper.get());

    auto moved = std::move(upper_handle);
    REQUIRE_FALSE(upper_handle.mounted());
    REQUIRE(moved.mounted());
    moved.close();
    REQUIRE(tree.hit_test(probe) == lower.get());
}

TEST_CASE("overlay host clears all overlays before device teardown", "[overlay][lifetime]") {
    auto host = scene::OverlayHost::create();
    host->set_content(std::make_shared<HitControl>(foundation::NanSize(200.0F, 100.0F)));
    auto first = std::make_shared<HitControl>(foundation::NanSize(40.0F, 30.0F));
    auto second = std::make_shared<HitControl>(foundation::NanSize(50.0F, 20.0F));
    auto first_handle = host->present(first);
    auto second_handle = host->present(second);

    REQUIRE(host->overlay_count() == 2);
    REQUIRE(first_handle.mounted());
    REQUIRE(second_handle.mounted());

    host->clear_overlays();

    REQUIRE(host->overlay_count() == 0);
    REQUIRE_FALSE(first_handle.mounted());
    REQUIRE_FALSE(second_handle.mounted());
    REQUIRE_FALSE(first->is_inside_tree());
    REQUIRE_FALSE(second->is_inside_tree());
}

TEST_CASE("overlay host rejects attached portal content", "[overlay][contract]") {
    auto host = scene::OverlayHost::create();
    auto parent = std::make_shared<scene::NanControl>();
    auto attached = std::make_shared<HitControl>(foundation::NanSize(20.0F, 20.0F));
    parent->add_child(attached);

    REQUIRE_THROWS_AS(host->present(attached), std::logic_error);
}

TEST_CASE("modal overlays block hit testing below the overlay layer", "[overlay][modal][input]") {
    auto host = scene::OverlayHost::create();
    auto content = std::make_shared<HitControl>(foundation::NanSize(200.0F, 100.0F));
    host->set_content(content);
    auto overlay = std::make_shared<HitControl>(foundation::NanSize(40.0F, 30.0F));
    auto handle = host->present(overlay, {.block_below = true});
    scene::NanSceneTree tree;
    tree.set_root(host);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    REQUIRE(tree.hit_test(foundation::NanPoint(180.0F, 90.0F)) == nullptr);
    handle.close();
    REQUIRE(tree.hit_test(foundation::NanPoint(180.0F, 90.0F)) == content.get());
}

TEST_CASE("dismiss layer receives clicks outside its content", "[overlay][dismiss][input]") {
    auto host = scene::OverlayHost::create();
    host->set_content(std::make_shared<HitControl>(foundation::NanSize(200.0F, 100.0F)));
    bool dismissed = false;
    auto content = std::make_shared<HitControl>(foundation::NanSize(40.0F, 30.0F));
    content->set_position(foundation::NanPoint(80.0F, 40.0F));
    auto dismiss = std::make_shared<widget::internal::DismissLayer>(
        [&dismissed](const auto) { dismissed = true; }
    );
    dismiss->set_content(content);
    auto handle = host->present(dismiss, {.block_below = true});
    scene::NanSceneTree tree;
    tree.set_root(host);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    REQUIRE(dismiss->size() == foundation::NanSize(200.0F, 100.0F));
    REQUIRE(content->global_bounds().contains_point(foundation::NanPoint(90.0F, 50.0F)));
    REQUIRE(tree.hit_test(foundation::NanPoint(10.0F, 10.0F)) == dismiss.get());
    REQUIRE(tree.hit_test(foundation::NanPoint(90.0F, 50.0F)) == content.get());
    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(10.0F, 10.0F),
    });
    REQUIRE(dismissed);
}

namespace
{
    /// Router-style page host: a plain control that only lays out NanControl
    /// children, so a nested OverlayHost (a LayerStack/NanNode2D) is skipped by
    /// ordinary layout. Mirrors NanRouter::PageHost.
    auto make_nested_host(
        scene::NanSceneTree& tree,
        const std::shared_ptr<scene::OverlayHost>& host
    ) -> std::shared_ptr<scene::NanControl> {
        auto page_host = std::make_shared<scene::NanControl>(foundation::NanSize(200.0F, 100.0F));
        page_host->add_child(host);
        tree.set_root(page_host);
        return page_host;
    }
} // namespace

TEST_CASE("nested overlay host lays out its layers against the viewport", "[overlay][nested][layout]") {
    auto host = scene::OverlayHost::create();
    auto content = std::make_shared<HitControl>(foundation::NanSize(40.0F, 30.0F));
    host->set_content(content);
    auto overlay = std::make_shared<HitControl>(foundation::NanSize(40.0F, 30.0F));
    overlay->set_position(foundation::NanPoint(150.0F, 60.0F));
    auto handle = host->present(overlay);

    scene::NanSceneTree tree;
    (void)make_nested_host(tree, host);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    // A LayerStack nested under a control root must still receive viewport layout.
    REQUIRE(content->size() == foundation::NanSize(200.0F, 100.0F));
    REQUIRE(host->layer_at(1)->layout_root()->size() == foundation::NanSize(200.0F, 100.0F));
    REQUIRE(tree.hit_test(foundation::NanPoint(160.0F, 70.0F)) == overlay.get());
}

TEST_CASE("nested modal overlay blocks hit testing below the overlay layer", "[overlay][nested][modal]") {
    auto host = scene::OverlayHost::create();
    auto content = std::make_shared<HitControl>(foundation::NanSize(200.0F, 100.0F));
    host->set_content(content);
    auto overlay = std::make_shared<HitControl>(foundation::NanSize(40.0F, 30.0F));
    auto handle = host->present(overlay, {.block_below = true});

    scene::NanSceneTree tree;
    (void)make_nested_host(tree, host);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    // The overlay surface does not cover this point, but the blocking layer must
    // still swallow the hit instead of falling through to application content.
    REQUIRE(tree.hit_test(foundation::NanPoint(180.0F, 90.0F)) == nullptr);
    handle.close();
    REQUIRE(tree.hit_test(foundation::NanPoint(180.0F, 90.0F)) == content.get());
}

TEST_CASE("nested dismiss layer receives clicks outside its content", "[overlay][nested][dismiss]") {
    auto host = scene::OverlayHost::create();
    auto content = std::make_shared<HitControl>(foundation::NanSize(200.0F, 100.0F));
    host->set_content(content);
    bool dismissed = false;
    auto panel = std::make_shared<HitControl>(foundation::NanSize(40.0F, 30.0F));
    panel->set_position(foundation::NanPoint(80.0F, 40.0F));
    auto dismiss = std::make_shared<widget::internal::DismissLayer>(
        [&dismissed](const auto) { dismissed = true; }
    );
    dismiss->set_content(panel);
    auto handle = host->present(dismiss, {.block_below = true});

    scene::NanSceneTree tree;
    (void)make_nested_host(tree, host);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    REQUIRE(dismiss->size() == foundation::NanSize(200.0F, 100.0F));
    REQUIRE(tree.hit_test(foundation::NanPoint(10.0F, 10.0F)) == dismiss.get());
    REQUIRE(tree.hit_test(foundation::NanPoint(90.0F, 50.0F)) == panel.get());

    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(90.0F, 50.0F),
    });
    REQUIRE_FALSE(dismissed);

    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(10.0F, 10.0F),
    });
    REQUIRE(dismissed);
}

TEST_CASE("closing a parent overlay closes its descendants first", "[overlay][nested]") {
    auto host = scene::OverlayHost::create();
    auto parent = std::make_shared<HitControl>(foundation::NanSize(40.0F, 30.0F));
    auto child = std::make_shared<HitControl>(foundation::NanSize(20.0F, 10.0F));
    auto grandchild = std::make_shared<HitControl>(foundation::NanSize(10.0F, 10.0F));

    auto parent_handle = host->present(parent);
    auto child_handle = host->present(
        child,
        {.level = scene::OverlayLevel::nested_popup, .parent = parent_handle.id()}
    );
    auto grandchild_handle = host->present(
        grandchild,
        {.level = scene::OverlayLevel::nested_popup, .parent = child_handle.id()}
    );

    REQUIRE(host->overlay_count() == 3);
    REQUIRE(host->overlay_parent(child_handle.id()) == parent_handle.id());
    REQUIRE(host->overlay_parent(grandchild_handle.id()) == child_handle.id());
    REQUIRE(host->overlay_child_count(parent_handle.id()) == 1);

    // 关闭父层：整棵子树都要消失，且后代带 parent 原因。
    parent_handle.close();
    REQUIRE(host->overlay_count() == 0);
    REQUIRE_FALSE(child_handle.mounted());
    REQUIRE_FALSE(grandchild_handle.mounted());
    REQUIRE(child_handle.close_reason() == scene::OverlayCloseReason::parent);
    REQUIRE(grandchild_handle.close_reason() == scene::OverlayCloseReason::parent);
    REQUIRE(parent_handle.close_reason() == scene::OverlayCloseReason::owner);
}

TEST_CASE("closing one overlay leaves its siblings mounted", "[overlay][nested]") {
    auto host = scene::OverlayHost::create();
    auto root = std::make_shared<HitControl>(foundation::NanSize(40.0F, 30.0F));
    auto first = std::make_shared<HitControl>(foundation::NanSize(20.0F, 10.0F));
    auto second = std::make_shared<HitControl>(foundation::NanSize(20.0F, 10.0F));

    auto root_handle = host->present(root);
    auto first_handle = host->present(first, {.parent = root_handle.id()});
    auto second_handle = host->present(second, {.parent = root_handle.id()});
    REQUIRE(host->overlay_child_count(root_handle.id()) == 2);

    // 关掉一个兄弟不影响另一个，也不影响父层。
    first_handle.close();
    REQUIRE_FALSE(first_handle.mounted());
    REQUIRE(second_handle.mounted());
    REQUIRE(root_handle.mounted());
    REQUIRE(host->overlay_count() == 2);
    REQUIRE(host->overlay_child_count(root_handle.id()) == 1);
}

TEST_CASE("an unrelated overlay survives another tree closing", "[overlay][nested]") {
    auto host = scene::OverlayHost::create();
    auto tree_a = std::make_shared<HitControl>(foundation::NanSize(40.0F, 30.0F));
    auto child_a = std::make_shared<HitControl>(foundation::NanSize(20.0F, 10.0F));
    auto tree_b = std::make_shared<HitControl>(foundation::NanSize(40.0F, 30.0F));
    auto child_b = std::make_shared<HitControl>(foundation::NanSize(20.0F, 10.0F));

    auto a = host->present(tree_a);
    auto a_child = host->present(child_a, {.parent = a.id()});
    auto b = host->present(tree_b);
    auto b_child = host->present(child_b, {.parent = b.id()});

    a.close();

    // 另一棵树必须完整存活 —— 血缘关系不能靠"关闭所有子层"来实现。
    REQUIRE_FALSE(a.mounted());
    REQUIRE_FALSE(a_child.mounted());
    REQUIRE(b.mounted());
    REQUIRE(b_child.mounted());
    REQUIRE(host->overlay_count() == 2);
}

TEST_CASE("a closed parent handle cannot adopt new children", "[overlay][nested]") {
    auto host = scene::OverlayHost::create();
    auto parent = std::make_shared<HitControl>(foundation::NanSize(40.0F, 30.0F));
    auto orphan = std::make_shared<HitControl>(foundation::NanSize(20.0F, 10.0F));

    auto parent_handle = host->present(parent);
    parent_handle.close();
    REQUIRE_FALSE(parent_handle.mounted());

    // 父层已不存在：拒绝挂子层，而不是制造一个永远收不掉的孤儿。
    REQUIRE_THROWS_AS(
        host->present(orphan, {.parent = parent_handle.id()}),
        std::invalid_argument
    );
    REQUIRE(host->overlay_count() == 0);
}

TEST_CASE("host teardown closes nested overlays with the teardown reason", "[overlay][nested]") {
    auto host = scene::OverlayHost::create();
    auto parent = std::make_shared<HitControl>(foundation::NanSize(40.0F, 30.0F));
    auto child = std::make_shared<HitControl>(foundation::NanSize(20.0F, 10.0F));

    auto parent_handle = host->present(parent);
    auto child_handle = host->present(child, {.parent = parent_handle.id()});

    host->clear_overlays();

    REQUIRE(host->overlay_count() == 0);
    REQUIRE_FALSE(parent_handle.mounted());
    REQUIRE_FALSE(child_handle.mounted());
    REQUIRE(parent_handle.close_reason() == scene::OverlayCloseReason::host_teardown);
    REQUIRE(child_handle.close_reason() == scene::OverlayCloseReason::host_teardown);
}
