//
// drag_gesture_tests — 真实输入事件驱动的拖拽（GestureArea + DragController）。
//
// 覆盖校验台那条路径：press → move(超过阈值) → move(到目标) → release，
// 其中 release 会触发 DragController::commit() 并换父。
//
// 接线约定（校验台曾在这里踩坑并 SIGSEGV）：
//   * 不要按引用捕获取自 BuildContext 的服务（如 `[&ui]`）。GestureArea 的
//     **裸** setter 不受 callback-lifetime 守卫，回调会活到交互之后，那时 build
//     已经返回 —— 捕获 `&ui` 就是悬垂访问。请捕获服务指针自身
//     （`auto* drags = ui.drag_controller()`），或走 builder 的 on_drag_* 钩子。
//   * 不需要手动 set_pointer_capture：PointerArea 在 press 时已把捕获目标设为 child。
//

#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/chip.hpp>
#include <nandina/widget/drag_controller.hpp>
#include <nandina/widget/gesture_area.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/layout.hpp>

#include <catch2/catch_test_macros.hpp>

#include <memory>

using namespace nandina;

namespace
{
    [[nodiscard]] auto make_box(float width, float height) -> std::shared_ptr<widget::Column> {
        auto box = widget::Column::create();
        box->set_width(width);
        box->set_height(height);
        box->set_gap(0.0F);
        box->set_accepts_drop(true);
        return box;
    }
} // namespace

TEST_CASE("gesture-driven drag commits without dangling access", "[widget][drag][gesture]") {
    // 源容器（左）与目标容器（右）：两个 200x200 并排。
    auto source = make_box(200.0F, 200.0F);
    auto target = make_box(200.0F, 200.0F);

    auto chip = widget::Chip::create("chip", false);
    auto gesture = widget::GestureArea::create();
    gesture->set_child(chip);
    source->add(gesture);

    auto root = widget::Column::create();
    auto row = widget::Row::create();
    row->set_gap(0.0F);
    row->add(source);
    row->add(target);
    root->add(row);

    scene::NanSceneTree tree;
    tree.set_root(root);
    tree.layout_root(foundation::NanSize(400.0F, 200.0F));

    widget::DragController controller;
    controller.install(tree, nullptr);

    // 与校验台相同的接线方式。
    gesture->set_on_drag_start([&](const scene::MouseButtonEvent& e) {
        const auto origin = chip->global_bounds().get_top_left();
        (void)controller.start(
            chip,
            nullptr,
            foundation::NanPoint(
                e.screen_pos().get_x() - origin.get_x(),
                e.screen_pos().get_y() - origin.get_y()
            )
        );
        // 不手动捕获：PointerArea 已把捕获目标设为 child。
    });
    gesture->set_on_drag_move([&](const scene::MouseMoveEvent& e) {
        controller.update(e.screen_pos());
    });
    gesture->set_on_drag_end([&](const scene::MouseButtonEvent&) { (void)controller.commit(); });

    const auto chip_bounds = chip->global_bounds();
    const foundation::NanPoint start_point(
        chip_bounds.get_left() + 4.0F,
        chip_bounds.get_top() + 4.0F
    );
    const auto target_bounds = target->global_bounds();
    const foundation::NanPoint drop_point(
        target_bounds.get_left() + target_bounds.get_width() * 0.5F,
        target_bounds.get_bottom() - 4.0F
    );

    REQUIRE(chip->parent() == gesture.get());
    REQUIRE(source->child_count() == 1);

    const scene::MouseButtonEvent press_event(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        start_point
    );
    tree.dispatch_mouse_button(press_event);
    const scene::MouseMoveEvent first_move(
        foundation::NanPoint(start_point.get_x() + 20.0F, start_point.get_y()),
        foundation::NanPoint(20.0F, 0.0F)
    );
    tree.dispatch_mouse_move(first_move);
    REQUIRE(controller.active());

    const scene::MouseMoveEvent drop_move(
        drop_point,
        foundation::NanPoint(
            drop_point.get_x() - start_point.get_x() - 20.0F,
            drop_point.get_y() - start_point.get_y()
        )
    );
    tree.dispatch_mouse_move(drop_move);
    REQUIRE(controller.drop_target() == target.get());

    // 松手：commit 会把 chip 移到 target，同时把 gesture 从 source 摘掉。
    const scene::MouseButtonEvent release_event(
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::release,
        drop_point
    );
    tree.dispatch_mouse_button(release_event);

    REQUIRE_FALSE(controller.active());
    REQUIRE(chip->parent() == target.get());
    // chip 被移出 GestureArea，但 GestureArea 自身仍挂在 source 下。
    REQUIRE(gesture->child_count() == 0);
    REQUIRE(source->child_count() == 1);
    REQUIRE(target->child_count() == 1);
}
