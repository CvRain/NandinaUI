#include "data_table.hpp"

#include "../../render/draw_context.hpp"
#include "../../scene/scene_tree.hpp"
#include "../../theme/nan_style.hpp"
#include "../table.hpp"
#include "item_host.hpp"
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
    namespace
    {
        /// 框架拥有的表头语义 wrapper（role=column_header + aria-sort）。视觉子节点由
        /// IHeaderDelegate 创建/更新；delegate 只替换视觉，不接管焦点/排序/语义。
        class HeaderCellHost final: public scene::NanControl {
        public:
            HeaderCellHost() {
                set_overflow(scene::ControlOverflow::clip);
            }

            /// delegate 身份变化或首次创建 → create_header；否则 update_header。
            /// create/update 失败时保留旧视觉，不留下半挂载节点或 scope。
            void sync(
                const std::shared_ptr<IHeaderDelegate>& delegate,
                const widget::BuildContext& ui,
                const HeaderBinding& binding
            ) {
                if (!delegate) {
                    throw std::invalid_argument("DataTable header delegate must not be null");
                }
                const std::string next_aria = binding.state.sort_direction.has_value()
                    ? (*binding.state.sort_direction == SortDirection::ascending ? "ascending"
                                                                                 : "descending")
                    : std::string {};
                if (aria_sort_ != next_aria) {
                    aria_sort_ = next_aria;
                    mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
                }
                if (!scope_released_ && delegate_ == delegate && visual_) {
                    delegate_->update_header(*visual_, binding);
                    mark_layout_dirty();
                    return;
                }
                auto scope = std::make_unique<reactive::ReactiveScope>(ui.graph());
                auto visual = delegate->create_header(ui.with_scope(*scope), binding);
                if (!visual || visual.get() == this || visual->parent() != nullptr
                    || visual->is_inside_tree() || visual->is_ancestor_of(*this))
                {
                    throw std::logic_error(
                        "DataTable header delegate must return a unique detached control"
                    );
                }
                visual->validate_anchor_parent(*this);
                delegate->update_header(*visual, binding);
                if (visual_) {
                    if (visual_->parent() == this) {
                        remove_and_delete(*visual_);
                    }
                    if (scope_) {
                        scope_->clear();
                    }
                }
                add_child(visual);
                visual_ = std::move(visual);
                scope_ = std::move(scope);
                delegate_ = delegate;
                scope_released_ = false;
                mark_layout_dirty();
            }

            [[nodiscard]] auto visual() const -> scene::NanControl* {
                return visual_.get();
            }

            void release_scope() {
                if (scope_) {
                    scope_->clear();
                }
                scope_released_ = true;
            }

        protected:
            void on_layout() override {
                if (visual_ == nullptr) {
                    return;
                }
                (void)visual_->measure_layout(foundation::NanLayoutConstraints::tight(size()));
                visual_->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, width(), height()));
            }

            [[nodiscard]] auto semantics_properties() const -> semantics::Properties override {
                return {.role = semantics::Role::column_header, .value = aria_sort_};
            }

        private:
            std::shared_ptr<IHeaderDelegate> delegate_;
            std::shared_ptr<scene::NanControl> visual_;
            std::unique_ptr<reactive::ReactiveScope> scope_;
            std::string aria_sort_;
            bool scope_released_ = false;
        };
    } // namespace

    // ─── 表头 host：固定、可聚焦、行语义；视觉由 IHeaderDelegate 提供 ─────────
    class DataTableHeaderHost final: public scene::NanControl {
    public:
        using ClickHandler = std::function<void(const ColumnId&)>;

        DataTableHeaderHost(
            const widget::BuildContext& ui,
            std::shared_ptr<IHeaderDelegate> default_delegate
        ):
            ui_(ui),
            default_delegate_(std::move(default_delegate)),
            theme_(ui.theme()),
            revision_(ui.theme_manager().revision()) {
            set_overflow(scene::ControlOverflow::clip);
        }

        void set_click_handler(ClickHandler handler) {
            click_ = std::move(handler);
        }

        void set_columns(const std::vector<const DataTableColumn*>& columns) {
            const auto previous_focus = focused_column();
            std::unordered_map<ColumnId, std::shared_ptr<HeaderCellHost>> next;
            std::vector<ColumnId> order;
            next.reserve(columns.size());
            order.reserve(columns.size());
            for (const auto* column: columns) {
                if (column == nullptr) {
                    continue;
                }
                const auto found = cells_.find(column->id);
                std::shared_ptr<HeaderCellHost> cell;
                if (found != cells_.end()) {
                    cell = found->second;
                }
                else {
                    cell = std::make_shared<HeaderCellHost>();
                    add_child(cell);
                }
                order.push_back(column->id);
                next.emplace(column->id, std::move(cell));
            }
            for (auto& [id, cell]: cells_) {
                if (!next.contains(id) && cell->parent() == this) {
                    remove_and_delete(*cell);
                }
            }
            cells_ = std::move(next);
            order_ = std::move(order);
            columns_.clear();
            for (const auto* column: columns) {
                if (column != nullptr) {
                    columns_.emplace(column->id, column);
                }
            }
            cell_rects_.assign(order_.size(), foundation::NanRect {});
            focused_index_ = 0;
            if (previous_focus.has_value()) {
                const auto found = std::find(order_.begin(), order_.end(), *previous_focus);
                if (found != order_.end()) {
                    focused_index_ = static_cast<std::size_t>(std::distance(order_.begin(), found));
                }
            }
            for (std::size_t index = 0; index < order_.size(); ++index) {
                const auto found = cells_.find(order_[index]);
                if (found != cells_.end() && found->second->parent() == this) {
                    (void)move_child(*found->second, index);
                }
            }
            sync_cells();
            mark_layout_dirty();
        }

        void set_sort(const std::optional<SortSpec> sort) {
            if (sort_ == sort) {
                return;
            }
            sort_ = sort;
            sync_cells();
            mark_dirty(scene::DirtyFlags::paint);
        }

        /// 主题 revision 变化：只 update 视觉（不重建），delegate 自行重解析 token。
        void set_theme(const theme::NanTheme& theme, const std::uint64_t revision) {
            theme_ = theme;
            revision_ = revision;
            sync_cells();
        }

        void set_cell_rects(std::vector<foundation::NanRect> rects) {
            cell_rects_ = std::move(rects);
            for (std::size_t index = 0; index < order_.size() && index < cell_rects_.size();
                 ++index)
            {
                if (auto found = cells_.find(order_[index]); found != cells_.end()) {
                    (void)found->second->measure_layout(
                        foundation::NanLayoutConstraints::tight(cell_rects_[index].get_size())
                    );
                    found->second->layout_to(cell_rects_[index]);
                }
            }
        }

        void on_exit_tree() override {
            for (auto& [id, cell]: cells_) {
                (void)id;
                if (cell) {
                    cell->release_scope();
                }
            }
            scene::NanControl::on_exit_tree();
        }

        [[nodiscard]] auto node_for(const ColumnId& id) const -> scene::NanControl* {
            const auto found = cells_.find(id);
            return found == cells_.end() ? nullptr : found->second.get();
        }

        [[nodiscard]] auto visible_node_for(const ColumnId& id) const -> scene::NanControl* {
            const auto found = cells_.find(id);
            return found == cells_.end() ? nullptr : found->second->visual();
        }

        [[nodiscard]] auto focused_column() const noexcept -> std::optional<ColumnId> {
            if (focused_index_ >= order_.size()) {
                return std::nullopt;
            }
            return order_[focused_index_];
        }

    protected:
        [[nodiscard]] auto is_focusable() const -> bool override {
            return true;
        }

        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override {
            const bool focused_now = get_tree() != nullptr && get_tree()->focused_node() == this;
            return {
                .role = semantics::Role::row,
                .state = {.focusable = true, .focused = focused_now},
            };
        }

        void on_layout() override {
            // DataTable assigns each header cell its exact column rectangle.
        }

        auto on_input(scene::InputEvent& event) -> bool override {
            if (order_.empty()) {
                return false;
            }
            if (event.type() == scene::EventType::focus_enter
                || event.type() == scene::EventType::focus_leave)
            {
                sync_cells();
                mark_dirty(scene::DirtyFlags::paint);
                return false;
            }
            if (event.type() == scene::EventType::mouse_button) {
                auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
                if (mouse.button() != scene::MouseButtonEvent::Button::left || !mouse.is_pressed())
                {
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
                    focused_index_ = index;
                    if (auto* tree = get_tree()) {
                        tree->set_focus(this);
                    }
                    sync_cells();
                    if (click_) {
                        click_(order_[index]);
                    }
                    mark_dirty(scene::DirtyFlags::paint);
                    event.accept();
                    return true;
                }
                return false;
            }
            if (event.type() != scene::EventType::key) {
                return false;
            }
            auto& key = static_cast<scene::KeyEvent&>(event);
            if (!key.is_press_or_repeat()) {
                return false;
            }
            using foundation::LogicalKey;
            const auto logical = key.logical_key();
            if (logical == LogicalKey::left || logical == LogicalKey::right
                || logical == LogicalKey::home || logical == LogicalKey::end)
            {
                if (logical == LogicalKey::home) {
                    focused_index_ = 0;
                }
                else if (logical == LogicalKey::end) {
                    focused_index_ = order_.size() - 1;
                }
                else if (logical == LogicalKey::left) {
                    focused_index_ = focused_index_ == 0 ? 0 : focused_index_ - 1;
                }
                else {
                    focused_index_ = std::min(focused_index_ + 1, order_.size() - 1);
                }
                sync_cells();
                mark_dirty(scene::DirtyFlags::paint);
                event.accept();
                return true;
            }
            const bool activate = logical == LogicalKey::enter || logical == LogicalKey::space;
            if (!activate || !key.is_pressed()) {
                return false;
            }
            if (focused_index_ >= order_.size()) {
                return false;
            }
            if (click_) {
                click_(order_[focused_index_]);
            }
            event.accept();
            return true;
        }

    private:
        [[nodiscard]] auto binding_for(const DataTableColumn& column) const -> HeaderBinding {
            std::optional<SortDirection> direction;
            if (sort_.has_value() && sort_->column == column.id) {
                direction = sort_->direction;
            }
            const auto focused = focused_column();
            const bool header_focused = get_tree() != nullptr && get_tree()->focused_node() == this;
            return HeaderBinding {
                .column = column,
                .theme = theme_,
                .state = HeaderState {
                    .sort_direction = direction,
                    .focused = header_focused && focused.has_value() && *focused == column.id,
                    .theme_revision = revision_,
                },
            };
        }

        void sync_cells() {
            for (const auto& id: order_) {
                const auto found = columns_.find(id);
                const auto cell = cells_.find(id);
                if (found == columns_.end() || cell == cells_.end()) {
                    continue;
                }
                const auto& delegate = found->second->header_delegate
                    ? found->second->header_delegate
                    : default_delegate_;
                cell->second->sync(delegate, ui_, binding_for(*found->second));
            }
        }

        widget::BuildContext ui_;
        std::shared_ptr<IHeaderDelegate> default_delegate_;
        theme::NanTheme theme_;
        std::uint64_t revision_ = 0;
        ClickHandler click_;
        std::unordered_map<ColumnId, std::shared_ptr<HeaderCellHost>> cells_;
        std::unordered_map<ColumnId, const DataTableColumn*> columns_;
        std::vector<ColumnId> order_;
        std::vector<foundation::NanRect> cell_rects_;
        std::optional<SortSpec> sort_;
        std::size_t focused_index_ = 0;
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
        default_header_delegate_ = std::make_shared<TextHeaderDelegate>();
        header_ = std::make_shared<DataTableHeaderHost>(ui_, default_header_delegate_);
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
        // A window sync queued in the same post-layout batch was deferred until the model
        // ledger was rebuilt; apply it now against the new display order.
        if (pending_window_sync_) {
            pending_window_sync_ = false;
            refresh_window();
        }
    }

    auto DataTable::has_pending_notifications() const noexcept -> bool {
        return pending_reset_ || !pending_ranges_.empty() || pending_scheduled_
            || pending_window_sync_;
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
        scroll_offset_ = clamped_offset(scroll_offset_);
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
        scroll_offset_ = clamped_offset(scroll_offset_);
        mark_layout_dirty();
    }

    auto DataTable::show_header() const noexcept -> bool {
        return show_header_;
    }

    // ─── 纵向视口 / 滚动 ────────────────────────────────────────────────────

    auto DataTable::scroll_offset() const noexcept -> float {
        return scroll_offset_;
    }

    auto DataTable::maximum_scroll_offset() const noexcept -> float {
        if (visible_columns_.empty() || body_viewport_height() <= 0.0F) {
            return 0.0F;
        }
        return window_math().maximum_scroll_offset();
    }

    void DataTable::set_scroll_offset(const float offset) {
        // 显式 offset / 滚轮操作取消尚未落位的 scroll_to_row 目标。
        pending_scroll_row_.reset();
        const float clamped = clamped_offset(offset);
        if (clamped == scroll_offset_) {
            return;
        }
        scroll_offset_ = clamped;
        if (traversal_phase()) {
            // 当前帧先把已物化行平移到位；窗口的增删物化延迟到 post_layout。
            position_window();
            schedule_window_sync();
            return;
        }
        refresh_window();
    }

    void DataTable::scroll_to_row(const std::size_t display_row) {
        if (visible_columns_.empty()) {
            pending_scroll_row_.reset();
            return;
        }
        if (display_row >= display_order_.size()) {
            return;
        }
        if (body_viewport_height() <= 0.0F) {
            pending_scroll_row_ = display_row;
            return;
        }
        pending_scroll_row_.reset();
        if (const auto target = window_math().reveal_offset(display_row, scroll_offset_)) {
            set_scroll_offset(*target);
        }
    }

    // ─── 重建 / 布局 ────────────────────────────────────────────────────────

    void DataTable::retain_selection() {
        std::unordered_set<RowId> rows;
        rows.reserve(model_ ? model_->row_count() : 0);
        if (model_) {
            for (std::size_t row = 0; row < model_->row_count(); ++row) {
                rows.insert(model_->row_id(row));
            }
        }
        std::unordered_set<ColumnId> visible;
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
        if (model_ != nullptr && !visible_columns_.empty()) {
            const std::size_t end = std::min(visible_window_.end, display_order_.size());
            if (end > visible_window_.first) {
                requests.reserve((end - visible_window_.first) * visible_columns_.size());
            }
            for (std::size_t display_row = visible_window_.first; display_row < end; ++display_row)
            {
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
        }
        host_->set_items(requests);
    }

    void DataTable::update_cells_in_ranges(const std::vector<RowRange>& ranges) {
        if (model_ == nullptr || ranges.empty()) {
            return;
        }
        std::vector<ItemHost::ItemRequest> requests;
        const std::size_t end = std::min(visible_window_.end, display_order_.size());
        for (std::size_t display_row = visible_window_.first; display_row < end; ++display_row) {
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
        if (model_ == nullptr || pending_reset_) {
            return;
        }
        std::vector<ItemHost::ItemRequest> requests;
        const std::size_t end = std::min(visible_window_.end, display_order_.size());
        if (end > visible_window_.first) {
            requests.reserve((end - visible_window_.first) * visible_columns_.size());
        }
        for (std::size_t display_row = visible_window_.first; display_row < end; ++display_row) {
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
        refresh_window();
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
        pending_window_sync_ = false;
        if (!model_) {
            display_order_.clear();
            display_row_for_row_.clear();
            visible_columns_ = visible_column_indices();
            visible_window_ = {};
            scroll_offset_ = 0.0F;
            pending_scroll_row_.reset();
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

        // 表头固定在顶部，不参与滚动。
        const float header = header_height();
        if (header_ != nullptr && header > 0.0F) {
            header_->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, table_width, header));
            std::vector<foundation::NanRect> rects;
            rects.reserve(widths.size());
            for (std::size_t visible = 0; visible < widths.size(); ++visible) {
                rects.push_back(
                    foundation::NanRect::from_xywh(
                        column_offsets_[visible],
                        0.0F,
                        widths[visible],
                        header
                    )
                );
            }
            header_->set_cell_rects(std::move(rects));
        }
        else if (header_ != nullptr) {
            header_->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 0.0F, 0.0F));
            header_->set_cell_rects({});
        }

        const auto math = window_math();
        // 首布局前挂起的滚动目标在这里按真实视口高度落位。
        if (pending_scroll_row_.has_value()) {
            if (const auto target = math.reveal_offset(*pending_scroll_row_, scroll_offset_)) {
                scroll_offset_ = *target;
            }
            pending_scroll_row_.reset();
        }
        scroll_offset_ = clamped_offset(scroll_offset_);
        const auto window =
            visible_columns_.empty() ? internal::RowWindow {} : math.visible_window(scroll_offset_);
        const bool window_changed =
            window.first != visible_window_.first || window.end != visible_window_.end;
        position_window();
        navigation_.set_page_rows(visible_window_.count());
        if (window_changed) {
            // 布局遍历期不能增删节点；post_layout 物化后由第二次布局落位。
            schedule_window_sync();
        }
    }

    auto DataTable::is_focusable() const -> bool {
        return true;
    }

    auto DataTable::semantics_properties() const -> semantics::Properties {
        const bool focused_now = get_tree() != nullptr && get_tree()->focused_node() == this;
        return {
            .role = semantics::Role::table,
            .state = {.focusable = true, .focused = focused_now},
        };
    }

    auto DataTable::cell_at_display(
        const std::size_t display_row,
        const std::size_t visible_column
    ) const -> std::optional<CellId> {
        if (model_ == nullptr || pending_reset_ || display_row >= display_order_.size()
            || visible_column >= visible_columns_.size())
        {
            return std::nullopt;
        }
        const std::size_t model_row = display_order_[display_row];
        if (model_row >= model_->row_count()) {
            return std::nullopt;
        }
        return CellId {
            .row = model_->row_id(model_row),
            .column = columns_[visible_columns_[visible_column]].id,
        };
    }

    auto DataTable::display_index_of(const CellId& cell) const -> std::optional<ModelIndex> {
        const auto found = display_row_for_row_.find(cell.row);
        if (found == display_row_for_row_.end()) {
            return std::nullopt;
        }
        for (std::size_t visible = 0; visible < visible_columns_.size(); ++visible) {
            if (columns_[visible_columns_[visible]].id == cell.column) {
                return ModelIndex {.row = found->second, .column = visible};
            }
        }
        return std::nullopt;
    }

    auto DataTable::extent_between(const CellId& anchor, const CellId& target) const
        -> std::vector<CellId> {
        if (model_ == nullptr || pending_reset_) {
            return {target};
        }
        const auto from = display_index_of(anchor);
        const auto to = display_index_of(target);
        if (!from.has_value() || !to.has_value()) {
            return {target};
        }
        const std::size_t row_begin = std::min(from->row, to->row);
        const std::size_t row_end = std::max(from->row, to->row);
        const std::size_t column_begin = std::min(from->column, to->column);
        const std::size_t column_end = std::max(from->column, to->column);
        std::vector<CellId> cells;
        cells.reserve((row_end - row_begin + 1) * (column_end - column_begin + 1));
        for (std::size_t display_row = row_begin; display_row <= row_end; ++display_row) {
            const RowId row = model_->row_id(display_order_[display_row]);
            for (std::size_t visible = column_begin; visible <= column_end; ++visible) {
                cells.push_back(
                    CellId {.row = row, .column = columns_[visible_columns_[visible]].id}
                );
            }
        }
        return cells;
    }

    auto DataTable::hit_cell(const foundation::NanPoint local) const -> std::optional<CellId> {
        if (model_ == nullptr || pending_reset_ || visible_columns_.empty()
            || column_offsets_.size() != visible_columns_.size() + 1)
        {
            return std::nullopt;
        }
        const float body_y = local.get_y() - header_height();
        const auto display_row = window_math().hit_row(body_y, scroll_offset_, visible_window_);
        if (!display_row.has_value() || display_row >= display_order_.size()) {
            return std::nullopt;
        }
        const float x = local.get_x();
        if (x < column_offsets_.front() || x > column_offsets_.back()) {
            return std::nullopt;
        }
        for (std::size_t visible = 0; visible + 1 < column_offsets_.size(); ++visible) {
            const float width = column_offsets_[visible + 1] - column_offsets_[visible];
            if (width > 0.0F && x >= column_offsets_[visible] && x < column_offsets_[visible + 1]) {
                return cell_at_display(*display_row, visible);
            }
        }
        return std::nullopt;
    }

    void DataTable::focus_cell(const CellId& target, const bool extend) {
        if (extend) {
            const auto anchor = selection_.anchor().value_or(target);
            selection_.apply(SelectionOp::extend, target, extent_between(anchor, target));
            return;
        }
        if (selection_.mode() == SelectionMode::single) {
            selection_.apply(SelectionOp::replace, target, {});
            return;
        }
        selection_.set_current(target);
    }

    auto DataTable::handle_key(scene::KeyEvent& key_event) -> bool {
        if (!key_event.is_press_or_repeat() || pending_reset_ || visible_window_.empty()
            || visible_columns_.empty())
        {
            return false;
        }
        using foundation::LogicalKey;
        const auto logical = key_event.logical_key();
        const bool directional = logical == LogicalKey::left || logical == LogicalKey::right
            || logical == LogicalKey::up || logical == LogicalKey::down;
        const bool navigation_key = directional || logical == LogicalKey::home
            || logical == LogicalKey::end || logical == LogicalKey::page_up
            || logical == LogicalKey::page_down;
        if (!navigation_key) {
            return false;
        }
        if (!navigation_.current().has_value()) {
            if (!directional) {
                return false;
            }
            const auto seed = cell_at_display(visible_window_.first, 0);
            if (!seed.has_value()) {
                return false;
            }
            navigation_.set_current(ModelIndex {.row = visible_window_.first, .column = 0});
            selection_.apply(SelectionOp::replace, *seed, {});
            scroll_to_row(visible_window_.first);
            return true;
        }
        bool moved = false;
        if (logical == LogicalKey::left) {
            moved = navigation_.move(GridDirection::left);
        }
        else if (logical == LogicalKey::right) {
            moved = navigation_.move(GridDirection::right);
        }
        else if (logical == LogicalKey::up) {
            moved = navigation_.move(GridDirection::up);
        }
        else if (logical == LogicalKey::down) {
            moved = navigation_.move(GridDirection::down);
        }
        else if (logical == LogicalKey::home) {
            moved = navigation_.home();
        }
        else if (logical == LogicalKey::end) {
            moved = navigation_.end();
        }
        else if (logical == LogicalKey::page_up) {
            moved = navigation_.page_up();
        }
        else {
            moved = navigation_.page_down();
        }
        if (!moved) {
            // The key is in the navigation domain but has no movement at this boundary.
            // Consume it without applying a stale current/anchor operation.
            return true;
        }
        const auto current = navigation_.current();
        if (!current.has_value()) {
            return false;
        }
        const auto target = cell_at_display(current->row, current->column);
        if (!target.has_value()) {
            return false;
        }
        focus_cell(*target, key_event.modifiers().shift);
        scroll_to_row(current->row);
        return true;
    }

    auto DataTable::on_input(scene::InputEvent& event) -> bool {
        if (event.type() == scene::EventType::mouse_button) {
            auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
            if (mouse.button() != scene::MouseButtonEvent::Button::left || !mouse.is_pressed()) {
                return false;
            }
            const auto target = hit_cell(to_local(mouse.screen_pos()));
            if (!target.has_value()) {
                return false;
            }
            if (auto* tree = get_tree()) {
                tree->set_focus(this);
            }
            const auto mods = mouse.modifiers();
            if (mods.shift) {
                const auto anchor = selection_.anchor().value_or(*target);
                selection_.apply(SelectionOp::extend, *target, extent_between(anchor, *target));
            }
            else if (mods.ctrl && selection_.mode() != SelectionMode::single) {
                selection_.apply(SelectionOp::toggle, *target, {});
            }
            else {
                selection_.apply(SelectionOp::replace, *target, {});
            }
            event.accept();
            return true;
        }
        if (event.type() == scene::EventType::key) {
            auto& key = static_cast<scene::KeyEvent&>(event);
            if (!handle_key(key)) {
                return false;
            }
            event.accept();
            return true;
        }
        if (event.type() != scene::EventType::mouse_wheel) {
            return false;
        }
        auto& wheel = static_cast<scene::MouseWheelEvent&>(event);
        const float dy = -wheel.delta().get_y() * wheel_step_;
        const float before = scroll_offset_;
        set_scroll_offset(scroll_offset_ + dy);
        if (scroll_offset_ == before) {
            return false;
        }
        event.accept();
        return true;
    }

    // ─── 窗口 / 视口辅助 ───────────────────────────────────────────────────

    auto DataTable::header_height() const noexcept -> float {
        return show_header_ && !visible_columns_.empty() ? row_height_ : 0.0F;
    }

    auto DataTable::body_viewport_height() const noexcept -> float {
        if (visible_columns_.empty()) {
            return 0.0F;
        }
        return std::max(0.0F, height() - header_height());
    }

    auto DataTable::window_math() const noexcept -> internal::UniformRowWindow {
        return internal::UniformRowWindow {
            row_height_,
            body_viewport_height(),
            display_order_.size(),
        };
    }

    auto DataTable::clamped_offset(const float offset) const noexcept -> float {
        if (visible_columns_.empty() || body_viewport_height() <= 0.0F) {
            return 0.0F;
        }
        return window_math().clamp_offset(offset);
    }

    void DataTable::refresh_window() {
        if (visible_columns_.empty()) {
            pending_scroll_row_.reset();
        }
        const auto math = window_math();
        scroll_offset_ = clamped_offset(scroll_offset_);
        visible_window_ =
            visible_columns_.empty() ? internal::RowWindow {} : math.visible_window(scroll_offset_);
        navigation_.set_page_rows(visible_window_.count());
        materialize_cells();
        position_window();
        mark_layout_dirty();
    }

    void DataTable::schedule_window_sync() {
        if (pending_window_sync_) {
            return;
        }
        pending_window_sync_ = true;
        auto* tree = get_tree();
        if (tree == nullptr) {
            apply_window_sync();
            return;
        }
        std::weak_ptr<DataTable> weak = std::static_pointer_cast<DataTable>(shared_from_this());
        tree->post_layout([weak, tree] {
            if (auto table = weak.lock(); table != nullptr && table->get_tree() == tree) {
                table->apply_window_sync();
            }
        });
    }

    void DataTable::apply_window_sync() {
        // The model callback for the same frame may still be queued behind this action.
        // Rebuild its display ledger first; otherwise a shrink can make this window sync
        // read stale row indices. Also skip when an earlier rebuild already consumed sync.
        if (!pending_window_sync_ || pending_reset_ || !pending_ranges_.empty()
            || pending_scheduled_)
        {
            return;
        }
        pending_window_sync_ = false;
        refresh_window();
    }

    void DataTable::position_window() {
        if (model_ == nullptr || host_ == nullptr) {
            return;
        }
        const float table_width = std::max(0.0F, width());
        host_->layout_to(
            foundation::NanRect::from_xywh(
                0.0F,
                header_height(),
                table_width,
                body_viewport_height()
            )
        );
        if (pending_reset_ || visible_columns_.empty()
            || column_offsets_.size() != visible_columns_.size() + 1)
        {
            return;
        }
        const std::size_t model_count = model_->row_count();
        const std::size_t end = std::min(visible_window_.end, display_order_.size());
        for (std::size_t display_row = visible_window_.first; display_row < end; ++display_row) {
            const std::size_t model_row = display_order_[display_row];
            if (model_row >= model_count) {
                continue;
            }
            const RowId row = model_->row_id(model_row);
            for (std::size_t visible = 0; visible < visible_columns_.size(); ++visible) {
                const auto& column = columns_[visible_columns_[visible]];
                if (auto* node = host_->node_for(CellId {.row = row, .column = column.id});
                    node != nullptr)
                {
                    const auto rect = foundation::NanRect::from_xywh(
                        column_offsets_[visible],
                        static_cast<float>(display_row) * row_height_ - scroll_offset_,
                        column_offsets_[visible + 1] - column_offsets_[visible],
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
        pending_window_sync_ = false;
        if (host_) {
            host_->release_scopes();
        }
        scene::NanControl::on_exit_tree();
    }

    void DataTable::on_theme_changed(const theme::ThemeManager& manager) {
        const auto& theme = manager.theme();
        if (header_) {
            header_->set_theme(theme, manager.revision());
            header_->set_sort(sort_);
        }
        // 作者单元格 delegate 通过新的 ItemState.theme_revision 感知主题变化；
        // Label 等自带主题的控件由场景树自身的主题传播更新。
        refresh_cell_states();
        mark_layout_dirty();
    }
} // namespace nandina::widget
