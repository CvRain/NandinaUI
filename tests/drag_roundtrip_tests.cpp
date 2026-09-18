//
// drag_roundtrip_tests — 条目在两个容器之间来回拖动的语义。
//
// 背景：校验台最初拖的是条目**内部的 chip**，而不是条目本身（GestureArea 包装器）。
// 后果有两个：
//   * 包装器留在原容器占着一个槽位 —— 视觉上是"空槽"；
//   * chip 被塞进目标容器已有包装器之间 —— 视觉上是"组件互相覆盖"。
// 本文件固定"Is the whole item that moves"这一语义，并覆盖往返拖动。
//

#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/chip.hpp>
#include <nandina/widget/drag_controller.hpp>
#include <nandina/widget/gesture_area.hpp>
#include <nandina/widget/layout.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using namespace nandina;

namespace
{
    /// 一个可拖放容器：Column 接受放置，内部每个条目是 GestureArea(Chip)。
    struct Zone {
        std::shared_ptr<widget::Column> column;
        std::vector<std::shared_ptr<widget::GestureArea>> items;
        std::vector<std::shared_ptr<widget::Chip>> chips;
    };

    /// 给一个条目接上拖拽回调：拖动**条目自身**（不是里面的 chip）。
    void wire_drag(
        const std::shared_ptr<widget::GestureArea>& item,
        widget::DragController& controller
    ) {
        item->set_on_drag_start([&controller, item](const scene::MouseButtonEvent& e) {
            const auto origin = item->global_bounds().get_top_left();
            (void)controller.start(
                item,
                nullptr,
                foundation::NanPoint(
                    e.screen_pos().get_x() - origin.get_x(),
                    e.screen_pos().get_y() - origin.get_y()
                )
            );
        });
        item->set_on_drag_move([&controller](const scene::MouseMoveEvent& e) {
            controller.update(e.screen_pos());
        });
        item->set_on_drag_end([&controller](const scene::MouseButtonEvent&) {
            (void)controller.commit();
        });
    }

    [[nodiscard]] auto make_zone(const std::string& title, int count) -> Zone {
        Zone zone;
        zone.column = widget::Column::create();
        zone.column->set_gap(6.0F);
        zone.column->set_name(title);
        zone.column->set_accepts_drop(true);
        for (int index = 0; index < count; ++index) {
            auto chip = widget::Chip::create(title + " #" + std::to_string(index), false);
            chip->set_name(title + " #" + std::to_string(index));
            auto item = widget::GestureArea::create();
            item->set_child(chip);
            zone.column->add(item);
            zone.items.push_back(item);
            zone.chips.push_back(chip);
        }
        return zone;
    }

    /// 把 `item` 从当前位置拖到 `target` 的末尾。
    void drag_item_to(
        scene::NanSceneTree& tree,
        widget::DragController& controller,
        const std::shared_ptr<widget::GestureArea>& item,
        const std::shared_ptr<widget::Column>& target
    ) {
        const auto item_bounds = item->global_bounds();
        const foundation::NanPoint start(
            item_bounds.get_left() + 4.0F,
            item_bounds.get_top() + item_bounds.get_height() * 0.5F
        );
        const auto target_bounds = target->global_bounds();
        REQUIRE(target->child_count() > 0);
        const auto* last = target->get_child(target->child_count() - 1)->as_control();
        REQUIRE(last != nullptr);
        // 落在最后一个条目下方的容器空白处；夹在容器内部，避免因为
        // "最后条目 + 间距" 恰好越过容器底沿而落到容器之外。
        const auto last_bottom = last->global_bounds().get_bottom();
        const auto drop_y = std::min(last_bottom + 2.0F, target_bounds.get_bottom() - 1.0F);
        const foundation::NanPoint drop(
            target_bounds.get_left() + target_bounds.get_width() * 0.5F,
            drop_y
        );

        tree.dispatch_mouse_button(
            scene::MouseButtonEvent(
                scene::MouseButtonEvent::Button::left,
                scene::MouseButtonEvent::Action::press,
                start
            )
        );
        tree.dispatch_mouse_move(
            scene::MouseMoveEvent(
                foundation::NanPoint(start.get_x() + 24.0F, start.get_y()),
                foundation::NanPoint(24.0F, 0.0F)
            )
        );
        tree.dispatch_mouse_move(
            scene::MouseMoveEvent(
                drop,
                foundation::NanPoint(
                    drop.get_x() - start.get_x() - 24.0F,
                    drop.get_y() - start.get_y()
                )
            )
        );
        REQUIRE(controller.drop_target() == target.get());
        tree.dispatch_mouse_button(
            scene::MouseButtonEvent(
                scene::MouseButtonEvent::Button::left,
                scene::MouseButtonEvent::Action::release,
                drop
            )
        );
        tree.flush_tree_mutations();
    }
} // namespace

TEST_CASE("dragging a whole item round-trips between zones", "[widget][drag][roundtrip]") {
    auto list_a = make_zone("List A", 3);
    auto grid_b = make_zone("Grid B", 3);

    auto root = widget::Column::create();
    auto row = widget::Row::create();
    row->set_gap(12.0F);
    row->add(list_a.column);
    row->add(grid_b.column);
    root->add(row);

    scene::NanSceneTree tree;
    tree.set_root(root);
    tree.layout_root(foundation::NanSize(600.0F, 400.0F));

    widget::DragController controller;
    controller.install(tree, nullptr);

    // 移动的是条目（GestureArea），它带着自己的 chip 一起走。
    const auto item = list_a.items[0];
    wire_drag(item, controller);

    drag_item_to(tree, controller, item, grid_b.column);
    REQUIRE(item->parent() == grid_b.column.get());
    REQUIRE(item->child_count() == 1);
    // 源容器少一个条目、目标容器多一个 —— 不留空槽、也不覆盖。
    REQUIRE(list_a.column->child_count() == 2);
    REQUIRE(grid_b.column->child_count() == 4);

    // 再拖回去：命中测试此时落在目标容器里的条目上，应当稳定地解析到目标容器。
    drag_item_to(tree, controller, item, list_a.column);
    REQUIRE(item->parent() == list_a.column.get());
    REQUIRE(item->child_count() == 1);
    REQUIRE(list_a.column->child_count() == 3);
    REQUIRE(grid_b.column->child_count() == 3);

    // 往返之后每个条目仍然各占一个槽位：容器子节点数 = 条目数，且没有嵌套。
    for (const auto& entry: list_a.items) {
        REQUIRE(entry->parent() == list_a.column.get());
        REQUIRE(entry->child_count() == 1);
    }
    for (const auto& entry: grid_b.items) {
        REQUIRE(entry->parent() == grid_b.column.get());
        REQUIRE(entry->child_count() == 1);
    }
}

TEST_CASE("repeated cross-zone drags keep the item intact", "[widget][drag][roundtrip]") {
    auto left = make_zone("Left", 2);
    auto right = make_zone("Right", 2);

    auto root = widget::Column::create();
    auto row = widget::Row::create();
    row->set_gap(12.0F);
    row->add(left.column);
    row->add(right.column);
    root->add(row);

    scene::NanSceneTree tree;
    tree.set_root(root);
    tree.layout_root(foundation::NanSize(600.0F, 400.0F));

    widget::DragController controller;
    controller.install(tree, nullptr);

    const auto item = left.items[0];
    wire_drag(item, controller);
    for (int round = 0; round < 3; ++round) {
        drag_item_to(tree, controller, item, right.column);
        REQUIRE(item->parent() == right.column.get());
        drag_item_to(tree, controller, item, left.column);
        REQUIRE(item->parent() == left.column.get());
    }

    // 无论来回多少次，两个容器都保持"每槽一个条目"。
    REQUIRE(left.column->child_count() == 2);
    REQUIRE(right.column->child_count() == 2);
    REQUIRE(item->child_count() == 1);
}
