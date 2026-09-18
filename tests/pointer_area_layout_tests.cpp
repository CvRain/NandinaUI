//
// pointer_area_layout_tests — 单子节点交互容器的布局契约。
//
// 背景：`PointerArea` / `GestureArea` 是**单子节点**容器（内部只认 `child_`）。
// 用 `add_child` 挂进去的节点会被 `scene` 接受（成为子节点、参与 draw），但
// `PointerArea::on_layout()` 只看 `child_`，于是：
//   * 该节点永远不被布局（停在原点）；
//   * PointerArea 自身量成 0 高度。
// 校验台里就出现过这种症状：一批条目全部叠在同一位置。本文件把这个陷阱固定下来。
//

#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/layout.hpp>
#include <nandina/widget/pointer_area.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>

using namespace nandina;

namespace
{
    /// 固定尺寸的实心块，便于断言布局结果。
    [[nodiscard]] auto block(float width, float height) -> std::shared_ptr<widget::Column> {
        auto box = widget::Column::create();
        box->set_width(width);
        box->set_height(height);
        return box;
    }
} // namespace

TEST_CASE(
    "pointer area lays out its single child via set_child",
    "[widget][layout][pointer-area]"
) {
    // 放进 Column 里测量：root 会被 tight 约束到视口，无法反映交互区域的自适应尺寸。
    auto column = widget::Column::create();
    auto area = widget::PointerArea::create();
    auto content = block(120.0F, 40.0F);
    area->set_child(content);
    column->add(area);

    scene::NanSceneTree tree;
    tree.set_root(column);
    tree.layout_root(foundation::NanSize(300.0F, 200.0F));

    // 交互区域自身采用子节点的测量尺寸。
    REQUIRE(area->size().get_width() == Catch::Approx(120.0F));
    REQUIRE(area->size().get_height() == Catch::Approx(40.0F));

    // 子节点被铺满交互区域。
    REQUIRE(content->size().get_width() == Catch::Approx(120.0F));
    REQUIRE(content->size().get_height() == Catch::Approx(40.0F));
    REQUIRE(content->position().get_x() == Catch::Approx(0.0F));
    REQUIRE(content->position().get_y() == Catch::Approx(0.0F));
}

TEST_CASE(
    "adding a child to a pointer area does not participate in layout",
    "[widget][layout][pointer-area]"
) {
    // 这是刻意的行为锁定：add_child 绕过了 PointerArea 的单子节点槽位，
    // 因此交互区域量成 0 尺寸。挂内容请用 set_child。
    auto column = widget::Column::create();
    auto area = widget::PointerArea::create();
    area->add_child(block(120.0F, 40.0F));
    column->add(area);

    scene::NanSceneTree tree;
    tree.set_root(column);
    tree.layout_root(foundation::NanSize(300.0F, 200.0F));

    REQUIRE(area->size().get_height() == Catch::Approx(0.0F));
    REQUIRE(area->size().get_width() == Catch::Approx(0.0F));
}

TEST_CASE("stacked pointer areas keep their column layout", "[widget][layout][pointer-area]") {
    // 复刻校验台 drag 区的结构：Column[ PointerArea[chip], PointerArea[chip] ]。
    // 若误用 add_child，所有条目会叠在 y = 0。
    auto column = widget::Column::create();
    column->set_gap(6.0F);

    for (int index = 0; index < 3; ++index) {
        auto area = widget::PointerArea::create();
        area->set_child(block(100.0F, 24.0F));
        column->add(area);
    }

    scene::NanSceneTree tree;
    tree.set_root(column);
    tree.layout_root(foundation::NanSize(300.0F, 200.0F));

    // 三个条目依次向下排列：y = 0 / 30 / 60（高 24 + 间距 6）。
    for (int index = 0; index < 3; ++index) {
        const auto* child = column->get_child(static_cast<std::size_t>(index))->as_control();
        REQUIRE(child != nullptr);
        REQUIRE(child->position().get_y() == Catch::Approx(static_cast<float>(index) * 30.0F));
        REQUIRE(child->size().get_height() == Catch::Approx(24.0F));
    }
}
