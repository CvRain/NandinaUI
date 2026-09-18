//
// drag_controller_tests — 拖动一个已挂载节点到另一个容器。
//
// 覆盖：命中测试选落点、拒绝落到自己/后代、按几何解析插入位置、提交换父、取消不改树。
// 这些是「把按钮从 ListView 拖进 Grid」这类编辑器/游戏 UI 交互的基础。
//

#include <nandina/foundation/geometry.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/drag_controller.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/layout.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <memory>
#include <vector>

using namespace nandina;

namespace
{
    /// 固定尺寸的可容纳容器（显式声明接受拖放：accepts_drop 默认 false）。
    [[nodiscard]] auto make_box(float width, float height) -> std::shared_ptr<widget::Column> {
        auto box = widget::Column::create();
        box->set_width(width);
        box->set_height(height);
        box->set_gap(0.0F);
        box->set_accepts_drop(true);
        return box;
    }

    [[nodiscard]] auto make_item(float width, float height) -> std::shared_ptr<widget::Column> {
        auto item = widget::Column::create();
        item->set_width(width);
        item->set_height(height);
        return item;
    }

    /// 记录指针经过的"幽灵"节点（仅用于断言服务会跟随指针）。
    struct Fixture {
        std::shared_ptr<widget::Column> source;
        std::shared_ptr<widget::Column> target;
        std::shared_ptr<widget::Column> item_a;
        std::shared_ptr<widget::Column> item_b;
        std::shared_ptr<widget::Column> dragged;
        std::shared_ptr<widget::Column> root;
        scene::NanSceneTree tree;
        widget::DragController controller;

        Fixture() {
            // 源容器（左）与目标容器（右），并排放在根行里。
            source = make_box(200.0F, 200.0F);
            target = make_box(200.0F, 200.0F);

            item_a = make_item(200.0F, 60.0F);
            item_b = make_item(200.0F, 60.0F);
            dragged = make_item(200.0F, 60.0F);
            source->add(item_a);
            source->add(item_b);
            source->add(dragged);

            // 目标容器两个条目（高 120），容器高 200：底部 80px 是容器自身的空白，
            // 用于验证「指针在容器内但不在任何条目上」时仍解析到容器。
            target->add(make_item(200.0F, 60.0F));
            target->add(make_item(200.0F, 60.0F));

            root = widget::Column::create();
            auto row = widget::Row::create();
            row->set_gap(0.0F);
            row->add(source);
            row->add(target);
            root->add(row);

            tree.set_root(root);
            tree.layout_root(foundation::NanSize(400.0F, 200.0F));
        }

        /// 目标容器内、某个条目中心线上的一个点（用于命中测试）。
        [[nodiscard]] auto point_in_target(float dy) const -> foundation::NanPoint {
            const auto bounds = target->global_bounds();
            return foundation::NanPoint {
                bounds.get_left() + bounds.get_width() * 0.5F,
                bounds.get_top() + dy,
            };
        }
    };
} // namespace

TEST_CASE("drag resolves the drop target under the pointer", "[widget][drag]") {
    Fixture fixture;
    fixture.controller.install(fixture.tree, nullptr);

    REQUIRE(fixture.controller.start(fixture.dragged));
    REQUIRE(fixture.controller.active());

    // 指针在源容器里、但落在某个条目之外的空白处：落点应解析到源容器自身。
    {
        const auto bounds = fixture.source->global_bounds();
        fixture.controller.update(
            foundation::NanPoint {
                bounds.get_left() + bounds.get_width() * 0.5F,
                bounds.get_bottom() - 2.0F,
            }
        );
        REQUIRE(fixture.controller.drop_target() == fixture.source.get());
    }

    // 指针移到目标容器：落点跟随。
    fixture.controller.update(fixture.point_in_target(190.0F));
    REQUIRE(fixture.controller.drop_target() == fixture.target.get());

    fixture.controller.cancel();
    REQUIRE_FALSE(fixture.controller.active());
}

TEST_CASE("drag refuses to drop onto itself or its own descendants", "[widget][drag]") {
    Fixture fixture;
    // 让被拖节点自己带一个子节点，验证"后代也不是合法落点"。
    auto child = make_item(50.0F, 20.0F);
    fixture.dragged->add(child);
    fixture.tree.layout_root(foundation::NanSize(400.0F, 200.0F));

    fixture.controller.install(fixture.tree, nullptr);
    REQUIRE(fixture.controller.start(fixture.dragged));

    // 指针压在被拖节点自身的中心：不允许解析到它或它的后代。
    const auto dragged_bounds = fixture.dragged->global_bounds();
    fixture.controller.update(
        foundation::NanPoint {
            dragged_bounds.get_left() + dragged_bounds.get_width() * 0.5F,
            dragged_bounds.get_top() + dragged_bounds.get_height() * 0.5F,
        }
    );
    REQUIRE(fixture.controller.drop_target() != fixture.dragged.get());
    REQUIRE(fixture.controller.drop_target() != child.get());
}

TEST_CASE("drag commit reparents the node at the resolved slot", "[widget][drag]") {
    Fixture fixture;
    fixture.controller.install(fixture.tree, nullptr);
    Fixture& f = fixture;

    const auto before_source = f.source->child_count();
    const auto before_target = f.target->child_count();
    REQUIRE(before_source == 3);
    REQUIRE(before_target == 2);

    REQUIRE(f.controller.start(f.dragged));
    // 指针落在目标容器底部的空白处（前两个条目之后）：应插到末尾（index 2）。
    f.controller.update(f.point_in_target(190.0F));
    REQUIRE(f.controller.commit());

    REQUIRE_FALSE(f.controller.active());
    REQUIRE(f.target->child_count() == before_target + 1);
    REQUIRE(f.source->child_count() == before_source - 1);
    REQUIRE(f.dragged->parent() == f.target.get());
    REQUIRE(f.target->get_child(2) == f.dragged.get());
}

TEST_CASE("drag cancel leaves the tree untouched", "[widget][drag]") {
    Fixture fixture;
    fixture.controller.install(fixture.tree, nullptr);

    REQUIRE(fixture.controller.start(fixture.dragged));
    fixture.controller.update(fixture.point_in_target(190.0F));
    REQUIRE(fixture.controller.drop_target() == fixture.target.get());

    fixture.controller.cancel();

    REQUIRE_FALSE(fixture.controller.active());
    REQUIRE(fixture.dragged->parent() == fixture.source.get());
    REQUIRE(fixture.source->child_count() == 3);
    REQUIRE(fixture.target->child_count() == 2);
}

TEST_CASE("drag commit without a drop target is a no-op", "[widget][drag]") {
    Fixture fixture;
    fixture.controller.install(fixture.tree, nullptr);

    REQUIRE(fixture.controller.start(fixture.dragged));
    // 未调用 update：没有落点。
    REQUIRE_FALSE(fixture.controller.commit());
    REQUIRE(fixture.dragged->parent() == fixture.source.get());
}

TEST_CASE("drag cannot start on a detached node", "[widget][drag]") {
    Fixture fixture;
    fixture.controller.install(fixture.tree, nullptr);

    auto detached = make_item(10.0F, 10.0F);
    REQUIRE_FALSE(fixture.controller.start(detached));
    REQUIRE_FALSE(fixture.controller.active());
}

TEST_CASE("custom predicate can narrow the accepted containers", "[widget][drag]") {
    Fixture fixture;
    // 只允许落到 target（例如只读列表不允许作为落点）。
    fixture.controller.install(fixture.tree, nullptr, [&fixture](const scene::NanNode& node) {
        return &node == fixture.target.get();
    });

    REQUIRE(fixture.controller.start(fixture.dragged));
    {
        const auto bounds = fixture.source->global_bounds();
        fixture.controller.update(
            foundation::NanPoint {
                bounds.get_left() + bounds.get_width() * 0.5F,
                bounds.get_bottom() - 2.0F,
            }
        );
    }
    // source 及其祖先都被谓词排除，因此没有落点。
    REQUIRE(fixture.controller.drop_target() == nullptr);

    fixture.controller.update(fixture.point_in_target(190.0F));
    REQUIRE(fixture.controller.drop_target() == fixture.target.get());
}

TEST_CASE("commit revalidates the drop target before touching the tree", "[widget][drag]") {
    Fixture fixture;
    fixture.controller.install(fixture.tree, nullptr);

    REQUIRE(fixture.controller.start(fixture.dragged));
    fixture.controller.update(fixture.point_in_target(190.0F));
    REQUIRE(fixture.controller.drop_target() == fixture.target.get());

    // 拖动过程中落点被移出树（例如面板卸载、浮层关闭）。commit() 必须在改树之前
    // 重新确认，安全返回 false，而不是把已经脱离树的指针交给 reparent()。
    auto* row = fixture.root->get_child(0);
    REQUIRE(row != nullptr);
    auto detached = fixture.root->remove_child(*row);
    REQUIRE(detached != nullptr);
    REQUIRE_FALSE(fixture.target->is_inside_tree());

    REQUIRE_FALSE(fixture.controller.commit());
    REQUIRE_FALSE(fixture.controller.active());
    // 被拖节点必须留在原父节点下，树结构不被破坏。
    REQUIRE(fixture.dragged->parent() == fixture.source.get());
    REQUIRE(fixture.source->child_count() == 3);
    REQUIRE(fixture.target->child_count() == 2);
}

TEST_CASE("a target that rejects the child is never used as a drop target", "[widget][drag]") {
    // 目标拒绝一切子节点（例如只读容器）：既不该被解析成落点，也不该在提交时改树。
    // 用一个只覆写 accepts_child() 的小控件来表达这条策略。
    class RejectingColumn final: public widget::Column {
    public:
        [[nodiscard]] auto accepts_child(const scene::NanNode&) const -> bool override {
            return false;
        }
    };

    auto source = make_box(200.0F, 200.0F);
    auto dragged = make_item(200.0F, 60.0F);
    source->add(dragged);

    auto rejecting = std::make_shared<RejectingColumn>();
    rejecting->set_width(200.0F);
    rejecting->set_height(200.0F);
    rejecting->set_accepts_drop(true);

    auto root = widget::Column::create();
    auto row = widget::Row::create();
    row->set_gap(0.0F);
    row->add(source);
    row->add(rejecting);
    root->add(row);

    scene::NanSceneTree tree;
    tree.set_root(root);
    tree.layout_root(foundation::NanSize(400.0F, 200.0F));

    widget::DragController controller;
    controller.install(tree, nullptr);
    REQUIRE(controller.start(dragged));

    const auto bounds = rejecting->global_bounds();
    controller.update(foundation::NanPoint(
        bounds.get_left() + bounds.get_width() * 0.5F,
        bounds.get_bottom() - 2.0F
    ));
    REQUIRE(controller.drop_target() == nullptr);
    REQUIRE_FALSE(controller.commit());
    REQUIRE(dragged->parent() == source.get());
}
