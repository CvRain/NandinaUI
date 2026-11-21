#include "grid_navigation.hpp"

#include <algorithm>

namespace nandina::widget
{
    auto GridNavigation::is_empty() const noexcept -> bool {
        return rows_ == 0 || columns_ == 0;
    }

    auto GridNavigation::clamp_index(ModelIndex index) const noexcept -> ModelIndex {
        return ModelIndex {
            .row = std::min(index.row, rows_ - 1),
            .column = std::min(index.column, columns_ - 1),
        };
    }

    void GridNavigation::set_shape(const std::size_t rows, const std::size_t columns) noexcept {
        rows_ = rows;
        columns_ = columns;
        if (is_empty()) {
            current_.reset();
            return;
        }
        // 空 → 非空不播种；已有 current 则夹取到新形状内。
        if (current_.has_value()) {
            current_ = clamp_index(*current_);
        }
    }

    void GridNavigation::set_page_rows(const std::size_t rows) noexcept {
        page_rows_ = rows;
    }

    void GridNavigation::set_current(const ModelIndex index) noexcept {
        if (is_empty()) {
            current_.reset();
            return;
        }
        current_ = clamp_index(index);
    }

    void GridNavigation::clear_current() noexcept {
        current_.reset();
    }

    auto GridNavigation::current() const noexcept -> std::optional<ModelIndex> {
        return current_;
    }

    auto GridNavigation::move(const GridDirection direction) noexcept -> bool {
        if (!current_.has_value() || is_empty()) {
            return false;
        }
        ModelIndex next = *current_;
        switch (direction) {
            case GridDirection::up:
                if (next.row == 0) {
                    return false;
                }
                --next.row;
                break;
            case GridDirection::down:
                if (next.row + 1 >= rows_) {
                    return false;
                }
                ++next.row;
                break;
            case GridDirection::left:
                if (next.column == 0) {
                    return false;
                }
                --next.column;
                break;
            case GridDirection::right:
                if (next.column + 1 >= columns_) {
                    return false;
                }
                ++next.column;
                break;
            default:
                return false; // 非法枚举值防御：不移动、不返回 true
        }
        current_ = next;
        return true;
    }

    auto GridNavigation::home() noexcept -> bool {
        if (!current_.has_value() || is_empty()) {
            return false;
        }
        current_->column = 0;
        return true;
    }

    auto GridNavigation::end() noexcept -> bool {
        if (!current_.has_value() || is_empty()) {
            return false;
        }
        current_->column = columns_ - 1;
        return true;
    }

    auto GridNavigation::page_up() noexcept -> bool {
        if (!current_.has_value() || is_empty() || page_rows_ == 0) {
            return false;
        }
        const std::size_t next_row = page_rows_ > current_->row ? 0 : current_->row - page_rows_;
        if (next_row == current_->row) {
            return false;
        }
        current_->row = next_row;
        return true;
    }

    auto GridNavigation::page_down() noexcept -> bool {
        if (!current_.has_value() || is_empty() || page_rows_ == 0) {
            return false;
        }
        const std::size_t remaining = rows_ - 1 - current_->row;
        const std::size_t next_row =
            page_rows_ >= remaining ? rows_ - 1 : current_->row + page_rows_;
        if (next_row == current_->row) {
            return false;
        }
        current_->row = next_row;
        return true;
    }

    auto GridNavigation::scroll_to_index() const noexcept -> std::optional<std::size_t> {
        if (!current_.has_value()) {
            return std::nullopt;
        }
        return current_->row;
    }
} // namespace nandina::widget
