//
// roving_focus_tests — 组内键盘漫游与 typeahead。
//
// 「一组同类条目、同一时刻只有一个当前项」是 RadioGroup / Tabs / Select 弹出列表 /
// 后续 Menu 的共同形态。RovingFocus 把这个模型抽出来，只回答焦点归属；本文件固定
// 它的契约，包括两种移动模式的差别与导航键的可达范围。
//

#include <nandina/scene/input_event.hpp>
#include <nandina/widget/key_codes.hpp>
#include <nandina/widget/roving_focus.hpp>

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>
#include <vector>

using namespace nandina;

namespace
{
    constexpr int kItemCount = 5;

    [[nodiscard]] auto press(int keycode) -> scene::KeyEvent {
        return scene::KeyEvent(keycode, scene::KeyEvent::Action::press);
    }

    [[nodiscard]] auto released(int keycode) -> scene::KeyEvent {
        return scene::KeyEvent(keycode, scene::KeyEvent::Action::release);
    }

    [[nodiscard]] auto typed(std::string text) -> scene::TextInputEvent {
        return scene::TextInputEvent(std::move(text));
    }

    /// 五个成员，可选地禁用其中若干，并带上用于 typeahead 的标签。
    struct Fixture {
        std::vector<bool> enabled = {true, true, true, true, true};
        std::vector<std::string> labels = {"alpha", "bravo", "charlie", "avocado", "delta"};
        widget::RovingFocus focus;

        Fixture() {
            const auto& enabled_ref = enabled;
            const auto& labels_ref = labels;
            focus.sync(
                enabled_ref.size(),
                [&enabled_ref](std::size_t index) { return enabled_ref[index]; },
                [&labels_ref](std::size_t index) -> std::string_view {
                    return labels_ref[index];
                }
            );
        }

        /// 按一次键并返回目标索引；未被识别时返回 -1。
        [[nodiscard]] auto key(int keycode) -> int {
            const auto intent = focus.handle_key(press(keycode));
            return intent.has_value() ? intent->index : -1;
        }

        [[nodiscard]] auto text(const std::string& value) -> int {
            const auto intent = focus.handle_text(typed(value));
            return intent.has_value() ? intent->index : -1;
        }
    };
} // namespace

TEST_CASE("arrow keys roam and wrap across members", "[widget][roving]") {
    Fixture f;

    REQUIRE(f.focus.active_index() == 0);
    REQUIRE(f.key(widget::keys::down) == 1);
    REQUIRE(f.key(widget::keys::down) == 2);
    REQUIRE(f.key(widget::keys::up) == 1);

    // 末尾继续向下：环绕到首个成员。
    f.focus.set_active_index(kItemCount - 1);
    REQUIRE(f.key(widget::keys::down) == 0);
    // 首个继续向上：环绕到末尾。
    f.focus.set_active_index(0);
    REQUIRE(f.key(widget::keys::up) == kItemCount - 1);
}

TEST_CASE("loop can be disabled at the boundary", "[widget][roving]") {
    Fixture f;
    f.focus.set_loop(false);

    f.focus.set_active_index(0);
    REQUIRE(f.key(widget::keys::up) == -1);
    REQUIRE(f.focus.active_index() == 0);

    f.focus.set_active_index(kItemCount - 1);
    REQUIRE(f.key(widget::keys::down) == -1);
    REQUIRE(f.focus.active_index() == kItemCount - 1);
}

TEST_CASE("orientation filters which arrows are accepted", "[widget][roving]") {
    SECTION("horizontal ignores up and down") {
        Fixture f;
        f.focus.set_orientation(widget::RovingOrientation::horizontal);
        REQUIRE(f.key(widget::keys::up) == -1);
        REQUIRE(f.key(widget::keys::down) == -1);
        REQUIRE(f.key(widget::keys::right) == 1);
    }

    SECTION("vertical ignores left and right") {
        Fixture f;
        f.focus.set_orientation(widget::RovingOrientation::vertical);
        REQUIRE(f.key(widget::keys::left) == -1);
        REQUIRE(f.key(widget::keys::right) == -1);
        REQUIRE(f.key(widget::keys::down) == 1);
    }

    SECTION("both accepts every arrow") {
        Fixture f;
        f.focus.set_orientation(widget::RovingOrientation::both);
        REQUIRE(f.key(widget::keys::right) == 1);
        REQUIRE(f.key(widget::keys::down) == 2);
        REQUIRE(f.key(widget::keys::left) == 1);
        REQUIRE(f.key(widget::keys::up) == 0);
    }
}

TEST_CASE("movement mode decides whether widget focus follows", "[widget][roving]") {
    SECTION("selection only") {
        Fixture f;
        f.focus.set_movement(widget::RovingMovement::selection_only);
        const auto intent = f.focus.handle_key(press(widget::keys::down));
        REQUIRE(intent.has_value());
        REQUIRE(intent->index == 1);
        REQUIRE_FALSE(intent->move_widget_focus);
    }

    SECTION("widget focus") {
        Fixture f;
        f.focus.set_movement(widget::RovingMovement::widget_focus);
        const auto intent = f.focus.handle_key(press(widget::keys::down));
        REQUIRE(intent.has_value());
        REQUIRE(intent->index == 1);
        REQUIRE(intent->move_widget_focus);
    }
}

TEST_CASE("home, end and page keys jump to the edges", "[widget][roving]") {
    Fixture f;

    REQUIRE(f.key(widget::keys::end) == kItemCount - 1);
    REQUIRE(f.key(widget::keys::home) == 0);
    REQUIRE(f.key(widget::keys::page_down) == kItemCount - 1);
    REQUIRE(f.key(widget::keys::page_up) == 0);
}

TEST_CASE("disabled members are skipped", "[widget][roving]") {
    Fixture f;
    // 1 和 2 不可聚焦。
    f.enabled = {true, false, false, true, true};
    {
        const auto& enabled_ref = f.enabled;
        f.focus.sync(
            enabled_ref.size(),
            [&enabled_ref](std::size_t index) { return enabled_ref[index]; }
        );
    }

    REQUIRE(f.key(widget::keys::down) == 3);
    REQUIRE(f.key(widget::keys::up) == 0);
    // 首尾跳转也要跳过禁用项。
    REQUIRE(f.key(widget::keys::end) == 4);
    REQUIRE(f.key(widget::keys::home) == 0);
    REQUIRE(f.focus.active_index() == 0);
}

TEST_CASE("typeahead matches labels and accumulates a prefix", "[widget][roving]") {
    Fixture f;

    // 单字符跳转：停在 alpha 时按 b -> bravo。
    REQUIRE(f.text("b") == 1);
    // 查找从**当前项之后**开始（重复按键才能轮转），所以 b 之后再按 a 会跳到
    // bravo 之后的 avocado，而不是回到前面的 alpha。
    REQUIRE(f.text("a") == 3);
    // 前缀累积：已到达 alpha，再按 l 命中 alpha 的 "al" 前缀。
    f.focus.set_active_index(0);
    REQUIRE(f.text("a") == 3);
    REQUIRE(f.text("l") == 0);
    REQUIRE(f.focus.typeahead_buffer() == "al");
}

TEST_CASE("typeahead cycles on a repeated character and resets on timeout", "[widget][roving]") {
    Fixture f;

    // a -> alpha（当前位置 0 之后的第一个 a 开头项是 avocado）。
    const int first = f.text("a");
    REQUIRE(first == 3);
    // 再次按 a：在同首字母项之间轮转。
    const int second = f.text("a");
    REQUIRE(second == 0);

    // 超时后缓冲清空。
    f.focus.set_typeahead_timeout(0.1F);
    f.focus.advance_time(0.2F);
    REQUIRE(f.focus.typeahead_buffer().empty());
}

TEST_CASE("member churn keeps the active index valid", "[widget][roving]") {
    Fixture f;
    f.focus.set_active_index(4);

    // 缩短成员表：活动项必须被夹回范围，不能指向已不存在的成员。
    f.focus.sync(2);
    REQUIRE(f.focus.active_index() == 1);

    // 清空：没有可用成员。
    f.focus.sync(0);
    REQUIRE(f.focus.active_index() == -1);
    REQUIRE(f.focus.handle_key(press(widget::keys::down)) == std::nullopt);
    REQUIRE(f.focus.handle_text(typed("a")) == std::nullopt);
}

TEST_CASE("non navigation keys are not consumed", "[widget][roving]") {
    Fixture f;

    // Enter / Space / Escape 归组件自己，roving focus 不得吞掉。
    REQUIRE(f.focus.handle_key(press(widget::keys::enter)) == std::nullopt);
    REQUIRE(f.focus.handle_key(press(widget::keys::space)) == std::nullopt);
    REQUIRE(f.focus.handle_key(press(widget::keys::escape)) == std::nullopt);
    REQUIRE(f.focus.handle_key(press(widget::keys::tab)) == std::nullopt);
    // 未识别的字符不移动活动项。
    REQUIRE(f.focus.handle_text(typed("z")) == std::nullopt);
    REQUIRE(f.focus.active_index() == 0);
    // 释放事件不触发导航。
    REQUIRE(f.focus.handle_key(released(widget::keys::down)) == std::nullopt);
}
