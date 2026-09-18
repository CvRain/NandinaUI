//
// layout_overflow_tests — 内容高于容器时的行为约束。
//
// 背景：校验台里出现过「内容纵向超出视口后，顶部若干行从可视区消失、只剩末尾
// 部分落在视口中部」的现象。本文件把「内容超出容器时，第一个子项必须从容器内容区
// 的起点开始」固定下来，避免回归。
//

#include <nandina/reactive/graph.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/layout.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>

using namespace nandina;

namespace
{
    /// 固定高度的占位块。
    [[nodiscard]] auto block(float height) -> std::shared_ptr<widget::Column> {
        auto column = widget::Column::create();
        column->set_height(height);
        return column;
    }

    /// 记录每个直接子项的 global_bounds。
    [[nodiscard]] auto child_offsets(const widget::Column& column) {
        std::vector<float> offsets;
        for (std::size_t index = 0; index < column.child_count(); ++index) {
            const auto* child = column.get_child(index)->as_control();
            if (child != nullptr) {
                offsets.push_back(child->global_bounds().get_y());
            }
        }
        return offsets;
    }
} // namespace

TEST_CASE("overflowing column keeps its first child at the content origin", "[layout][overflow]") {
    // 视口 800x400，内容 10×120 = 1200 > 400，Padding 上下各 20。
    auto column = widget::Column::create();
    column->set_gap(0.0F);
    for (int index = 0; index < 10; ++index) {
        column->add(block(120.0F));
    }

    constexpr float padding = 20.0F;
    auto page = widget::Padding::create(foundation::NanInsets::all(padding));
    page->set_child(column);

    scene::NanSceneTree tree;
    tree.set_root(page);
    tree.layout_root(foundation::NanSize(800.0F, 400.0F));

    // 根容器被约束到视口。
    REQUIRE(page->size().get_height() == Catch::Approx(400.0F));
    // 内容起点：至少不能跑到 padding 之上。
    const auto offsets = child_offsets(*column);
    REQUIRE(offsets.size() == 10);
    REQUIRE(offsets.front() >= padding - 1.0F);
    // 相邻子项保持顺序且紧邻（gap = 0）。
    for (std::size_t index = 1; index < offsets.size(); ++index) {
        REQUIRE(offsets[index] >= offsets[index - 1] + 120.0F - 1.0F);
    }
}

TEST_CASE("overflowing row keeps its first child at the content origin", "[layout][overflow]") {
    auto row = widget::Row::create();
    row->set_gap(0.0F);
    for (int index = 0; index < 10; ++index) {
        auto item = block(100.0F);
        item->set_width(200.0F);
        row->add(item);
    }

    constexpr float padding = 20.0F;
    auto page = widget::Padding::create(foundation::NanInsets::all(padding));
    page->set_child(row);

    scene::NanSceneTree tree;
    tree.set_root(page);
    tree.layout_root(foundation::NanSize(400.0F, 200.0F));

    REQUIRE(page->size().get_width() == Catch::Approx(400.0F));
    const auto* first = row->get_child(0)->as_control();
    REQUIRE(first != nullptr);
    REQUIRE(first->global_bounds().get_x() >= padding - 1.0F);
}

TEST_CASE("content that fits is unaffected by overflow clamping", "[layout][overflow]") {
    auto column = widget::Column::create();
    column->set_gap(0.0F);
    column->add(block(100.0F));
    column->add(block(100.0F));

    constexpr float padding = 20.0F;
    auto page = widget::Padding::create(foundation::NanInsets::all(padding));
    page->set_child(column);

    scene::NanSceneTree tree;
    tree.set_root(page);
    tree.layout_root(foundation::NanSize(800.0F, 400.0F));

    const auto offsets = child_offsets(*column);
    REQUIRE(offsets.size() == 2);
    REQUIRE(offsets[0] == Catch::Approx(padding));
    REQUIRE(offsets[1] == Catch::Approx(padding + 100.0F));
}
