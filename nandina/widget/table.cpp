// widget/table — static table layout and structural accessibility roles.
#include "table.hpp"

#include "../scene/scene_tree.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        class TablePart final: public scene::NanControl {
        public:
            explicit TablePart(const semantics::Role role): role_(role) {
                set_overflow(scene::ControlOverflow::clip);
            }

        protected:
            auto on_measure(const foundation::NanLayoutConstraints constraints)
                -> foundation::NanSize override {
                if (auto* child = child_count() == 1 ? get_child(0)->as_control() : nullptr) {
                    return child->measure_layout(constraints);
                }
                return constraints.constrain(foundation::NanSize {});
            }
            void on_layout() override {
                // Row hosts are laid out by Table; cell hosts fill their single content slot.
                if (role_ != semantics::Role::row && child_count() == 1) {
                    get_child(0)->as_control()->layout_to(
                        foundation::NanRect::from_xywh(0.0F, 0.0F, width(), height())
                    );
                }
            }
            auto semantics_properties() const -> semantics::Properties override {
                return {.role = role_};
            }

        private:
            semantics::Role role_;
        };

        void require_gap(const float gap) {
            if (!std::isfinite(gap) || gap < 0.0F) {
                throw std::invalid_argument("Table gap must be finite and non-negative");
            }
        }

        /// 行间距总和（gap × (rows-1)）超出 float 可表示范围时拒绝：与列轨道
        /// validate_tracks 的有限和检查对称，避免 set_row_gap(FLT_MAX) + 多行把
        /// 测量高度推成 inf（inf 会沿父级布局扩散）。
        void require_row_gap_total(const float gap, const std::size_t row_count) {
            if (row_count < 2) {
                return;
            }
            const double total = static_cast<double>(gap) * static_cast<double>(row_count - 1);
            if (total > static_cast<double>(std::numeric_limits<float>::max())) {
                throw std::invalid_argument("Table row gap total must be finite");
            }
        }

        auto layout_extent(const double value) -> float {
            if (!std::isfinite(value) || value < 0.0
                || value > static_cast<double>(std::numeric_limits<float>::max()))
            {
                throw std::invalid_argument(
                    "Table content extent must fit finite layout coordinates"
                );
            }
            return static_cast<float>(value);
        }
    } // namespace

    Table::Table(std::vector<TableColumn> columns) {
        set_columns(std::move(columns));
        set_overflow(scene::ControlOverflow::clip);
    }

    auto Table::create(std::vector<TableColumn> columns) -> std::shared_ptr<Table> {
        return std::make_shared<Table>(std::move(columns));
    }

    void Table::validate_tracks(const std::vector<TableColumn>& columns, const float gap) const {
        if (columns.empty()) {
            throw std::invalid_argument("Table requires at least one column");
        }
        double total = static_cast<double>(gap) * static_cast<double>(columns.size() - 1);
        for (const auto& column: columns) {
            if (!std::isfinite(column.flex) || column.flex <= 0.0F
                || (column.width && (!std::isfinite(*column.width) || *column.width < 0.0F)))
            {
                throw std::invalid_argument("Table widths must be non-negative and flex positive");
            }
            total += column.width.value_or(0.0F);
        }
        if (total > std::numeric_limits<float>::max()) {
            throw std::invalid_argument("Table track widths and gaps must have a finite sum");
        }
    }

    void Table::set_columns(std::vector<TableColumn> columns) {
        validate_tracks(columns, column_gap_);
        if ((header_ || !rows_.empty()) && columns.size() != columns_.size()) {
            throw std::logic_error("Table column count cannot change while rows exist");
        }
        columns_ = std::move(columns);
        mark_layout_dirty();
    }

    auto Table::columns() const noexcept -> const std::vector<TableColumn>& {
        return columns_;
    }

    void Table::validate_cells(const std::vector<Cells>& rows) const {
        std::unordered_set<const scene::NanControl*> seen;
        const TablePart prospective_parent(semantics::Role::cell);
        for (const auto& cells: rows) {
            if (cells.size() != columns_.size()) {
                throw std::invalid_argument("Table row size must match the column count");
            }
            for (const auto& cell: cells) {
                if (!cell) {
                    throw std::invalid_argument("Table cell must not be null");
                }
                if (cell.get() == this || cell->is_ancestor_of(*this) || cell->parent()
                    || cell->is_inside_tree() || !seen.insert(cell.get()).second)
                {
                    throw std::logic_error("Table cells must be unique detached controls");
                }
                cell->validate_anchor_parent(prospective_parent);
            }
        }
    }

    auto Table::make_row(Cells cells, const bool header) -> Row {
        Row row {.host = std::make_shared<TablePart>(semantics::Role::row), .content = cells};
        row.slots.reserve(cells.size());
        for (auto& cell: cells) {
            auto slot = std::make_shared<TablePart>(
                header ? semantics::Role::column_header : semantics::Role::cell
            );
            slot->add_child(std::move(cell));
            row.host->add_child(slot);
            row.slots.push_back(std::move(slot));
        }
        return row;
    }

    void Table::set_header(Cells cells) {
        validate_cells({cells});
        require_row_gap_total(row_gap_, rows_.size() + 1);
        auto next = make_row(std::move(cells), true);
        if (header_) {
            remove_and_delete(*header_->host);
        }
        header_ = std::move(next);
        add_child(header_->host);
        content_changed();
    }

    void Table::clear_header() {
        if (!header_) {
            return;
        }
        remove_and_delete(*header_->host);
        header_.reset();
        content_changed();
    }

    auto Table::has_header() const noexcept -> bool {
        return header_.has_value();
    }

    void Table::add_row(Cells cells) {
        validate_cells({cells});
        require_row_gap_total(row_gap_, total_rows() + 1);
        auto row = make_row(std::move(cells), false);
        add_child(row.host);
        rows_.push_back(std::move(row));
        content_changed();
    }

    void Table::set_rows(std::vector<Cells> rows) {
        validate_cells(rows);
        require_row_gap_total(row_gap_, rows.size() + (header_ ? 1 : 0));
        std::vector<Row> next;
        next.reserve(rows.size());
        for (auto& cells: rows) {
            next.push_back(make_row(std::move(cells), false));
        }
        for (const auto& row: rows_) {
            remove_and_delete(*row.host);
        }
        rows_ = std::move(next);
        for (const auto& row: rows_) {
            add_child(row.host);
        }
        content_changed();
    }

    void Table::clear_rows() {
        set_rows({});
    }

    auto Table::row_count() const noexcept -> std::size_t {
        return rows_.size();
    }

    auto Table::total_rows() const noexcept -> std::size_t {
        return rows_.size() + (header_ ? 1 : 0);
    }

    auto Table::header_cell(const std::size_t column) const -> scene::NanControl* {
        if (!header_) {
            throw std::out_of_range("Table has no header");
        }
        return header_->content.at(column).get();
    }

    auto Table::cell(const std::size_t row, const std::size_t column) const -> scene::NanControl* {
        return rows_.at(row).content.at(column).get();
    }

    void Table::set_column_gap(const float gap) {
        require_gap(gap);
        validate_tracks(columns_, gap);
        column_gap_ = gap;
        mark_layout_dirty();
    }

    void Table::set_row_gap(const float gap) {
        require_gap(gap);
        require_row_gap_total(gap, total_rows());
        row_gap_ = gap;
        mark_layout_dirty();
    }

    auto Table::column_gap() const noexcept -> float {
        return column_gap_;
    }

    auto Table::row_gap() const noexcept -> float {
        return row_gap_;
    }

    void Table::order_rows() {
        std::size_t index = 0;
        if (header_) {
            (void)move_child(*header_->host, index++);
        }
        for (const auto& row: rows_) {
            (void)move_child(*row.host, index++);
        }
    }

    void Table::content_changed() {
        mark_layout_dirty();
        mark_semantics_dirty();
        if (auto* tree = get_tree(); tree && tree->defers_tree_mutation()) {
            // add_child can be deferred. Restore semantic row order only after all attachments.
            auto self = std::static_pointer_cast<Table>(shared_from_this());
            tree->defer_tree_mutation([self = std::move(self)] { self->order_rows(); });
        }
        else {
            order_rows();
        }
    }

    auto Table::geometry(const float available_width) -> Geometry {
        Geometry result;
        result.widths.resize(columns_.size(), 0.0F);
        std::vector<const Row*> rows;
        if (header_) {
            rows.push_back(&*header_);
        }
        for (const auto& row: rows_) {
            rows.push_back(&row);
        }
        double reserved = static_cast<double>(column_gap_) * (columns_.size() - 1);
        double weight = 0.0;
        for (std::size_t column = 0; column < columns_.size(); ++column) {
            if (columns_[column].width) {
                result.widths[column] = *columns_[column].width;
                reserved += result.widths[column];
            }
            else {
                weight += columns_[column].flex;
                if (!std::isfinite(available_width)) {
                    for (const auto* row: rows) {
                        result.widths[column] = std::max(
                            result.widths[column],
                            row->slots[column]
                                ->measure_layout(foundation::NanLayoutConstraints::loose())
                                .get_width()
                        );
                    }
                }
            }
        }
        if (std::isfinite(available_width) && weight > 0.0) {
            const double free = std::max(0.0, static_cast<double>(available_width) - reserved);
            for (std::size_t column = 0; column < columns_.size(); ++column) {
                if (!columns_[column].width) {
                    result.widths[column] =
                        static_cast<float>(free * columns_[column].flex / weight);
                }
            }
        }
        double total_width = static_cast<double>(column_gap_) * (columns_.size() - 1);
        for (const auto width: result.widths) {
            total_width += width;
        }
        result.width = layout_extent(total_width);
        double total_height = 0.0;
        for (const auto* row: rows) {
            float height = 0.0F;
            for (std::size_t column = 0; column < columns_.size(); ++column) {
                const float width = result.widths[column];
                height = std::max(
                    height,
                    row->slots[column]
                        ->measure_layout({
                            .min_width = width,
                            .max_width = width,
                        })
                        .get_height()
                );
            }
            result.heights.push_back(height);
            total_height += height;
        }
        if (!rows.empty()) {
            total_height += static_cast<double>(row_gap_) * (rows.size() - 1);
        }
        result.height = layout_extent(total_height);
        return result;
    }

    auto Table::on_measure(const foundation::NanLayoutConstraints constraints)
        -> foundation::NanSize {
        const auto measured = geometry(constraints.max_width);
        intrinsic_width_ = std::isfinite(constraints.max_width)
            ? std::nullopt
            : std::optional<float>(measured.width);
        return constraints.constrain(foundation::NanSize(measured.width, measured.height));
    }

    void Table::on_layout() {
        // A loose parent may arrange us at exactly our intrinsic width. Preserve its
        // intrinsic column allocation instead of switching to weighted tracks in arrange.
        const bool intrinsic = intrinsic_width_ && width() == *intrinsic_width_;
        const auto measured =
            geometry(intrinsic ? std::numeric_limits<float>::infinity() : width());
        std::size_t row_index = 0;
        double top = 0.0;
        const auto arrange = [&](const Row& row) {
            const float height = measured.heights[row_index++];
            row.host->layout_to(
                foundation::NanRect::from_xywh(0.0F, layout_extent(top), measured.width, height)
            );
            double left = 0.0;
            for (std::size_t column = 0; column < columns_.size(); ++column) {
                const float width = measured.widths[column];
                row.slots[column]->layout_to(
                    foundation::NanRect::from_xywh(layout_extent(left), 0.0F, width, height)
                );
                left += static_cast<double>(width) + column_gap_;
            }
            top += static_cast<double>(height) + row_gap_;
        };
        if (header_) {
            arrange(*header_);
        }
        for (const auto& row: rows_) {
            arrange(row);
        }
    }

    auto Table::semantics_properties() const -> semantics::Properties {
        return {.role = semantics::Role::table};
    }
} // namespace nandina::widget
