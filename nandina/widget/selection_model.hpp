//
// widget/selection_model — generic identity-based selection state for data components.
//
// 数据组件（列表、表格等）共享的选中状态依据：以稳定 Key 为身份，不持有控件、
// 不依赖布局或窗口，可脱离场景树单测。规则见
// docs/references/data_components_and_virtualization.md §选择模型。
//
// 与 MenuSelection 的分工：MenuSelection 面向菜单条目（checkbox/radio kind、disabled
// 规则、MenuItem::checked 唯一事实来源）；SelectionModel 面向数据行，身份是键而不是
// 条目对象，并承担「范围选择 / 数据替换时保留合法成员」这两件菜单不需要的事。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_SELECTION_MODEL_HPP
#define NANDINA_EXPERIMENT_WIDGET_SELECTION_MODEL_HPP

#include "../reactive/event.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

namespace nandina::widget
{
    /// 数据组件的选择语义。
    enum class SelectionMode : std::uint8_t {
        /// 单选：任何一次 select / 范围选择都只保留一个成员。
        single,
        /// 多选：成员独立累加，范围选择替换为跨度内全部成员。
        multiple,
    };

    /**
     * 以稳定 Key 为身份的选中状态。selected() 保持「首次选中顺序」而非键序，
     * 便于组件按用户选择次序消费；范围选择按 ordered 参数中的位置展开。
     */
    template<std::equality_comparable Key>
    class SelectionModel {
    public:
        explicit SelectionModel(SelectionMode mode = SelectionMode::single) noexcept: mode_(mode) {}

        /**
         * 切换模式。multiple → single 只保留第一个成员；裁剪后若锚点指向已移除的
         * 成员，重指向保留的首成员（保持「锚点 = 已选中成员」不变量）。
         * 变化时触发一次事件；会运行订阅方回调，因此不标 noexcept。
         */
        void set_mode(const SelectionMode mode) {
            if (mode_ == mode) {
                return;
            }
            mode_ = mode;
            bool changed = false;
            if (mode == SelectionMode::single && selected_.size() > 1) {
                selected_.erase(selected_.begin() + 1, selected_.end());
                changed = true;
            }
            if (anchor_ && std::ranges::find(selected_, *anchor_) == selected_.end()) {
                anchor_ = selected_.empty() ? std::nullopt : std::optional<Key>(selected_.front());
                changed = true;
            }
            if (changed) {
                changed_.emit();
            }
        }
        [[nodiscard]] auto mode() const noexcept -> SelectionMode {
            return mode_;
        }

        /**
         * 选中一个成员。single 模式替换为 {key}；multiple 模式追加。
         * 已选中时返回 false 且不触发事件。更新范围选择锚点。
         */
        auto select(const Key& key) -> bool {
            if (mode_ == SelectionMode::single) {
                if (selected_.size() == 1 && selected_.front() == key) {
                    return false;
                }
                selected_.assign(1, key);
            }
            else {
                if (contains(key)) {
                    return false;
                }
                selected_.push_back(key);
            }
            anchor_ = key;
            changed_.emit();
            return true;
        }

        /**
         * 切换一个成员。single 模式：已选则清空，未选则选中；
         * multiple 模式：在选中与未选中之间翻转。返回是否发生变化。
         */
        auto toggle(const Key& key) -> bool {
            const auto found = std::ranges::find(selected_, key);
            if (found != selected_.end()) {
                selected_.erase(found);
                if (anchor_ && *anchor_ == key) {
                    anchor_.reset();
                }
                changed_.emit();
                return true;
            }
            return select(key);
        }

        /**
         * 清空选中。返回是否发生变化；变化时触发事件并清空锚点。
         */
        auto clear() -> bool {
            if (selected_.empty()) {
                return false;
            }
            selected_.clear();
            anchor_.reset();
            changed_.emit();
            return true;
        }

        /**
         * 范围选择：把 ordered 中 from 与 to（含两端）之间的成员设为选中。
         *
         * single 模式等价于 select(to)；multiple 模式把现有选中**替换**为跨度内
         * 全部成员（跨度按 ordered 位置展开，保持行序）。from / to 不在 ordered 中
         * 时返回 false 且不改动任何状态。更新锚点为 to。
         *
         * ordered 的键应当唯一（VirtualListView::set_source 已保证）；含重复键时
         * 跨度会防御性去重（保留首次出现序），维持「selected() 是无重复的身份集合」
         * 不变量。返回值只表示选中集合是否变化：幂等重复的范围选择不触发事件，
         * 但锚点仍更新为 to。
         */
        auto select_range(const std::vector<Key>& ordered, const Key& from, const Key& to) -> bool {
            if (mode_ == SelectionMode::single) {
                // single 等价于 select(to)，但仍要校验 from/to 都在 ordered 中：
                // 契约要求未知键不修改任何状态。
                if (std::ranges::find(ordered, from) == ordered.end()
                    || std::ranges::find(ordered, to) == ordered.end())
                {
                    return false;
                }
                return select(to);
            }
            if (ordered.empty()) {
                return false;
            }
            const auto from_it = std::ranges::find(ordered, from);
            const auto to_it = std::ranges::find(ordered, to);
            if (from_it == ordered.end() || to_it == ordered.end()) {
                return false;
            }
            const auto lo = std::min(from_it, to_it);
            const auto hi = std::max(from_it, to_it);
            std::vector<Key> next(lo, hi + 1);
            if (next == selected_) {
                // 跨度集合不变：不触发事件，但落点（to）仍要成为新锚点 ——
                // shift 点击在两个端点间往返时，锚点跟随最后一次落点。
                if (anchor_ != to) {
                    anchor_ = to;
                }
                return false;
            }
            // 存储前防御性去重（ordered 违约含重复键时保持身份集合语义）：
            // 只在实际落盘路径上跑一次 O(n) 哈希去重，正常调用方无额外开销。
            std::unordered_set<Key> seen;
            seen.reserve(next.size());
            std::vector<Key> deduped;
            deduped.reserve(next.size());
            for (auto& key: next) {
                if (seen.insert(key).second) {
                    deduped.push_back(key);
                }
            }
            selected_ = std::move(deduped);
            anchor_ = to;
            changed_.emit();
            return true;
        }

        /**
         * 数据替换后保留仍在 universe 中的选中成员；其余移除。
         * 返回是否发生了变化（移除或清空锚点也算）。
         */
        auto retain(const std::vector<Key>& universe) -> bool {
            bool removed = false;
            std::erase_if(selected_, [&](const Key& key) {
                const bool keep = std::ranges::find(universe, key) != universe.end();
                removed = removed || !keep;
                return !keep;
            });
            if (anchor_ && std::ranges::find(universe, *anchor_) == universe.end()) {
                anchor_.reset();
                removed = true;
            }
            if (removed) {
                changed_.emit();
            }
            return removed;
        }

        [[nodiscard]] auto contains(const Key& key) const -> bool {
            return std::ranges::find(selected_, key) != selected_.end();
        }
        [[nodiscard]] auto count() const noexcept -> std::size_t {
            return selected_.size();
        }
        /// 首次选中顺序的成员列表。
        [[nodiscard]] auto selected() const noexcept -> const std::vector<Key>& {
            return selected_;
        }
        [[nodiscard]] auto first_selected() const -> std::optional<Key> {
            if (selected_.empty()) {
                return std::nullopt;
            }
            return std::optional<Key>(selected_.front());
        }
        /// 最近一次且仍被选中的落点，供 shift 范围扩展使用；toggle 移除锚点成员时清空。
        [[nodiscard]] auto anchor() const noexcept -> const std::optional<Key>& {
            return anchor_;
        }

        /// 选中状态被用户操作改变时触发（无载荷：订阅方重新读取 selected()）。
        [[nodiscard]] auto changed() const noexcept -> const reactive::Event<>& {
            return changed_;
        }

    private:
        SelectionMode mode_;
        std::vector<Key> selected_;
        std::optional<Key> anchor_;
        reactive::Event<> changed_;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_SELECTION_MODEL_HPP
