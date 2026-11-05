// widget/table — shared column tracks and natural row heights for static content.
#ifndef NANDINA_EXPERIMENT_WIDGET_TABLE_HPP
#define NANDINA_EXPERIMENT_WIDGET_TABLE_HPP

#include "../scene/control.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace nandina::widget
{
    /// Fixed logical width when present; otherwise a weighted share of finite width,
    /// or the maximum intrinsic cell width when measured without a width bound.
    struct TableColumn {
        std::optional<float> width;
        float flex = 1.0F;
    };

    /// Static, non-focusable table. Content owns its typography and interaction.
    /// Rows are fully materialized; sorting, selection and virtualization belong to DataTable.
    class Table final: public scene::NanControl {
    public:
        using Cells = std::vector<std::shared_ptr<scene::NanControl>>;

        explicit Table(std::vector<TableColumn> columns = {{}, {}});
        [[nodiscard]] static auto create(std::vector<TableColumn> columns = {{}, {}})
            -> std::shared_ptr<Table>;

        /// Changing the column count requires an empty table (including its header).
        void set_columns(std::vector<TableColumn> columns);
        [[nodiscard]] auto columns() const noexcept -> const std::vector<TableColumn>&;
        void set_header(Cells cells);
        void clear_header();
        [[nodiscard]] auto has_header() const noexcept -> bool;
        void add_row(Cells cells);
        void set_rows(std::vector<Cells> rows);
        void clear_rows();
        [[nodiscard]] auto row_count() const noexcept -> std::size_t;
        /// Original content nodes; indexes outside the current data throw out_of_range.
        [[nodiscard]] auto header_cell(std::size_t column) const -> scene::NanControl*;
        [[nodiscard]] auto cell(std::size_t row, std::size_t column) const -> scene::NanControl*;

        void set_column_gap(float gap);
        void set_row_gap(float gap);
        [[nodiscard]] auto column_gap() const noexcept -> float;
        [[nodiscard]] auto row_gap() const noexcept -> float;

    protected:
        [[nodiscard]] auto on_measure(foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        struct Row {
            std::shared_ptr<scene::NanControl> host;
            Cells slots;
            Cells content;
        };
        struct Geometry {
            std::vector<float> widths;
            std::vector<float> heights;
            float width = 0.0F;
            float height = 0.0F;
        };

        void validate_cells(const std::vector<Cells>& rows) const;
        [[nodiscard]] auto make_row(Cells cells, bool header) -> Row;
        [[nodiscard]] auto geometry(float available_width) -> Geometry;
        [[nodiscard]] auto total_rows() const noexcept -> std::size_t;
        void order_rows();
        void content_changed();
        void validate_tracks(const std::vector<TableColumn>& columns, float gap) const;

        std::vector<TableColumn> columns_;
        std::optional<Row> header_;
        std::vector<Row> rows_;
        float column_gap_ = 0.0F;
        float row_gap_ = 0.0F;
        std::optional<float> intrinsic_width_;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_TABLE_HPP
