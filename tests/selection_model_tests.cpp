//
// selection_model_tests — SelectionModel 纯逻辑契约。
//
// 覆盖：single/multiple 语义、幂等与事件、范围选择的跨度替换、锚点、
// 数据替换 retain 裁剪、模式切换降级。
//

#include <nandina/reactive/event.hpp>
#include <nandina/widget/selection_model.hpp>

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <vector>

namespace
{
    using Model = nandina::widget::SelectionModel<int>;
} // namespace

TEST_CASE("SelectionModel single mode replaces and is idempotent", "[widget][selection]") {
    Model model {nandina::widget::SelectionMode::single};
    int changes = 0;
    auto sub = model.changed().subscribe([&] { ++changes; });

    REQUIRE(model.select(3));
    REQUIRE(model.selected() == std::vector<int> {3});
    REQUIRE(model.anchor() == std::optional<int> {3});
    REQUIRE(changes == 1);

    // 同键重复选中：无变化、不触发事件。
    REQUIRE_FALSE(model.select(3));
    REQUIRE(model.selected() == std::vector<int> {3});
    REQUIRE(changes == 1);

    // 换键：替换而不是累加。
    REQUIRE(model.select(7));
    REQUIRE(model.selected() == std::vector<int> {7});
    REQUIRE(changes == 2);

    // toggle 已选成员 = 清空；再 toggle = 选中。
    REQUIRE(model.toggle(7));
    REQUIRE(model.selected().empty());
    REQUIRE(model.anchor().has_value() == false);
    REQUIRE(model.toggle(7));
    REQUIRE(model.selected() == std::vector<int> {7});

    REQUIRE(model.clear());
    REQUIRE(model.selected().empty());
    REQUIRE_FALSE(model.clear());
    REQUIRE(changes == 5);
}

TEST_CASE("SelectionModel multiple mode accumulates and toggles", "[widget][selection]") {
    Model model {nandina::widget::SelectionMode::multiple};
    REQUIRE(model.select(1));
    REQUIRE(model.select(2));
    REQUIRE_FALSE(model.select(2));
    REQUIRE(model.selected() == std::vector<int> {1, 2});

    REQUIRE(model.toggle(1));
    REQUIRE(model.selected() == std::vector<int> {2});
    REQUIRE(model.toggle(1));
    REQUIRE(model.selected() == std::vector<int> {2, 1});
}

TEST_CASE("SelectionModel range selection replaces with the span", "[widget][selection]") {
    Model model {nandina::widget::SelectionMode::multiple};
    (void)model.select(1);
    (void)model.select(5);
    const std::vector<int> ordered {1, 2, 3, 4, 5, 6};

    REQUIRE(model.select_range(ordered, 5, 2));
    // 跨度按行序展开（含两端），替换旧选中。
    REQUIRE(model.selected() == std::vector<int> {2, 3, 4, 5});
    REQUIRE(model.anchor() == std::optional<int> {2});

    // 同样的跨度重复选择：无变化。
    REQUIRE_FALSE(model.select_range(ordered, 2, 5));

    // 未知键：不改动任何状态。
    const auto before = model.selected();
    REQUIRE_FALSE(model.select_range(ordered, 5, 99));
    REQUIRE(model.selected() == before);
}

TEST_CASE("SelectionModel single-mode range equals select(to)", "[widget][selection]") {
    Model model {nandina::widget::SelectionMode::single};
    (void)model.select(2);
    const std::vector<int> ordered {1, 2, 3, 4};
    REQUIRE(model.select_range(ordered, 1, 4));
    REQUIRE(model.selected() == std::vector<int> {4});
}

TEST_CASE("SelectionModel retain prunes removed keys and anchors", "[widget][selection]") {
    Model model {nandina::widget::SelectionMode::multiple};
    (void)model.select(1);
    (void)model.select(2);
    (void)model.select(3);
    model.select_range(std::vector<int> {1, 2, 3}, 1, 3);
    REQUIRE(model.anchor() == std::optional<int> {3});

    int changes = 0;
    auto sub = model.changed().subscribe([&] { ++changes; });

    // 数据替换后 key 2 / 3 消失：选中裁剪到 {1}，锚点清空。
    REQUIRE(model.retain(std::vector<int> {1, 4}));
    REQUIRE(model.selected() == std::vector<int> {1});
    REQUIRE_FALSE(model.anchor().has_value());
    REQUIRE(changes == 1);

    // 无裁剪：不触发事件。
    REQUIRE_FALSE(model.retain(std::vector<int> {1, 4}));
    REQUIRE(changes == 1);
}

TEST_CASE("SelectionModel single-mode range rejects unknown keys", "[widget][selection]") {
    Model model {nandina::widget::SelectionMode::single};
    (void)model.select(2);
    const std::vector<int> ordered {1, 2, 3, 4};

    // 契约 §3：未知键不修改任何状态 —— single 模式也不能绕过成员检查直接选中 to。
    REQUIRE_FALSE(model.select_range(ordered, 1, 99));
    REQUIRE(model.selected() == std::vector<int> {2});
    REQUIRE(model.anchor() == std::optional<int> {2});
    REQUIRE_FALSE(model.select_range(ordered, 99, 2));
    REQUIRE(model.selected() == std::vector<int> {2});
}

TEST_CASE(
    "SelectionModel repeated span updates the anchor without an event",
    "[widget][selection]"
) {
    Model model {nandina::widget::SelectionMode::multiple};
    (void)model.select(1);
    model.select_range(std::vector<int> {1, 2, 3, 4}, 1, 4);
    REQUIRE(model.anchor() == std::optional<int> {4});

    int changes = 0;
    auto sub = model.changed().subscribe([&] { ++changes; });

    // 同一跨度反向选择：集合不变（不触发事件），但落点 to 成为新锚点 ——
    // shift 点击在两个端点间往返时锚点必须跟随最后一次落点。
    REQUIRE_FALSE(model.select_range(std::vector<int> {1, 2, 3, 4}, 4, 1));
    REQUIRE(model.anchor() == std::optional<int> {1});
    REQUIRE(changes == 0);
}

TEST_CASE("SelectionModel mode switch degrades to single selection", "[widget][selection]") {
    Model model {nandina::widget::SelectionMode::multiple};
    (void)model.select(1);
    (void)model.select(2);
    (void)model.select(3);

    int changes = 0;
    auto sub = model.changed().subscribe([&] { ++changes; });
    model.set_mode(nandina::widget::SelectionMode::single);
    REQUIRE(model.selected() == std::vector<int> {1});
    REQUIRE(changes == 1);

    // 同模式重复设置：不触发事件。
    model.set_mode(nandina::widget::SelectionMode::single);
    REQUIRE(changes == 1);
}

TEST_CASE("SelectionModel mode degrade repoints a stale anchor", "[widget][selection]") {
    Model model {nandina::widget::SelectionMode::multiple};
    (void)model.select(1);
    (void)model.select(2);
    (void)model.select(3);
    REQUIRE(model.anchor() == std::optional<int> {3});

    // 降级保留首成员：锚点不得仍指向被裁剪的成员（锚点 = 已选中成员不变量）。
    model.set_mode(nandina::widget::SelectionMode::single);
    REQUIRE(model.selected() == std::vector<int> {1});
    REQUIRE(model.anchor() == std::optional<int> {1});

    // 切回 multiple：新成员独立累加，锚点跟随最近落点。
    model.set_mode(nandina::widget::SelectionMode::multiple);
    (void)model.select(2);
    (void)model.select(3);
    REQUIRE(model.selected() == std::vector<int> {1, 2, 3});
    REQUIRE(model.anchor() == std::optional<int> {3});
}

TEST_CASE("SelectionModel mode switches without pruning do not emit", "[widget][selection]") {
    Model model {nandina::widget::SelectionMode::single};
    (void)model.select(5);

    int changes = 0;
    auto sub = model.changed().subscribe([&] { ++changes; });
    // 同模式：early return。
    model.set_mode(nandina::widget::SelectionMode::single);
    // 单成员切到 multiple：无裁剪、锚点仍合法 → 不触发。
    model.set_mode(nandina::widget::SelectionMode::multiple);
    REQUIRE(changes == 0);
}

TEST_CASE("SelectionModel range with from == to selects a single key", "[widget][selection]") {
    Model model {nandina::widget::SelectionMode::multiple};
    const std::vector<int> ordered {1, 2, 3, 4};
    REQUIRE(model.select_range(ordered, 3, 3));
    REQUIRE(model.selected() == std::vector<int> {3});
    REQUIRE(model.anchor() == std::optional<int> {3});
}

TEST_CASE("SelectionModel range with empty ordered rejects in both modes", "[widget][selection]") {
    Model multiple_model {nandina::widget::SelectionMode::multiple};
    (void)multiple_model.select(1);
    REQUIRE_FALSE(multiple_model.select_range({}, 1, 1));
    REQUIRE(multiple_model.selected() == std::vector<int> {1});

    Model single_model {nandina::widget::SelectionMode::single};
    (void)single_model.select(2);
    REQUIRE_FALSE(single_model.select_range({}, 1, 1));
    REQUIRE(single_model.selected() == std::vector<int> {2});
}

TEST_CASE(
    "SelectionModel toggle keeps a non-anchor member and clears a removed anchor",
    "[widget][selection]"
) {
    Model model {nandina::widget::SelectionMode::multiple};
    (void)model.select(1);
    (void)model.select(2);
    (void)model.select(3);
    REQUIRE(model.anchor() == std::optional<int> {3});

    // 移除非锚点成员：锚点保持。
    (void)model.toggle(1);
    REQUIRE(model.selected() == std::vector<int> {2, 3});
    REQUIRE(model.anchor() == std::optional<int> {3});

    // 移除锚点成员：锚点清空。
    (void)model.toggle(3);
    REQUIRE(model.selected() == std::vector<int> {2});
    REQUIRE_FALSE(model.anchor().has_value());
}

TEST_CASE("SelectionModel retain with an empty universe clears everything", "[widget][selection]") {
    Model model {nandina::widget::SelectionMode::multiple};
    (void)model.select(1);
    (void)model.select(2);
    model.select_range(std::vector<int> {1, 2, 3}, 1, 2);
    REQUIRE(model.anchor() == std::optional<int> {2});

    int changes = 0;
    auto sub = model.changed().subscribe([&] { ++changes; });
    REQUIRE(model.retain({}));
    REQUIRE(model.selected().empty());
    REQUIRE_FALSE(model.anchor().has_value());
    REQUIRE(changes == 1);
}

TEST_CASE("SelectionModel retain no-ops on an empty selection", "[widget][selection]") {
    Model model {nandina::widget::SelectionMode::multiple};
    int changes = 0;
    auto sub = model.changed().subscribe([&] { ++changes; });
    REQUIRE_FALSE(model.retain(std::vector<int> {1, 2, 3}));
    REQUIRE(changes == 0);
}

TEST_CASE("SelectionModel range dedupes a duplicated ordered", "[widget][selection]") {
    Model model {nandina::widget::SelectionMode::multiple};
    (void)model.select(5);
    // ordered 含重复键（调用方违约）：跨度 [1,1,2] 去重为 {1,2}，
    // 保持「selected() 是无重复的身份集合」不变量。
    const std::vector<int> duplicated {1, 1, 2};
    REQUIRE(model.select_range(duplicated, 2, 1));
    REQUIRE(model.selected() == std::vector<int> {1, 2});
    REQUIRE(model.count() == 2);
}
