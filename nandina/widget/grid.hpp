//
// widget/grid — grid layout container.
//
// Children fill cells row by row. Column count is fixed; rows grow as children
// are added. Each column receives an equal fraction of the available main-axis
// space (after gaps). Row heights are the maximum measured height in that row.
// Cells honour LayoutFlexPolicy for sizing and alignment within each cell.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_GRID_HPP
#define NANDINA_EXPERIMENT_WIDGET_GRID_HPP

#include "../scene/control.hpp"
#include "layout.hpp"

#include <cstddef>
#include <memory>

namespace nandina::widget
{

    class Grid: public scene::NanControl {
    public:
        /// Create a grid with the given number of columns.
        explicit Grid(int columns = 2);

        [[nodiscard]] static auto create(int columns = 2) -> std::shared_ptr<Grid>;

        /// Add a child. Children fill the grid row by row, left to right.
        auto add(std::shared_ptr<scene::NanControl> child) -> Grid&;

        /// Set the fixed number of columns.
        auto set_columns(int columns) -> Grid&;

        /// Set the horizontal gap between columns.
        auto set_column_gap(float gap) -> Grid&;

        /// Set the vertical gap between rows.
        auto set_row_gap(float gap) -> Grid&;

        /// Set both gaps at once.
        auto set_gap(float column_gap, float row_gap) -> Grid&;

        /// Set the vertical alignment of children within their row cells.
        auto set_cross_alignment(LayoutAlignment alignment) -> Grid&;

        [[nodiscard]] auto columns() const -> int;
        [[nodiscard]] auto column_gap() const -> float;
        [[nodiscard]] auto row_gap() const -> float;
        [[nodiscard]] auto cross_alignment() const -> LayoutAlignment;

        /// Re-measure and re-layout all children.
        void relayout();

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;
        void on_ready() override;

    private:
        int columns_ = 2;
        float column_gap_ = 0.0F;
        float row_gap_ = 0.0F;
        LayoutAlignment cross_alignment_ = LayoutAlignment::start;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_GRID_HPP
