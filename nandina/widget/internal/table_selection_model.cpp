#include "table_selection_model.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace nandina::widget
{
    TableSelectionModel::TableSelectionModel(
        const SelectionMode mode,
        const SelectionBehavior behavior
    ):
        mode_(mode),
        behavior_(behavior) {
        if (behavior_ != SelectionBehavior::cells) {
            throw std::invalid_argument(
                "TableSelectionModel supports only SelectionBehavior::cells in batch 2A"
            );
        }
    }

    auto TableSelectionModel::behavior() const noexcept -> SelectionBehavior {
        return behavior_;
    }

    auto TableSelectionModel::mode() const noexcept -> SelectionMode {
        return mode_;
    }

    auto TableSelectionModel::current() const -> std::optional<CellId> {
        return current_;
    }

    auto TableSelectionModel::anchor() const -> std::optional<CellId> {
        return anchor_;
    }

    auto TableSelectionModel::is_selected(const CellId& cell) const -> bool {
        return selected_index_.contains(cell);
    }

    auto TableSelectionModel::selected() const -> const std::vector<CellId>& {
        return selected_;
    }

    auto TableSelectionModel::changed() const -> const reactive::Event<>& {
        return changed_;
    }

    void TableSelectionModel::commit(
        std::vector<CellId> next_selected,
        std::optional<CellId> next_current,
        std::optional<CellId> next_anchor
    ) {
        const bool selection_changed = next_selected != selected_;
        if (!selection_changed && next_current == current_ && next_anchor == anchor_) {
            return;
        }
        if (selection_changed) {
            // Build the replacement membership index before mutating selected_; a failed
            // allocation must leave the observable selection and index pair unchanged.
            std::unordered_set<CellId, internal::CellIdHash> next_index;
            next_index.reserve(next_selected.size());
            for (const auto& cell: next_selected) {
                next_index.insert(cell);
            }
            selected_ = std::move(next_selected);
            selected_index_ = std::move(next_index);
        }
        current_ = std::move(next_current);
        anchor_ = std::move(next_anchor);
        changed_.emit();
    }

    void TableSelectionModel::clear_selection() {
        if (selected_.empty() && !anchor_.has_value()) {
            return;
        }
        selected_.clear();
        selected_index_.clear();
        anchor_.reset();
        changed_.emit();
    }

    void TableSelectionModel::set_current(std::optional<CellId> current) {
        // current 不参与选择集合；直接改，不复制/重建 selected。
        if (current_ == current) {
            return;
        }
        current_ = std::move(current);
        changed_.emit();
    }

    void TableSelectionModel::retain(const std::function<bool(const CellId&)>& keep) {
        // 强异常保证：keep 可能在遍历中抛异常，所有 keep 调用完成前不改任何成员。
        std::vector<CellId> next_selected;
        next_selected.reserve(selected_.size());
        for (const auto& cell: selected_) {
            if (keep(cell)) {
                next_selected.push_back(cell);
            }
        }
        std::optional<CellId> next_current = current_;
        if (next_current.has_value() && !keep(*next_current)) {
            next_current.reset();
        }
        std::optional<CellId> next_anchor = anchor_;
        if (next_anchor.has_value() && !keep(*next_anchor)) {
            next_anchor.reset();
        }
        commit(std::move(next_selected), std::move(next_current), std::move(next_anchor));
    }

    auto TableSelectionModel::normalize_extent(
        const CellId& target,
        const std::span<const CellId> extent
    ) const -> std::vector<CellId> {
        std::vector<CellId> normalized;
        normalized.reserve(extent.size() + 1);
        if (extent.empty()) {
            normalized.push_back(target);
            return normalized;
        }
        std::unordered_set<CellId, internal::CellIdHash> seen;
        seen.reserve(extent.size());
        for (const auto& cell: extent) {
            if (seen.insert(cell).second) {
                normalized.push_back(cell);
            }
        }
        return normalized;
    }

    void TableSelectionModel::apply(
        const SelectionOp op,
        const CellId target,
        const std::span<const CellId> extent
    ) {
        switch (op) {
            case SelectionOp::replace:
            case SelectionOp::toggle:
            case SelectionOp::extend:
                break;
            default:
                throw std::invalid_argument("TableSelectionModel apply operation is invalid");
        }

        if (!extent.empty() && std::ranges::find(extent, target) == extent.end()) {
            throw std::invalid_argument("TableSelectionModel extent must contain the target cell");
        }

        std::vector<CellId> next_selected;
        std::optional<CellId> next_anchor;
        if (mode_ == SelectionMode::single) {
            // 单选：忽略 extent，只保留 target；toggle 对已选 target 做取消选中。
            // anchor 始终是 shift 起点（可指向未选中单元），取消选中时同样落到 target。
            if (op == SelectionOp::toggle && is_selected(target)) {
                next_selected = {};
            }
            else {
                next_selected = {target};
            }
            next_anchor = target;
        }
        else {
            switch (op) {
                case SelectionOp::replace:
                    next_selected = normalize_extent(target, extent);
                    next_anchor = target;
                    break;
                case SelectionOp::toggle:
                    next_selected = selected_;
                    if (is_selected(target)) {
                        std::erase(next_selected, target);
                    }
                    else {
                        next_selected.push_back(target);
                    }
                    next_anchor = target;
                    break;
                case SelectionOp::extend:
                    next_selected = normalize_extent(target, extent);
                    next_anchor = anchor_.has_value() ? anchor_ : std::optional<CellId>(target);
                    break;
                default:
                    throw std::invalid_argument("TableSelectionModel apply operation is invalid");
            }
        }
        // current 始终跟随操作目标（键盘/程序化入口），与 anchor 的"shift 起点"职责分离。
        commit(std::move(next_selected), std::optional<CellId>(target), std::move(next_anchor));
    }
} // namespace nandina::widget
