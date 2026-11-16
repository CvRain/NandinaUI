#include "data_table.hpp"

#include "../../render/draw_context.hpp"
#include "../../scene/scene_tree.hpp"
#include "../../theme/nan_style.hpp"
#include "../table.hpp"
#include "track_solver.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace nandina::widget
{
    // ─── 表头：普通 Label + 排序指示图元（不伪造 model_row）──────────────────
    class DataTableHeaderHost final: public scene::NanControl {
    public:
        using ClickHandler = std::function<void(const ColumnId&)>;

        explicit DataTableHeaderHost(const widget::BuildContext& ui):
            graph_(&ui.graph()),
            theme_(ui.theme()),
            indicator_color_(
                theme::resolve_theme_color(
                    ui.theme(),
                    theme::ThemeColor::token(theme::ColorToken::foreground)
                )
            ) {
            set_overflow(scene::ControlOverflow::clip);
        }

        void set_click_handler(ClickHandler handler) {
            click_ = std::move(handler);
        }

        void set_columns(const std::vector<const DataTableColumn*>& columns) {
            std::unordered_map<ColumnId, std::shared_ptr<Label>, internal::ColumnIdHash> next;
            std::vector<ColumnId> order;
            next.reserve(columns.size());
            order.reserve(columns.size());
            for (const auto* column: columns) {
                if (column == nullptr) {
                    continue;
                }
                const auto found = labels_.find(column->id);
                std::shared_ptr<Label> label;
                if (found != labels_.end()) {
                    label = found->second;
                    label->set_text(column->title);
                }
                else {
                    label = Label::create(*graph_, column->title, theme_);
                    label->set_typography_role(theme::TypographyRole::label);
                    label->set_max_lines(1);
                    label->set_overflow(text::TextOverflow::ellipsis);
                    add_child(label);
                }
                order.push_back(column->id);
                next.emplace(column->id, std::move(label));
            }
            for (auto& [id, label]: labels_) {
                if (!next.contains(id) && label->parent() == this) {
                    remove_and_delete(*label);
                }
            }
            labels_ = std::move(next);
            order_ = std::move(order);
            cell_rects_.assign(order_.size(), foundation::NanRect {});
            mark_layout_dirty();
        }

        void set_sort(const std::optional<SortSpec> sort) {
            if (sort_ == sort) {
                return;
            }
            sort_ = sort;
            mark_dirty(scene::DirtyFlags::paint);
        }

        void set_cell_rects(std::vector<foundation::NanRect> rects) {
            cell_rects_ = std::move(rects);
            for (std::size_t index = 0; index < order_.size() && index < cell_rects_.size();
                 ++index)
            {
                if (auto found = labels_.find(order_[index]); found != labels_.end()) {
                    auto& label = *found->second;
                    (void)label.measure_layout(
                        foundation::NanLayoutConstraints::tight(cell_rects_[index].get_size())
                    );
                    label.layout_to(cell_rects_[index]);
                }
            }
        }

        [[nodiscard]] auto node_for(const ColumnId& id) const -> scene::NanControl* {
            const auto found = labels_.find(id);
            return found == labels_.end() ? nullptr : found->second.get();
        }

    protected:
        void on_layout() override {
            // DataTable assigns each visible header label its exact column rectangle.
        }

        auto on_input(scene::InputEvent& event) -> bool override {
            if (event.type() != scene::EventType::mouse_button || order_.empty()) {
                return false;
            }
            auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
            if (mouse.button() != scene::MouseButtonEvent::Button::left || !mouse.is_pressed()) {
                return false;
            }
            const auto local = to_local(mouse.screen_pos());
            for (std::size_t index = 0; index < cell_rects_.size(); ++index) {
                const auto rect = cell_rects_[index];
                if (rect.get_width() <= 0.0F || rect.get_height() <= 0.0F) {
                    continue;
                }
                if (local.get_x() < rect.get_left() || local.get_x() > rect.get_right()
                    || local.get_y() < rect.get_top() || local.get_y() > rect.get_bottom())
                {
                    continue;
                }
                if (click_) {
                    click_(order_[index]);
                }
                event.accept();
                return true;
            }
            return false;
        }

        auto on_draw(render::DrawContext& ctx) -> void override {
            if (!sort_.has_value()) {
                return;
            }
            const auto found = labels_.find(sort_->column);
            if (found == labels_.end()) {
                return;
            }
            const auto* label = found->second.get();
            const auto label_rect = label->local_rect();
            const float center_x = label_rect.get_right() - 10.0F;
            const float center_y = label_rect.get_top() + label_rect.get_height() * 0.5F;
            const float half = 3.5F;
            const bool ascending = sort_->direction == SortDirection::ascending;
            const auto apex =
                foundation::NanPoint(center_x, ascending ? center_y - half : center_y + half);
            const auto left = foundation::NanPoint(
                center_x - half,
                ascending ? center_y + half : center_y - half
            );
            const auto right = foundation::NanPoint(
                center_x + half,
                ascending ? center_y + half : center_y - half
            );
            const auto world = ctx.world_transform();
            const auto color =
                indicator_color_.with_alpha(indicator_color_.alpha() * ctx.opacity());
            const float thickness = ctx.logical_to_screen(1.2F);
            const auto a = world.transform_point(apex);
            const auto b = world.transform_point(left);
            const auto c = world.transform_point(right);
            ctx.device().draw_line(a, b, thickness, color);
            ctx.device().draw_line(b, c, thickness, color);
            ctx.device().draw_line(c, a, thickness, color);
        }

    private:
        reactive::Graph* graph_ = nullptr;
        theme::NanTheme theme_;
        theme::NanColor indicator_color_ {};
        ClickHandler click_;
        std::unordered_map<ColumnId, std::shared_ptr<Label>, internal::ColumnIdHash> labels_;
        std::vector<ColumnId> order_;
        std::vector<foundation::NanRect> cell_rects_;
        std::optional<SortSpec> sort_;
    };

    namespace
    {
        auto row_in_ranges(const std::size_t row, const std::vector<RowRange>& ranges) -> bool {
            return std::ranges::any_of(ranges, [row](const RowRange range) {
                return row >= range.first && row < range.last_exclusive;
            });
        }

        void merge_range(std::vector<RowRange>& ranges, RowRange range) {
            ranges.push_back(range);
            std::ranges::sort(ranges, {}, &RowRange::first);
            std::vector<RowRange> merged;
            merged.reserve(ranges.size());
            for (const auto& current: ranges) {
                if (merged.empty() || current.first > merged.back().last_exclusive) {
                    merged.push_back(current);
                }
                else {
                    merged.back().last_exclusive =
                        std::max(merged.back().last_exclusive, current.last_exclusive);
                }
            }
            ranges = std::move(merged);
        }

        auto pairing_valid(const void* token, const bool has_callback) -> bool {
            return (token != nullptr) == has_callback;
        }

        auto valid_sort_direction(const SortDirection direction) -> bool {
            switch (direction) {
                case SortDirection::ascending:
                case SortDirection::descending:
                    return true;
                default:
                    return false;
            }
        }

        auto valid_sort_reason(const SortRequestReason reason) -> bool {
            switch (reason) {
                case SortRequestReason::user_header_click:
                case SortRequestReason::programmatic:
                    return true;
                default:
                    return false;
            }
        }
    } // namespace

    DataTable::DataTable(
        const widget::BuildContext& ui,
        std::vector<DataTableColumn> columns,
        const SelectionMode selection_mode
    ):
        ui_(ui),
        selection_(selection_mode, SelectionBehavior::cells) {
        set_overflow(scene::ControlOverflow::clip);
        validate_columns(columns, nullptr);
        auto delegates = build_default_delegates(columns);
        columns_ = std::move(columns);
        default_delegates_ = std::move(delegates);
        host_ = std::make_shared<ItemHost>(ui_);
        header_ = std::make_shared<DataTableHeaderHost>(ui_);
        header_->set_click_handler([this](const ColumnId& id) { handle_header_click(id); });
        add_child(header_);
        add_child(host_);
        sort_callback_sub_ = sort_requested_.subscribe([this](const SortRequest& request) {
            if (user_sort_callback_) {
                user_sort_callback_(request);
            }
        });
        selection_sub_ = selection_.changed().subscribe([this] {
            if (updating_selection_) {
                return;
            }
            sync_navigation();
            refresh_cell_states();
        });
        visible_columns_ = visible_column_indices();
        rebuild_all();
    }

    DataTable::~DataTable() = default;

    auto DataTable::create(
        const widget::BuildContext& ui,
        std::vector<DataTableColumn> columns,
        const SelectionMode selection_mode
    ) -> std::shared_ptr<DataTable> {
        return std::make_shared<DataTable>(ui, std::move(columns), selection_mode);
    }

    // ─── 列 / 模型校验 ──────────────────────────────────────────────────────

    void DataTable::validate_columns(
        const std::vector<DataTableColumn>& columns,
        const IRowModel* model
    ) const {
        std::unordered_set<std::string> ids;
        ids.reserve(columns.size());
        std::vector<internal::TrackSpec> tracks;
        tracks.reserve(columns.size());
        for (const auto& column: columns) {
            if (column.id.value.empty()) {
                throw std::invalid_argument("DataTable column id must not be empty");
            }
            if (!ids.insert(column.id.value).second) {
                throw std::invalid_argument("DataTable column ids must be unique");
            }
            if (!pairing_valid(column.value.model_token, static_cast<bool>(column.value.text))) {
                throw std::invalid_argument(
                    "DataTable column value token and accessor must both be present or absent"
                );
            }
            if (!pairing_valid(
                    column.comparator.model_token,
                    static_cast<bool>(column.comparator.compare)
                ))
            {
                throw std::invalid_argument(
                    "DataTable column comparator token and comparator must both be present or absent"
                );
            }
            switch (column.sort) {
                case SortStrategy::local:
                    if (!column.comparator.compare || column.comparator.model_token == nullptr) {
                        throw std::invalid_argument(
                            "DataTable local sort column requires a typed comparator"
                        );
                    }
                    break;
                case SortStrategy::none:
                case SortStrategy::external:
                    if (column.comparator.compare || column.comparator.model_token != nullptr) {
                        throw std::invalid_argument(
                            "DataTable non-local sort column must not carry a comparator"
                        );
                    }
                    break;
                default:
                    throw std::invalid_argument("DataTable column sort strategy is invalid");
            }
            tracks.push_back(
                internal::TrackSpec {
                    .width = column.track.width,
                    .flex = column.track.flex,
                }
            );
            if (model != nullptr) {
                const void* token = model->model_type_token();
                if (column.value.model_token != nullptr && column.value.model_token != token) {
                    throw std::invalid_argument("DataTable column value model type mismatch");
                }
                if (column.comparator.model_token != nullptr
                    && column.comparator.model_token != token)
                {
                    throw std::invalid_argument("DataTable column comparator model type mismatch");
                }
                if (column.delegate != nullptr) {
                    const void* accepted = column.delegate->accepted_model_type_token();
                    if (accepted != nullptr && accepted != token) {
                        throw std::invalid_argument(
                            "DataTable column delegate model type mismatch"
                        );
                    }
                }
            }
        }
        // 空 columns 仅允许出现在构造阶段；只要存在候选列，就在提交前复用静态 Table
        // 的完整轨道规则，避免非法宽度/权重潜伏到首次 layout 才失败。
        if (!tracks.empty()) {
            internal::validate_tracks(tracks, 0.0F);
        }
    }

    auto DataTable::build_default_delegates(const std::vector<DataTableColumn>& columns) const
        -> std::vector<std::shared_ptr<IItemDelegate>> {
        std::vector<std::shared_ptr<IItemDelegate>> delegates;
        delegates.reserve(columns.size());
        for (const auto& column: columns) {
            if (column.delegate) {
                delegates.push_back(column.delegate);
            }
            else {
                delegates.push_back(std::make_shared<TextItemDelegate>(column.value.text));
            }
        }
        return delegates;
    }

    void DataTable::commit_columns(std::vector<DataTableColumn> columns) {
        auto next_delegates = build_default_delegates(columns);
        columns_ = std::move(columns);
        default_delegates_ = std::move(next_delegates);
        if (sort_.has_value()) {
            const auto* sorted_column = find_column(sort_->column);
            if (sorted_column == nullptr || sorted_column->sort == SortStrategy::none) {
                sort_.reset();
            }
        }
        visible_columns_ = visible_column_indices();
        rebuild_all();
    }

    void DataTable::set_columns(std::vector<DataTableColumn> columns) {
        if (columns.empty()) {
            throw std::invalid_argument("DataTable requires at least one column");
        }
        validate_columns(columns, model_.get());
        commit_columns(std::move(columns));
    }

    auto DataTable::columns() const noexcept -> const std::vector<DataTableColumn>& {
        return columns_;
    }

    auto DataTable::column(const ColumnId& id) const noexcept -> const DataTableColumn* {
        return find_column(id);
    }

    void DataTable::set_model(std::shared_ptr<IRowModel> model) {
        if (model.get() == model_.get()) {
            return;
        }
        // 先在旧状态上校验现有 columns：失败时旧 model/columns/排序/控件全不变。
        validate_columns(columns_, model.get());
        unsubscribe_model();
        pending_scheduled_ = false;
        if (host_) {
            // A different model may reuse RowIds with unrelated create-time delegate state.
            host_->reset_items();
        }
        model_ = std::move(model);
        pending_reset_ = false;
        pending_ranges_.clear();
        // 脱离树也可构建/观测（视图契约：只有显式退出树才断订阅）。
        subscribe_model();
        rebuild_all();
    }

    auto DataTable::model() const noexcept -> IRowModel* {
        return model_.get();
    }

    // ─── 通知状态机 ────────────────────────────────────────────────────────

    auto DataTable::traversal_phase() const -> bool {
        const auto* tree = get_tree();
        return tree != nullptr && tree->phase() != scene::FramePhase::idle;
    }

    void DataTable::subscribe_model() {
        if (subscribed_ || !model_) {
            return;
        }
        reset_sub_ = model_->reset_event().subscribe([this] { on_model_reset(); });
        changed_sub_ = model_->data_changed_event().subscribe([this](const RowRange range) {
            on_model_data_changed(range);
        });
        subscribed_ = true;
    }

    void DataTable::unsubscribe_model() {
        reset_sub_.disconnect();
        changed_sub_.disconnect();
        subscribed_ = false;
    }

    void DataTable::on_model_reset() {
        if (traversal_phase()) {
            pending_reset_ = true;
            pending_ranges_.clear();
            schedule_pending();
            return;
        }
        pending_reset_ = false;
        pending_ranges_.clear();
        rebuild_all();
    }

    void DataTable::on_model_data_changed(const RowRange range) {
        if (!traversal_phase()) {
            apply_data_changes({range});
            return;
        }
        if (pending_reset_) {
            return;
        }
        merge_range(pending_ranges_, range);
        schedule_pending();
    }

    void DataTable::schedule_pending() {
        if (pending_scheduled_) {
            return;
        }
        auto* tree = get_tree();
        if (tree == nullptr) {
            apply_pending();
            return;
        }
        pending_scheduled_ = true;
        std::weak_ptr<DataTable> weak = std::static_pointer_cast<DataTable>(shared_from_this());
        tree->post_layout([weak, tree] {
            if (auto table = weak.lock(); table != nullptr && table->get_tree() == tree) {
                table->apply_pending();
            }
        });
    }

    void DataTable::apply_pending() {
        pending_scheduled_ = false;
        if (pending_reset_) {
            pending_reset_ = false;
            pending_ranges_.clear();
            rebuild_all();
            return;
        }
        auto ranges = std::move(pending_ranges_);
        pending_ranges_.clear();
        if (!ranges.empty()) {
            apply_data_changes(ranges);
        }
    }

    auto DataTable::has_pending_notifications() const noexcept -> bool {
        return pending_reset_ || !pending_ranges_.empty() || pending_scheduled_;
    }

    // ─── 排序 ───────────────────────────────────────────────────────────────

    auto DataTable::set_sort(std::optional<SortSpec> sort, const SortRequestReason reason) -> bool {
        if (!valid_sort_reason(reason)) {
            throw std::invalid_argument("DataTable sort request reason is invalid");
        }
        if (!sort.has_value()) {
            if (!sort_.has_value()) {
                return false;
            }
            const auto previous = *sort_;
            sort_.reset();
            const auto* sorted_column = find_column(previous.column);
            const bool external =
                sorted_column != nullptr && sorted_column->sort == SortStrategy::external;
            display_order_.clear();
            if (model_) {
                display_order_.resize(model_->row_count());
                std::iota(display_order_.begin(), display_order_.end(), std::size_t {0});
            }
            sync_derived();
            if (external) {
                emit_sort_request(std::nullopt, reason);
            }
            return true;
        }

        if (!valid_sort_direction(sort->direction)) {
            throw std::invalid_argument("DataTable sort direction is invalid");
        }
        const auto* column = find_column(sort->column);
        if (column == nullptr || column->sort == SortStrategy::none) {
            return false;
        }
        if (sort_.has_value() && *sort_ == *sort) {
            return false;
        }

        if (column->sort == SortStrategy::local) {
            if (!column->comparator.compare) {
                return false;
            }
            if (!model_) {
                sort_ = *sort;
                sync_derived();
                return true;
            }
            std::vector<std::size_t> order(model_->row_count());
            std::iota(order.begin(), order.end(), std::size_t {0});
            const bool ascending = sort->direction == SortDirection::ascending;
            const auto compare = column->comparator.compare;
            // 比较器抛异常时 display_order_/sort_ 尚未改动，旧状态完整保留。
            std::stable_sort(
                order.begin(),
                order.end(),
                [&compare,
                 &model = model_,
                 ascending](const std::size_t lhs, const std::size_t rhs) {
                    const auto result = compare(*model, lhs, rhs);
                    return ascending ? result < 0 : result > 0;
                }
            );
            sort_ = *sort;
            display_order_ = std::move(order);
            sync_derived();
            return true;
        }

        // external：先提交状态，再同步发事件；显示序以模型顺序为准。
        sort_ = *sort;
        display_order_.clear();
        if (model_) {
            display_order_.resize(model_->row_count());
            std::iota(display_order_.begin(), display_order_.end(), std::size_t {0});
        }
        sync_derived();
        emit_sort_request(*sort_, reason);
        return true;
    }

    auto DataTable::sort_spec() const noexcept -> const std::optional<SortSpec>& {
        return sort_;
    }

    auto DataTable::sort_requested() noexcept -> reactive::Event<SortRequest>& {
        return sort_requested_;
    }

    void DataTable::set_on_sort_requested(std::function<void(const SortRequest&)> callback) {
        if (!callback) {
            user_sort_callback_ = {};
            return;
        }
        const auto lifetime = ui_.scope().lifetime();
        user_sort_callback_ = [lifetime,
                               callback = std::move(callback)](const SortRequest& request) {
            if (!lifetime.expired()) {
                callback(request);
            }
        };
    }

    void
    DataTable::emit_sort_request(std::optional<SortSpec> sort, const SortRequestReason reason) {
        SortRequest request {.sort = std::move(sort), .reason = reason};
        sort_requested_.emit(request);
    }

    void DataTable::handle_header_click(const ColumnId& id) {
        const auto* column = find_column(id);
        if (column == nullptr || column->sort == SortStrategy::none) {
            return;
        }
        std::optional<SortSpec> next;
        if (sort_.has_value() && sort_->column == id) {
            next = sort_->direction == SortDirection::ascending
                ? std::optional<SortSpec>(
                      SortSpec {.column = id, .direction = SortDirection::descending}
                  )
                : std::nullopt;
        }
        else {
            next = SortSpec {.column = id, .direction = SortDirection::ascending};
        }
        (void)set_sort(std::move(next), SortRequestReason::user_header_click);
    }

    // ─── 选择 / 导航 / 查询 ─────────────────────────────────────────────────

    auto DataTable::selection() const noexcept -> const TableSelectionModel& {
        return selection_;
    }

    auto DataTable::navigation() const noexcept -> const GridNavigation& {
        return navigation_;
    }

    auto DataTable::apply_selection(
        const SelectionOp op,
        CellId target,
        const std::span<const CellId> extent
    ) -> bool {
        switch (op) {
            case SelectionOp::replace:
            case SelectionOp::toggle:
            case SelectionOp::extend:
                break;
            default:
                throw std::invalid_argument("DataTable selection operation is invalid");
        }
        if (!extent.empty() && std::ranges::find(extent, target) == extent.end()) {
            throw std::invalid_argument("DataTable selection extent must contain the target cell");
        }
        if (!contains_cell(target)
            || !std::ranges::all_of(
                extent,
                [this](const CellId& cell) { return contains_cell(cell); }
            ))
        {
            return false;
        }
        selection_.apply(op, std::move(target), extent);
        return true;
    }

    auto DataTable::set_current_cell(std::optional<CellId> current) -> bool {
        if (current.has_value() && !contains_cell(*current)) {
            return false;
        }
        selection_.set_current(std::move(current));
        return true;
    }

    void DataTable::clear_selection() {
        selection_.clear_selection();
    }

    auto DataTable::find_column(const ColumnId& id) const noexcept -> const DataTableColumn* {
        for (const auto& column: columns_) {
            if (column.id == id) {
                return &column;
            }
        }
        return nullptr;
    }

    auto DataTable::find_column_index(const ColumnId& id) const noexcept
        -> std::optional<std::size_t> {
        for (std::size_t index = 0; index < columns_.size(); ++index) {
            if (columns_[index].id == id) {
                return index;
            }
        }
        return std::nullopt;
    }

    auto DataTable::visible_column_indices() const -> std::vector<std::size_t> {
        std::vector<std::size_t> visible;
        for (std::size_t index = 0; index < columns_.size(); ++index) {
            if (columns_[index].visible) {
                visible.push_back(index);
            }
        }
        return visible;
    }

    auto DataTable::row_count() const noexcept -> std::size_t {
        return display_order_.size();
    }

    auto DataTable::column_count() const noexcept -> std::size_t {
        return visible_columns_.size();
    }

    auto DataTable::item_count() const noexcept -> std::size_t {
        return host_ ? host_->item_count() : 0;
    }

    auto DataTable::cell_node(const CellId& id) const -> scene::NanControl* {
        return host_ ? host_->node_for(id) : nullptr;
    }

    auto DataTable::header_node(const ColumnId& id) const -> scene::NanControl* {
        return header_ ? header_->node_for(id) : nullptr;
    }

    auto DataTable::model_row_for_display(const std::size_t display_row) const
        -> std::optional<std::size_t> {
        if (display_row >= display_order_.size()) {
            return std::nullopt;
        }
        return display_order_[display_row];
    }

    auto DataTable::display_row_for(const RowId row) const -> std::optional<std::size_t> {
        const auto found = display_row_for_row_.find(row);
        if (found == display_row_for_row_.end()) {
            return std::nullopt;
        }
        return found->second;
    }

    void DataTable::set_row_height(const float row_height) {
        if (!std::isfinite(row_height) || row_height <= 0.0F) {
            throw std::invalid_argument("DataTable row height must be finite and positive");
        }
        row_height_ = row_height;
        mark_layout_dirty();
    }

    auto DataTable::row_height() const noexcept -> float {
        return row_height_;
    }

    void DataTable::set_show_header(const bool show_header) noexcept {
        if (show_header_ == show_header) {
            return;
        }
        show_header_ = show_header;
        mark_layout_dirty();
    }

    auto DataTable::show_header() const noexcept -> bool {
        return show_header_;
    }

    // ─── 重建 / 布局 ────────────────────────────────────────────────────────

    void DataTable::retain_selection() {
        std::unordered_set<RowId, internal::RowIdHash> rows;
        rows.reserve(model_ ? model_->row_count() : 0);
        if (model_) {
            for (std::size_t row = 0; row < model_->row_count(); ++row) {
                rows.insert(model_->row_id(row));
            }
        }
        std::unordered_set<ColumnId, internal::ColumnIdHash> visible;
        visible.reserve(visible_columns_.size());
        for (const auto index: visible_columns_) {
            visible.insert(columns_[index].id);
        }
        selection_.retain([&rows, &visible](const CellId& cell) {
            return rows.contains(cell.row) && visible.contains(cell.column);
        });
    }

    void DataTable::sync_navigation() {
        navigation_.set_shape(display_order_.size(), visible_columns_.size());
        const auto current = selection_.current();
        if (!current.has_value()) {
            navigation_.clear_current();
            return;
        }
        const auto display_row = display_row_for(current->row);
        if (!display_row.has_value()) {
            navigation_.clear_current();
            return;
        }
        for (std::size_t visible = 0; visible < visible_columns_.size(); ++visible) {
            if (columns_[visible_columns_[visible]].id == current->column) {
                navigation_.set_current(ModelIndex {.row = *display_row, .column = visible});
                return;
            }
        }
        navigation_.clear_current();
    }

    auto DataTable::contains_cell(const CellId& cell) const -> bool {
        if (!display_row_for_row_.contains(cell.row)) {
            return false;
        }
        return std::ranges::any_of(visible_columns_, [this, &cell](const std::size_t index) {
            return columns_[index].id == cell.column;
        });
    }

    auto DataTable::item_state_for(const CellId& cell) const -> ItemState {
        return ItemState {
            .selected = selection_.is_selected(cell),
            .current = selection_.current() == std::optional<CellId>(cell),
            .hovered = false,
            .enabled = true,
            .theme_revision = ui_.theme_manager().revision(),
        };
    }

    void DataTable::materialize_cells() {
        std::vector<ItemHost::ItemRequest> requests;
        if (!model_) {
            host_->set_items(requests);
            return;
        }
        const std::size_t rows = display_order_.size();
        requests.reserve(rows * visible_columns_.size());
        for (std::size_t display_row = 0; display_row < rows; ++display_row) {
            const std::size_t model_row = display_order_[display_row];
            const RowId row = model_->row_id(model_row);
            for (std::size_t visible = 0; visible < visible_columns_.size(); ++visible) {
                const std::size_t column_index = visible_columns_[visible];
                const auto& column = columns_[column_index];
                const CellId cell {.row = row, .column = column.id};
                requests.push_back(
                    ItemHost::ItemRequest {
                        .id = cell,
                        .model = model_.get(),
                        .model_row = model_row,
                        .display_index = ModelIndex {.row = display_row, .column = visible},
                        .row = row,
                        .column = &column.id,
                        .state = item_state_for(cell),
                        .delegate = default_delegates_[column_index],
                    }
                );
            }
        }
        host_->set_items(requests);
    }

    void DataTable::update_cells_in_ranges(const std::vector<RowRange>& ranges) {
        if (!model_ || ranges.empty()) {
            return;
        }
        std::vector<ItemHost::ItemRequest> requests;
        for (std::size_t display_row = 0; display_row < display_order_.size(); ++display_row) {
            const std::size_t model_row = display_order_[display_row];
            if (!row_in_ranges(model_row, ranges)) {
                continue;
            }
            const RowId row = model_->row_id(model_row);
            for (std::size_t visible = 0; visible < visible_columns_.size(); ++visible) {
                const std::size_t column_index = visible_columns_[visible];
                const auto& column = columns_[column_index];
                const CellId cell {.row = row, .column = column.id};
                requests.push_back(
                    ItemHost::ItemRequest {
                        .id = cell,
                        .model = model_.get(),
                        .model_row = model_row,
                        .display_index = ModelIndex {.row = display_row, .column = visible},
                        .row = row,
                        .column = &column.id,
                        .state = item_state_for(cell),
                        .delegate = default_delegates_[column_index],
                    }
                );
            }
        }
        if (!requests.empty()) {
            host_->update_items(requests);
            mark_layout_dirty();
        }
    }

    void DataTable::apply_data_changes(const std::vector<RowRange>& ranges) {
        if (!model_ || ranges.empty()) {
            return;
        }
        if (sort_.has_value()) {
            const auto* column = find_column(sort_->column);
            if (column != nullptr && column->sort == SortStrategy::local) {
                // data_changed carries no changed-column information, so any local sort must
                // rebuild from model order and re-run the comparator before refreshing cells.
                display_order_.resize(model_->row_count());
                std::iota(display_order_.begin(), display_order_.end(), std::size_t {0});
                apply_sort_to_display();
                sync_derived();
                return;
            }
        }
        update_cells_in_ranges(ranges);
    }

    void DataTable::apply_sort_to_display() {
        if (!sort_.has_value() || !model_) {
            return;
        }
        const auto* column = find_column(sort_->column);
        if (column == nullptr || column->sort != SortStrategy::local || !column->comparator.compare)
        {
            return;
        }
        const bool ascending = sort_->direction == SortDirection::ascending;
        const auto compare = column->comparator.compare;
        // reset 触发的重排不承诺回滚：比较器抛异常时保留模型顺序。
        try {
            std::stable_sort(
                display_order_.begin(),
                display_order_.end(),
                [&compare,
                 &model = model_,
                 ascending](const std::size_t lhs, const std::size_t rhs) {
                    const auto result = compare(*model, lhs, rhs);
                    return ascending ? result < 0 : result > 0;
                }
            );
        }
        catch (...) {
            std::iota(display_order_.begin(), display_order_.end(), std::size_t {0});
        }
    }

    void DataTable::refresh_cell_states() {
        if (!model_) {
            return;
        }
        std::vector<ItemHost::ItemRequest> requests;
        requests.reserve(display_order_.size() * visible_columns_.size());
        for (std::size_t display_row = 0; display_row < display_order_.size(); ++display_row) {
            const std::size_t model_row = display_order_[display_row];
            const RowId row = model_->row_id(model_row);
            for (std::size_t visible = 0; visible < visible_columns_.size(); ++visible) {
                const std::size_t column_index = visible_columns_[visible];
                const auto& column = columns_[column_index];
                const CellId cell {.row = row, .column = column.id};
                requests.push_back(
                    ItemHost::ItemRequest {
                        .id = cell,
                        .model = model_.get(),
                        .model_row = model_row,
                        .display_index = ModelIndex {.row = display_row, .column = visible},
                        .row = row,
                        .column = &column.id,
                        .state = item_state_for(cell),
                        .delegate = default_delegates_[column_index],
                    }
                );
            }
        }
        if (!requests.empty()) {
            host_->update_items(requests);
            mark_layout_dirty();
        }
    }

    void DataTable::sync_derived() {
        display_row_for_row_.clear();
        display_row_for_row_.reserve(display_order_.size());
        for (std::size_t display_row = 0; display_row < display_order_.size(); ++display_row) {
            if (model_) {
                display_row_for_row_.emplace(
                    model_->row_id(display_order_[display_row]),
                    display_row
                );
            }
        }
        visible_columns_ = visible_column_indices();
        updating_selection_ = true;
        retain_selection();
        updating_selection_ = false;
        sync_navigation();
        materialize_cells();
        if (header_) {
            std::vector<const DataTableColumn*> visible;
            visible.reserve(visible_columns_.size());
            for (const auto index: visible_columns_) {
                visible.push_back(&columns_[index]);
            }
            header_->set_columns(visible);
            header_->set_sort(sort_);
        }
        mark_layout_dirty();
    }

    void DataTable::rebuild_all() {
        // A structural rebuild reads the model's current state. Any older queued range/reset
        // belongs to the previous snapshot; discard it rather than applying stale row indexes.
        pending_scheduled_ = false;
        pending_reset_ = false;
        pending_ranges_.clear();
        if (!model_) {
            display_order_.clear();
            display_row_for_row_.clear();
            visible_columns_ = visible_column_indices();
            host_->reset_items();
            selection_.retain([](const CellId&) { return false; });
            navigation_.set_shape(0, visible_columns_.size());
            sync_derived();
            return;
        }
        display_order_.resize(model_->row_count());
        std::iota(display_order_.begin(), display_order_.end(), std::size_t {0});
        apply_sort_to_display();
        sync_derived();
    }

    auto DataTable::column_widths(const float available_width) const -> std::vector<float> {
        if (visible_columns_.empty()) {
            return {};
        }
        std::vector<internal::TrackSpec> tracks;
        tracks.reserve(visible_columns_.size());
        for (const auto index: visible_columns_) {
            tracks.push_back(
                internal::TrackSpec {
                    .width = columns_[index].track.width,
                    .flex = columns_[index].track.flex,
                }
            );
        }
        std::vector<float> intrinsic(tracks.size(), 0.0F);
        std::optional<float> bounded;
        if (std::isfinite(available_width)) {
            bounded = available_width;
        }
        else {
            for (std::size_t index = 0; index < tracks.size(); ++index) {
                intrinsic[index] = tracks[index].width ? *tracks[index].width : 160.0F;
            }
        }
        const auto solved = internal::solve_tracks(tracks, bounded, intrinsic, 0.0F);
        return solved.widths;
    }

    auto DataTable::on_measure(const foundation::NanLayoutConstraints constraints)
        -> foundation::NanSize {
        const auto widths = column_widths(constraints.max_width);
        const float content_width = std::accumulate(widths.begin(), widths.end(), 0.0F);
        double content_height = 0.0;
        if (!visible_columns_.empty()) {
            if (show_header_) {
                content_height += row_height_;
            }
            content_height +=
                static_cast<double>(display_order_.size()) * static_cast<double>(row_height_);
        }
        if (!std::isfinite(content_width) || content_width < 0.0F || !std::isfinite(content_height)
            || content_height < 0.0
            || content_height > static_cast<double>(std::numeric_limits<float>::max()))
        {
            throw std::invalid_argument(
                "DataTable content extent must fit finite layout coordinates"
            );
        }
        return constraints.constrain(
            foundation::NanSize(content_width, static_cast<float>(content_height))
        );
    }

    auto DataTable::on_layout() -> void {
        const float table_width = std::max(0.0F, width());
        const auto widths = column_widths(table_width);
        column_offsets_.assign(widths.size() + 1, 0.0F);
        for (std::size_t index = 0; index < widths.size(); ++index) {
            column_offsets_[index + 1] = column_offsets_[index] + widths[index];
        }
        const float header_height = show_header_ && !visible_columns_.empty() ? row_height_ : 0.0F;
        if (header_height > 0.0F) {
            header_->layout_to(
                foundation::NanRect::from_xywh(0.0F, 0.0F, table_width, header_height)
            );
            std::vector<foundation::NanRect> rects;
            rects.reserve(widths.size());
            for (std::size_t visible = 0; visible < widths.size(); ++visible) {
                rects.push_back(
                    foundation::NanRect::from_xywh(
                        column_offsets_[visible],
                        0.0F,
                        widths[visible],
                        header_height
                    )
                );
            }
            header_->set_cell_rects(std::move(rects));
        }
        else {
            header_->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 0.0F, 0.0F));
            header_->set_cell_rects({});
        }
        const float host_height = visible_columns_.empty()
            ? 0.0F
            : static_cast<float>(display_order_.size()) * row_height_;
        host_->layout_to(
            foundation::NanRect::from_xywh(0.0F, header_height, table_width, host_height)
        );
        for (std::size_t display_row = 0; display_row < display_order_.size(); ++display_row) {
            const RowId row = model_->row_id(display_order_[display_row]);
            for (std::size_t visible = 0; visible < visible_columns_.size(); ++visible) {
                const auto& column = columns_[visible_columns_[visible]];
                if (auto* node = host_->node_for(CellId {.row = row, .column = column.id});
                    node != nullptr)
                {
                    const auto rect = foundation::NanRect::from_xywh(
                        column_offsets_[visible],
                        static_cast<float>(display_row) * row_height_,
                        widths[visible],
                        row_height_
                    );
                    (void)node->measure_layout(
                        foundation::NanLayoutConstraints::tight(rect.get_size())
                    );
                    node->layout_to(rect);
                }
            }
        }
    }

    // ─── 树生命周期 ────────────────────────────────────────────────────────

    void DataTable::on_enter_tree() {
        scene::NanControl::on_enter_tree();
        subscribe_model();
    }

    void DataTable::on_ready() {
        scene::NanControl::on_ready();
        rebuild_all();
    }

    void DataTable::on_exit_tree() {
        unsubscribe_model();
        pending_scheduled_ = false;
        pending_reset_ = false;
        pending_ranges_.clear();
        if (host_) {
            host_->release_scopes();
        }
        scene::NanControl::on_exit_tree();
    }
} // namespace nandina::widget
