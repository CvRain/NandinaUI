//
// widget/roving_focus — 组内键盘漫游与 typeahead 的实现。
//

#include "roving_focus.hpp"

#include "key_codes.hpp"

#include "../scene/input_event.hpp"

#include <algorithm>
#include <cctype>

namespace nandina::widget
{
    namespace
    {
        /// 成员数量变化时要把活动项夹回合法范围。
        [[nodiscard]] auto clamp_index(const int index, const std::size_t count) -> int {
            if (count == 0) {
                return -1;
            }
            if (index < 0) {
                return 0;
            }
            const auto size = static_cast<int>(count);
            return index >= size ? size - 1 : index;
        }

        /// 标签是否以缓冲开头（ASCII 大小写不敏感）。
        [[nodiscard]] auto starts_with_ci(const std::string_view label, const std::string& prefix)
            -> bool {
            if (prefix.empty() || label.size() < prefix.size()) {
                return false;
            }
            for (std::size_t i = 0; i < prefix.size(); ++i) {
                const auto lhs = static_cast<unsigned char>(label[i]);
                const auto rhs = static_cast<unsigned char>(prefix[i]);
                if (std::tolower(lhs) != std::tolower(rhs)) {
                    return false;
                }
            }
            return true;
        }
    } // namespace

    void RovingFocus::sync(
        const std::size_t count,
        const std::function<bool(std::size_t)>& accepts_focus,
        const std::function<std::string_view(std::size_t)>& label
    ) {
        members_.resize(count);
        for (std::size_t i = 0; i < count; ++i) {
            members_[i].accepts_focus = accepts_focus ? accepts_focus(i) : true;
            // 拷贝而不是 std::move：同一个回调要分发给所有成员。
            members_[i].label = label;
        }
        // 成员增删后活动项可能越界或落在不可聚焦项上，夹回最近的合法位置。
        active_ = clamp_index(active_, members_.size());
        if (!focusable(static_cast<std::size_t>(active_))) {
            const auto first = edge_focusable(false);
            active_ = first;
        }
    }

    auto RovingFocus::focusable(const std::size_t index) const -> bool {
        return index < members_.size() && members_[index].accepts_focus;
    }

    auto RovingFocus::active_index() const -> int {
        if (!has_members()) {
            return -1;
        }
        return clamp_index(active_, members_.size());
    }

    void RovingFocus::set_active_index(const int index) {
        active_ = clamp_index(index, members_.size());
    }

    auto RovingFocus::label_of(const std::size_t index) const -> std::string_view {
        if (index >= members_.size() || !members_[index].label) {
            return {};
        }
        return members_[index].label(index);
    }

    void RovingFocus::set_typeahead_timeout(const float seconds) noexcept {
        typeahead_timeout_ = seconds > 0.0F ? seconds : 0.0F;
    }

    void RovingFocus::reset_typeahead() {
        buffer_.clear();
        typeahead_elapsed_ = 0.0F;
    }

    void RovingFocus::advance_time(const float dt) {
        if (buffer_.empty()) {
            return;
        }
        typeahead_elapsed_ += std::max(0.0F, dt);
        if (typeahead_elapsed_ > typeahead_timeout_) {
            reset_typeahead();
        }
    }

    auto RovingFocus::next_focusable(const int from, const int direction) const -> int {
        if (!has_members() || direction == 0) {
            return -1;
        }
        const auto size = static_cast<int>(members_.size());
        // 起点可能越界（例如成员刚被删掉），先夹回范围再走一步。
        int current = clamp_index(from < 0 ? (direction > 0 ? -1 : size) : from, members_.size());
        for (int step = 0; step < size; ++step) {
            current += direction;
            if (current < 0 || current >= size) {
                if (!loop_) {
                    return -1;
                }
                current = current < 0 ? size - 1 : 0;
            }
            if (focusable(static_cast<std::size_t>(current))) {
                return current;
            }
        }
        return -1;
    }

    auto RovingFocus::edge_focusable(const bool from_end) const -> int {
        if (!has_members()) {
            return -1;
        }
        const auto size = static_cast<int>(members_.size());
        for (int step = 0; step < size; ++step) {
            const int index = from_end ? size - 1 - step : step;
            if (focusable(static_cast<std::size_t>(index))) {
                return index;
            }
        }
        return -1;
    }

    auto RovingFocus::typeahead_match() const -> int {
        if (!has_members() || buffer_.empty()) {
            return -1;
        }
        const auto size = static_cast<int>(members_.size());
        const int start = std::max(0, active_index());
        // 从当前项之后开始找，这样"重复按同一个键"能在同首字母的成员间轮转。
        for (int step = 1; step <= size; ++step) {
            const int index = (start + step) % size;
            if (!focusable(static_cast<std::size_t>(index))) {
                continue;
            }
            if (starts_with_ci(label_of(static_cast<std::size_t>(index)), buffer_)) {
                return index;
            }
        }
        return -1;
    }

    auto RovingFocus::make_intent(const int index) const -> Intent {
        return Intent {
            .index = index,
            .move_widget_focus = movement_ == RovingMovement::widget_focus,
        };
    }

    auto RovingFocus::handle_key(const scene::KeyEvent& event) -> std::optional<Intent> {
        if (!event.is_pressed() || !has_members()) {
            return std::nullopt;
        }
        const bool vertical = orientation_ != RovingOrientation::horizontal;
        const bool horizontal = orientation_ != RovingOrientation::vertical;

        int target = -1;
        switch (event.keycode()) {
            case keys::up:
                if (!vertical) {
                    return std::nullopt;
                }
                target = next_focusable(active_index(), -1);
                break;
            case keys::down:
                if (!vertical) {
                    return std::nullopt;
                }
                target = next_focusable(active_index(), 1);
                break;
            case keys::left:
                if (!horizontal) {
                    return std::nullopt;
                }
                target = next_focusable(active_index(), -1);
                break;
            case keys::right:
                if (!horizontal) {
                    return std::nullopt;
                }
                target = next_focusable(active_index(), 1);
                break;
            case keys::home:
                target = edge_focusable(false);
                break;
            case keys::end:
                target = edge_focusable(true);
                break;
            case keys::page_up:
                // 没有可见项高度信息，用"跳到首个 / 末尾"作为等价语义。
                target = edge_focusable(false);
                break;
            case keys::page_down:
                target = edge_focusable(true);
                break;
            default:
                return std::nullopt;
        }

        if (target < 0) {
            return std::nullopt;
        }
        active_ = target;
        // 方向键属于导航，输入新键即结束上一次 typeahead 查找。
        reset_typeahead();
        return make_intent(target);
    }

    auto RovingFocus::handle_text(const scene::TextInputEvent& event)
        -> std::optional<Intent> {
        if (!has_members()) {
            return std::nullopt;
        }
        const auto text = event.text();
        // 只吃单个可打印字符；组合键与多字符输入（IME 提交）留给组件自己处理。
        if (text.size() != 1) {
            return std::nullopt;
        }
        const auto character = static_cast<unsigned char>(text.front());
        if (std::iscntrl(character) != 0 || std::isspace(character) != 0) {
            return std::nullopt;
        }

        const bool repeating = buffer_.size() == 1 && buffer_.front() == text.front();
        if (repeating) {
            // 同一字符重复：在同首字母的成员间轮转，缓冲不增长。
            const int target = typeahead_match();
            if (target < 0) {
                return std::nullopt;
            }
            active_ = target;
            typeahead_elapsed_ = 0.0F;
            return make_intent(target);
        }

        buffer_.append(text.data(), text.size());
        int target = typeahead_match();
        if (target < 0 && buffer_.size() > 1) {
            // 累积后的缓冲不再是任何成员的前缀（例如先后按了 b、a）。此时重新以最新
            // 字符开始查找，而不是把缓冲留着 —— 否则一次误按会让 typeahead 卡死，
            // 后续输入全部无解。
            buffer_.assign(1, text.front());
            target = typeahead_match();
        }
        typeahead_elapsed_ = 0.0F;
        if (target < 0) {
            // 单字符也无匹配：保留缓冲（继续输入仍可能命中），但不移动活动项。
            return std::nullopt;
        }
        active_ = target;
        return make_intent(target);
    }
} // namespace nandina::widget
