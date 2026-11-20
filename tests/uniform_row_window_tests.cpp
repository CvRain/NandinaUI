// Direct unit tests for the pure uniform-row window maths shared by DataTable and ListView.
#include <nandina/widget/internal/uniform_row_window.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <limits>
#include <optional>

namespace internal = nandina::widget::internal;

namespace
{
    constexpr float kRowHeight = 20.0F;

    auto make_window(
        const std::size_t item_count,
        const float viewport_height,
        const float row_height = kRowHeight
    ) -> internal::UniformRowWindow {
        return internal::UniformRowWindow {row_height, viewport_height, item_count};
    }

    auto
    reveal(const internal::UniformRowWindow& window, const std::size_t index, const float offset)
        -> std::optional<float> {
        return window.reveal_offset(index, offset);
    }
} // namespace

TEST_CASE("uniform row window computes content height and clamps offsets", "[uniform-row-window]") {
    const auto window = make_window(10, 100.0F);
    CHECK(window.item_count() == 10);
    CHECK(window.content_height() == 200.0F);
    CHECK(window.maximum_scroll_offset() == 100.0F);

    // 顶部、底部与越界偏移都被夹回 [0, maximum]。
    CHECK(window.clamp_offset(-25.0F) == 0.0F);
    CHECK(window.clamp_offset(0.0F) == 0.0F);
    CHECK(window.clamp_offset(40.0F) == 40.0F);
    CHECK(window.clamp_offset(100.0F) == 100.0F);
    CHECK(window.clamp_offset(1000.0F) == 100.0F);
    CHECK(window.clamp_offset(std::numeric_limits<float>::quiet_NaN()) == 0.0F);
    CHECK(window.clamp_offset(-std::numeric_limits<float>::infinity()) == 0.0F);
    CHECK(window.clamp_offset(std::numeric_limits<float>::infinity()) == 100.0F);

    SECTION("empty data has no window or scroll range") {
        const auto empty = make_window(0, 100.0F);
        CHECK(empty.content_height() == 0.0F);
        CHECK(empty.maximum_scroll_offset() == 0.0F);
        CHECK(empty.clamp_offset(50.0F) == 0.0F);
        CHECK(empty.visible_window(0.0F).empty());
        CHECK(empty.hit_row(0.0F, 0.0F, {}) == std::nullopt);
        CHECK(reveal(empty, 0, 0.0F) == std::nullopt);
    }

    SECTION("zero-height viewport produces no visible window") {
        const auto pre_layout = make_window(10, 0.0F);
        CHECK(pre_layout.content_height() == 200.0F);
        CHECK(pre_layout.maximum_scroll_offset() == 200.0F);
        CHECK(pre_layout.clamp_offset(250.0F) == 200.0F);
        CHECK(pre_layout.visible_window(0.0F).empty());
        CHECK(reveal(pre_layout, 3, 0.0F) == 0.0F);
        CHECK(reveal(pre_layout, 3, 25.0F) == 25.0F);
    }
}

TEST_CASE("uniform row window uses a half-open visible interval", "[uniform-row-window]") {
    // 整行视口：5 行完整可见，end 是排除边界。
    const auto full = make_window(10, 100.0F).visible_window(0.0F);
    CHECK(full.first == 0);
    CHECK(full.end == 5);
    CHECK(full.count() == 5);
    CHECK_FALSE(full.empty());

    // 半行视口：第 3 行只露出一半，仍属于可见窗口。
    const auto half = make_window(10, 50.0F).visible_window(0.0F);
    CHECK(half.first == 0);
    CHECK(half.end == 3);
    CHECK(half.count() == 3);

    // 视口高度不足一行时仍有 1 行可见。
    const auto tiny = make_window(10, 5.0F).visible_window(0.0F);
    CHECK(tiny.first == 0);
    CHECK(tiny.end == 1);

    // 偏移不改变窗口端点（向前半行）。
    const auto shifted = make_window(10, 50.0F).visible_window(30.0F);
    CHECK(shifted.first == 1);
    CHECK(shifted.end == 4);

    // 可见窗口自己夹取 offset，不把隐藏前置留给后续调用方。
    const auto before_top = make_window(10, 50.0F).visible_window(-100.0F);
    CHECK(before_top.first == 0);
    CHECK(before_top.end == 3);
    const auto after_bottom = make_window(10, 50.0F).visible_window(1000.0F);
    CHECK(after_bottom.first == 7);
    CHECK(after_bottom.end == 10);
    const auto nan_offset =
        make_window(10, 50.0F).visible_window(std::numeric_limits<float>::quiet_NaN());
    CHECK(nan_offset.first == 0);
    CHECK(nan_offset.end == 3);

    // 空数据：半开区间自然为空。
    const auto empty = make_window(0, 100.0F).visible_window(0.0F);
    CHECK(empty.first == empty.end);
    CHECK(empty.empty());
}

TEST_CASE("uniform row window handles exact row-boundary viewports", "[uniform-row-window]") {
    // 视口恰好 3 行：ceil(60/20)-1 = 2，end = 3。
    const auto boundary = make_window(10, 60.0F).visible_window(0.0F);
    CHECK(boundary.first == 0);
    CHECK(boundary.end == 3);

    const auto at_one_row = make_window(10, 60.0F).visible_window(20.0F);
    CHECK(at_one_row.first == 1);
    CHECK(at_one_row.end == 4);

    const auto at_three_rows = make_window(10, 60.0F).visible_window(60.0F);
    CHECK(at_three_rows.first == 3);
    CHECK(at_three_rows.end == 6);

    // 内容短于视口：窗口覆盖全部行，end 夹到行数。
    const auto short_content = make_window(2, 100.0F).visible_window(0.0F);
    CHECK(short_content.first == 0);
    CHECK(short_content.end == 2);
}

TEST_CASE("uniform row window clamps offsets at the scroll extremes", "[uniform-row-window]") {
    const auto window = make_window(10, 60.0F);
    CHECK(window.maximum_scroll_offset() == 140.0F);

    // 底边：窗口从第 7 行开始，覆盖最后 3 行。
    const auto bottom = window.visible_window(140.0F);
    CHECK(bottom.first == 7);
    CHECK(bottom.end == 10);

    // 越过底部仍夹取；偏移不被纯窗口数学直接改写，由调用方写回 clamp_offset。
    CHECK(window.clamp_offset(9999.0F) == 140.0F);
    const auto clamped_bottom = window.visible_window(window.clamp_offset(9999.0F));
    CHECK(clamped_bottom.first == 7);
    CHECK(clamped_bottom.end == 10);

    const auto off_by_half = window.visible_window(139.5F);
    CHECK(off_by_half.first == 6);
    CHECK(off_by_half.end == 10);
}

TEST_CASE("uniform row window reveals visible, above and below rows", "[uniform-row-window]") {
    const auto window = make_window(10, 100.0F);

    // 目标已完整可见：offset 不变。
    CHECK(reveal(window, 3, 40.0F) == 40.0F);
    CHECK(reveal(window, 2, 40.0F) == 40.0F);

    // 目标在视口上方：贴顶。
    CHECK(reveal(window, 1, 40.0F) == 20.0F);
    CHECK(reveal(window, 0, 40.0F) == 0.0F);

    // 目标在视口下方：贴底。
    CHECK(reveal(window, 8, 40.0F) == 80.0F);
    CHECK(reveal(window, 9, 40.0F) == 100.0F);

    // 已在底部时滚动到末行保持不动。
    CHECK(reveal(window, 9, 100.0F) == 100.0F);

    // 输入 offset 先夹取，然后再做显现计算。
    CHECK(reveal(window, 0, std::numeric_limits<float>::quiet_NaN()) == 0.0F);
    CHECK(reveal(window, 9, std::numeric_limits<float>::infinity()) == 100.0F);

    // 首布局前视口高度为 0：原样返回 offset，由视图决定挂起。
    const auto pre_layout = make_window(10, 0.0F);
    CHECK(reveal(pre_layout, 5, 0.0F) == 0.0F);
    CHECK(reveal(pre_layout, 5, 30.0F) == 30.0F);
}

TEST_CASE("uniform row window rejects invalid reveal indices", "[uniform-row-window]") {
    const auto window = make_window(10, 100.0F);
    CHECK(reveal(window, 10, 0.0F) == std::nullopt);
    CHECK(reveal(window, 1000, 0.0F) == std::nullopt);

    const auto empty = make_window(0, 100.0F);
    CHECK(reveal(empty, 0, 0.0F) == std::nullopt);
}

TEST_CASE("uniform row window hit-tests rows inside the viewport", "[uniform-row-window]") {
    const auto window = make_window(10, 60.0F);
    const auto visible = window.visible_window(0.0F);
    REQUIRE(visible.end == 3);

    CHECK(window.hit_row(0.0F, 0.0F, visible) == 0);
    CHECK(window.hit_row(19.5F, 0.0F, visible) == 0);
    CHECK(window.hit_row(20.0F, 0.0F, visible) == 1);
    CHECK(window.hit_row(39.5F, 0.0F, visible) == 1);
    CHECK(window.hit_row(40.0F, 0.0F, visible) == 2);

    // 视口底边（整行视口）命中最后一个已物化行，而不是未物化的下一行。
    CHECK(window.hit_row(60.0F, 0.0F, visible) == 2);

    // 半行视口：底边落在半可见行上，不夹取。
    const auto half = make_window(10, 50.0F);
    const auto half_visible = half.visible_window(0.0F);
    REQUIRE(half_visible.end == 3);
    CHECK(half.hit_row(50.0F, 0.0F, half_visible) == 2);

    // 视口外或负坐标不命中。
    CHECK(window.hit_row(-1.0F, 0.0F, visible) == std::nullopt);
    CHECK(window.hit_row(61.0F, 0.0F, visible) == std::nullopt);
    CHECK(window.hit_row(std::numeric_limits<float>::quiet_NaN(), 0.0F, visible) == std::nullopt);

    // 命中与可见窗口共用同一 offset 夹取规则。
    CHECK(window.hit_row(0.0F, -100.0F, visible) == 0);
    const auto bottom_visible = window.visible_window(1000.0F);
    CHECK(window.hit_row(0.0F, 1000.0F, bottom_visible) == 7);
}

TEST_CASE("row window treats malformed half-open ranges as empty", "[uniform-row-window]") {
    const internal::RowWindow reversed {.first = 5, .end = 2};
    CHECK(reversed.empty());
    CHECK(reversed.count() == 0);

    const auto window = make_window(10, 60.0F);
    CHECK(window.hit_row(60.0F, 0.0F, reversed) == 3);

    // 越过数据末尾的窗口上界会被夹回 item_count，不返回越界行。
    const internal::RowWindow oversized {.first = 0, .end = 100};
    CHECK(window.hit_row(60.0F, 0.0F, oversized) == 3);
}

TEST_CASE("uniform row window hit-tests the last row and short content", "[uniform-row-window]") {
    // 内容短于视口：只有 2 行，底边与视口外都不命中。
    const auto short_content = make_window(2, 100.0F);
    const auto visible = short_content.visible_window(0.0F);
    REQUIRE(visible.end == 2);
    CHECK(short_content.hit_row(0.0F, 0.0F, visible) == 0);
    CHECK(short_content.hit_row(20.0F, 0.0F, visible) == 1);
    CHECK(short_content.hit_row(40.0F, 0.0F, visible) == std::nullopt);
    CHECK(short_content.hit_row(100.0F, 0.0F, visible) == std::nullopt);

    // 内容恰好等于视口：底边不命中额外行。
    const auto exact = make_window(5, 100.0F);
    const auto exact_visible = exact.visible_window(0.0F);
    REQUIRE(exact_visible.end == 5);
    CHECK(exact.hit_row(99.9F, 0.0F, exact_visible) == 4);
    CHECK(exact.hit_row(100.0F, 0.0F, exact_visible) == std::nullopt);

    // 滚到底后底边是内容底，不命中。
    const auto bottom = make_window(10, 100.0F);
    const auto bottom_visible = bottom.visible_window(100.0F);
    REQUIRE(bottom_visible.first == 5);
    REQUIRE(bottom_visible.end == 10);
    CHECK(bottom.hit_row(60.0F, 100.0F, bottom_visible) == 8);
    CHECK(bottom.hit_row(99.9F, 100.0F, bottom_visible) == 9);
    CHECK(bottom.hit_row(100.0F, 100.0F, bottom_visible) == std::nullopt);

    // 最后一行半可见时仍可命中。
    const auto partial = make_window(3, 50.0F);
    const auto partial_visible = partial.visible_window(0.0F);
    CHECK(partial_visible.end == 3);
    CHECK(partial.hit_row(50.0F, 0.0F, partial_visible) == 2);
}
