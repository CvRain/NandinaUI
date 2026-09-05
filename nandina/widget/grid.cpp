//
// widget/grid — grid layout container implementation.
//

#include "grid.hpp"

#include "../scene/scene_tree.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace nandina::widget
{
    namespace
    {
        [[nodiscard]] auto collect_controls(scene::NanNode& parent)
            -> std::vector<std::shared_ptr<scene::NanControl>> {
            std::vector<std::shared_ptr<scene::NanControl>> result;
            result.reserve(parent.child_count());
            for (std::size_t i = 0; i < parent.child_count(); ++i) {
                auto* node = parent.get_child(i);
                auto* control = node != nullptr ? node->as_control() : nullptr;
                if (control != nullptr && control->visible()) {
                    result.push_back(
                        std::static_pointer_cast<scene::NanControl>(node->shared_from_this())
                    );
                }
            }
            return result;
        }

        [[nodiscard]] auto cell_constraints(scene::LayoutConstraints parent, float cell_width)
            -> scene::LayoutConstraints {
            return {
                .min_width = 0.0F,
                .max_width = std::max(0.0F, cell_width),
                .min_height = 0.0F,
                .max_height = parent.max_height,
            };
        }

        [[nodiscard]] auto alignment_offset(LayoutAlignment alignment, float available, float used)
            -> float {
            const auto remaining = std::max(0.0F, available - used);
            switch (alignment) {
                case LayoutAlignment::center:
                    return remaining * 0.5F;
                case LayoutAlignment::end:
                    return remaining;
                case LayoutAlignment::stretch:
                case LayoutAlignment::start:
                case LayoutAlignment::space_between:
                default:
                    return 0.0F;
            }
        }
    } // namespace

    Grid::Grid(const int columns): columns_(std::max(1, columns)) {}

    auto Grid::create(const int columns) -> std::shared_ptr<Grid> {
        return std::make_shared<Grid>(columns);
    }

    auto Grid::add(std::shared_ptr<scene::NanControl> child) -> Grid& {
        NanControl::add_child(std::move(child));
        mark_layout_dirty();
        return *this;
    }

    auto Grid::set_columns(const int columns) -> Grid& {
        columns_ = std::max(1, columns);
        mark_layout_dirty();
        return *this;
    }

    auto Grid::set_column_gap(const float gap) -> Grid& {
        column_gap_ = std::max(0.0F, gap);
        mark_layout_dirty();
        return *this;
    }

    auto Grid::set_row_gap(const float gap) -> Grid& {
        row_gap_ = std::max(0.0F, gap);
        mark_layout_dirty();
        return *this;
    }

    auto Grid::set_gap(const float column_gap, const float row_gap) -> Grid& {
        column_gap_ = std::max(0.0F, column_gap);
        row_gap_ = std::max(0.0F, row_gap);
        mark_layout_dirty();
        return *this;
    }

    auto Grid::set_cross_alignment(const LayoutAlignment alignment) -> Grid& {
        cross_alignment_ = alignment;
        mark_layout_dirty();
        return *this;
    }

    auto Grid::columns() const -> int {
        return columns_;
    }

    auto Grid::column_gap() const -> float {
        return column_gap_;
    }

    auto Grid::row_gap() const -> float {
        return row_gap_;
    }

    auto Grid::cross_alignment() const -> LayoutAlignment {
        return cross_alignment_;
    }

    void Grid::relayout() {
        mark_layout_dirty();
        if (!is_inside_tree()) {
            return;
        }
        const auto current = get_tree()->layout_root(size());
    }

    auto Grid::on_measure(const scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto children = collect_controls(*this);
        if (children.empty() || columns_ <= 0) {
            const auto empty = foundation::NanSize(
                column_gap_ * static_cast<float>(std::max(0, columns_ - 1)),
                row_gap_ * static_cast<float>(std::max(0, columns_ - 1))
            );
            return constraints.constrain(empty);
        }

        const int total_children = static_cast<int>(children.size());
        const int row_count = (total_children + columns_ - 1) / columns_; // ceil division

        // Total gap space
        const float total_column_gap = column_gap_ * static_cast<float>(std::max(0, columns_ - 1));
        const float total_row_gap = row_gap_ * static_cast<float>(std::max(0, row_count - 1));

        // Available space per cell
        const float available_width = std::max(0.0F, constraints.max_width - total_column_gap);
        const float cell_width = available_width / static_cast<float>(columns_);

        // Measure each child within its cell width
        std::vector<float> column_widths(static_cast<std::size_t>(columns_), 0.0F);
        std::vector<float> row_heights(static_cast<std::size_t>(row_count), 0.0F);

        for (int i = 0; i < total_children; ++i) {
            const int col = i % columns_;
            const int row = i / columns_;
            const auto child_constraints = cell_constraints(constraints, cell_width);
            const auto measured =
                children[static_cast<std::size_t>(i)]->measure_layout(child_constraints);
            column_widths[static_cast<std::size_t>(col)] =
                std::max(column_widths[static_cast<std::size_t>(col)], measured.get_width());
            row_heights[static_cast<std::size_t>(row)] =
                std::max(row_heights[static_cast<std::size_t>(row)], measured.get_height());
        }

        float total_width = total_column_gap;
        for (const auto w: column_widths) {
            total_width += w;
        }
        float total_height = total_row_gap;
        for (const auto h: row_heights) {
            total_height += h;
        }

        return constraints.constrain(foundation::NanSize(total_width, total_height));
    }

    auto Grid::on_layout() -> void {
        const auto children = collect_controls(*this);
        if (children.empty() || columns_ <= 0) {
            return;
        }

        const auto container_size = size();
        const int total_children = static_cast<int>(children.size());
        const int row_count = (total_children + columns_ - 1) / columns_;

        const float total_column_gap = column_gap_ * static_cast<float>(std::max(0, columns_ - 1));

        const float available_width = std::max(0.0F, container_size.get_width() - total_column_gap);
        const float cell_width = available_width / static_cast<float>(columns_);

        // Measure to get row heights
        std::vector<float> row_heights(static_cast<std::size_t>(row_count), 0.0F);
        for (int i = 0; i < total_children; ++i) {
            const int row = i / columns_;
            const auto child_constraints =
                cell_constraints(scene::LayoutConstraints::loose(), cell_width);
            const auto measured =
                children[static_cast<std::size_t>(i)]->measure_layout(child_constraints);
            row_heights[static_cast<std::size_t>(row)] =
                std::max(row_heights[static_cast<std::size_t>(row)], measured.get_height());
        }

        // Layout each child
        float y = 0.0F;
        for (int row = 0; row < row_count; ++row) {
            const float row_height = row_heights[static_cast<std::size_t>(row)];
            float x = 0.0F;
            const int cols_in_row = std::min(columns_, total_children - row * columns_);

            for (int col = 0; col < cols_in_row; ++col) {
                const int idx = row * columns_ + col;
                auto& child = children[static_cast<std::size_t>(idx)];

                // Allow children to be smaller than the cell on the cross axis
                // so alignment (center/end) takes effect.
                const auto child_constraints = scene::LayoutConstraints {
                    .min_width = 0.0F,
                    .max_width = cell_width,
                    .min_height = 0.0F,
                    .max_height = row_height,
                };
                const auto measured = child->measure_layout(child_constraints);

                const float child_y =
                    alignment_offset(cross_alignment_, row_height, measured.get_height());

                child->layout_to(
                    foundation::NanRect::from_xywh(
                        x,
                        y + child_y,
                        cell_width,
                        measured.get_height()
                    )
                );
                x += cell_width + column_gap_;
            }
            y += row_height + row_gap_;
        }
    }

    void Grid::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

} // namespace nandina::widget
