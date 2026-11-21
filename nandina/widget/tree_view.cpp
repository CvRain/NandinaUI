#include "tree_view.hpp"

#include "../render/draw_context.hpp"
#include "../theme/theme.hpp"
#include "internal/item_host.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace nandina::widget
{
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

        auto make_request(
            IRowModel& model,
            const std::shared_ptr<IItemDelegate>& delegate,
            const std::size_t model_row,
            const RowId id,
            const std::size_t display_row,
            const ItemState state
        ) -> ItemHost::ItemRequest {
            return ItemHost::ItemRequest {
                .id = CellId {.row = id, .column = ColumnId {}},
                .model = &model,
                .model_row = model_row,
                .display_index = ModelIndex {.row = display_row, .column = 0},
                .row = id,
                .column = nullptr,
                .state = state,
                .delegate = delegate,
            };
        }

        class ScopedSelectionRefresh {
        public:
            explicit ScopedSelectionRefresh(bool& flag): flag_(flag) {
                flag_ = true;
            }
            ~ScopedSelectionRefresh() {
                flag_ = false;
            }

            ScopedSelectionRefresh(const ScopedSelectionRefresh&) = delete;
            auto operator=(const ScopedSelectionRefresh&) -> ScopedSelectionRefresh& = delete;
            ScopedSelectionRefresh(ScopedSelectionRefresh&&) = delete;
            auto operator=(ScopedSelectionRefresh&&) -> ScopedSelectionRefresh& = delete;

        private:
            bool& flag_;
        };
    } // namespace

    TreeView::TreeView(
        const widget::BuildContext& ui,
        const SelectionMode selection_mode,
        const float row_height
    ):
        ui_(ui),
        selection_(selection_mode),
        row_height_(row_height) {
        if (!std::isfinite(row_height_) || row_height_ <= 0.0F) {
            throw std::invalid_argument("TreeView row height must be finite and positive");
        }
        set_overflow(scene::ControlOverflow::clip);
        expander_color_ = theme::resolve_theme_color(
            ui_.theme(),
            theme::ThemeColor::token(theme::ColorToken::muted_foreground)
        );
        host_ = std::make_shared<ItemHost>(ui_);
        add_child(host_);
        selection_sub_ = selection_.changed().subscribe([this] {
            if (updating_selection_) {
                return;
            }
            refresh_item_states();
        });
        rebuild_all();
    }

    TreeView::~TreeView() = default;

    auto TreeView::create(
        const widget::BuildContext& ui,
        const SelectionMode selection_mode,
        const float row_height
    ) -> std::shared_ptr<TreeView> {
        return std::make_shared<TreeView>(ui, selection_mode, row_height);
    }

    // ─── 模型 / delegate ────────────────────────────────────────────────────

    void TreeView::set_model(std::shared_ptr<ITreeModel> model) {
        if (model.get() == model_.get()) {
            return;
        }
        if (delegate_ && model) {
            const void* accepted = delegate_->accepted_model_type_token();
            if (accepted != nullptr && accepted != model->model_type_token()) {
                throw std::invalid_argument("TreeView delegate model type mismatch");
            }
        }
        unsubscribe_model();
        pending_scheduled_ = false;
        if (host_) {
            host_->reset_items();
        }
        model_ = std::move(model);
        pending_reset_ = false;
        pending_ranges_.clear();
        subscribe_model();
        rebuild_all();
    }

    auto TreeView::model() const noexcept -> ITreeModel* {
        return model_.get();
    }

    void TreeView::set_delegate(std::shared_ptr<IItemDelegate> delegate) {
        if (delegate == delegate_) {
            return;
        }
        if (delegate && model_) {
            const void* accepted = delegate->accepted_model_type_token();
            if (accepted != nullptr && accepted != model_->model_type_token()) {
                throw std::invalid_argument("TreeView delegate model type mismatch");
            }
        }
        delegate_ = std::move(delegate);
        if (host_) {
            host_->reset_items();
        }
        if (pending_reset_) {
            // The old visible rows may index a model that has already shrunk. Let the queued
            // reset rebuild the ledger first; rebuild_all() materializes with the new delegate.
            pending_window_sync_ = true;
            schedule_pending();
            mark_layout_dirty();
            return;
        }
        refresh_window();
        mark_layout_dirty();
    }

    auto TreeView::delegate() const noexcept -> const std::shared_ptr<IItemDelegate>& {
        return delegate_;
    }

    // ─── 行高 / 缩进 ────────────────────────────────────────────────────────

    void TreeView::set_row_height(const float row_height) {
        if (!std::isfinite(row_height) || row_height <= 0.0F) {
            throw std::invalid_argument("TreeView row height must be finite and positive");
        }
        if (row_height_ == row_height) {
            return;
        }
        row_height_ = row_height;
        if (traversal_phase()) {
            scroll_offset_ = clamped_offset(scroll_offset_);
            window_position();
            schedule_window_sync();
        }
        else {
            refresh_window();
        }
        mark_layout_dirty();
    }

    auto TreeView::row_height() const noexcept -> float {
        return row_height_;
    }

    void TreeView::set_indent_width(const float width) {
        if (!std::isfinite(width) || width < 0.0F) {
            throw std::invalid_argument("TreeView indent width must be finite and non-negative");
        }
        indent_width_ = width;
        window_position();
        mark_layout_dirty();
    }

    auto TreeView::indent_width() const noexcept -> float {
        return indent_width_;
    }

    void TreeView::set_expander_gutter(const float width) {
        if (!std::isfinite(width) || width < 0.0F) {
            throw std::invalid_argument("TreeView expander gutter must be finite and non-negative");
        }
        expander_gutter_ = width;
        window_position();
        mark_layout_dirty();
    }

    auto TreeView::expander_gutter() const noexcept -> float {
        return expander_gutter_;
    }

    // ─── 行查询 ─────────────────────────────────────────────────────────────

    auto TreeView::row_count() const noexcept -> std::size_t {
        return visible_rows_.size();
    }

    auto TreeView::node_count() const noexcept -> std::size_t {
        return model_ != nullptr ? model_->row_count() : 0;
    }

    auto TreeView::item_count() const noexcept -> std::size_t {
        return host_ != nullptr ? host_->item_count() : 0;
    }

    auto TreeView::row_node(const RowId row) const -> scene::NanControl* {
        if (host_ == nullptr) {
            return nullptr;
        }
        return host_->node_for(CellId {.row = row, .column = ColumnId {}});
    }

    auto TreeView::display_row_for(const RowId row) const -> std::optional<std::size_t> {
        const auto found = visible_index_.find(row);
        if (found == visible_index_.end()) {
            return std::nullopt;
        }
        return found->second;
    }

    auto TreeView::row_id_for_display(const std::size_t display_row) const -> std::optional<RowId> {
        if (display_row >= visible_rows_.size()) {
            return std::nullopt;
        }
        return visible_rows_[display_row].row;
    }

    auto TreeView::depth_for_display(const std::size_t display_row) const -> std::size_t {
        if (display_row >= visible_rows_.size()) {
            return 0;
        }
        return visible_rows_[display_row].depth;
    }

    // ─── 展开（视图持有）────────────────────────────────────────────────────

    auto TreeView::is_expanded(const RowId row) const -> bool {
        return expanded_.contains(row);
    }

    auto TreeView::expand(const RowId row) -> bool {
        return set_expanded(row, true);
    }

    auto TreeView::collapse(const RowId row) -> bool {
        return set_expanded(row, false);
    }

    auto TreeView::toggle_expanded(const RowId row) -> bool {
        return set_expanded(row, !expanded_.contains(row));
    }

    void TreeView::expand_all() {
        bool changed = false;
        for (const auto& [row, model_row]: node_rows_) {
            (void)model_row;
            if (row_has_children(row) && expanded_.insert(row).second) {
                changed = true;
            }
        }
        if (!changed) {
            return;
        }
        if (traversal_phase()) {
            schedule_structure_sync();
        }
        else {
            rebuild_visible_rows();
            refresh_window();
        }
        expansion_changed_.emit();
    }

    void TreeView::collapse_all() {
        if (expanded_.empty()) {
            return;
        }
        expanded_.clear();
        if (traversal_phase()) {
            schedule_structure_sync();
        }
        else {
            rebuild_visible_rows();
            refresh_window();
        }
        expansion_changed_.emit();
    }

    auto TreeView::expansion_changed() const noexcept -> const reactive::Event<>& {
        return expansion_changed_;
    }

    void TreeView::set_on_expansion_changed(std::function<void()> callback) {
        user_expansion_sub_.disconnect();
        if (callback) {
            user_expansion_sub_ = expansion_changed_.subscribe(std::move(callback));
        }
    }

    // ─── 选择 ──────────────────────────────────────────────────────────────

    auto TreeView::selection() const noexcept -> const SelectionModel<RowId>& {
        return selection_;
    }

    auto TreeView::select(const RowId row) -> bool {
        if (!contains_node(row)) {
            return false;
        }
        current_row_ = row;
        bool expanded_ancestor = false;
        for (auto parent = model_->parent(row); parent.has_value();
             parent = model_->parent(*parent))
        {
            if (expanded_.insert(*parent).second) {
                expanded_ancestor = true;
            }
        }
        (void)selection_.select(row);
        if (expanded_ancestor) {
            if (traversal_phase()) {
                pending_select_row_ = row;
                schedule_structure_sync();
            }
            else {
                rebuild_visible_rows();
                refresh_window();
                if (const auto display = display_row_for(row); display.has_value()) {
                    scroll_to_row(*display);
                }
            }
            expansion_changed_.emit();
        }
        else if (const auto display = display_row_for(row); display.has_value()) {
            scroll_to_row(*display);
        }
        refresh_item_states();
        return true;
    }

    void TreeView::clear_selection() {
        (void)selection_.clear();
    }

    auto TreeView::selection_changed() const noexcept -> const reactive::Event<>& {
        return selection_.changed();
    }

    void TreeView::set_on_selection_changed(std::function<void()> callback) {
        user_selection_sub_.disconnect();
        if (callback) {
            user_selection_sub_ = selection_.changed().subscribe(std::move(callback));
        }
    }

    // ─── 纵向视口 ──────────────────────────────────────────────────────────

    auto TreeView::scroll_offset() const noexcept -> float {
        return scroll_offset_;
    }

    auto TreeView::maximum_scroll_offset() const noexcept -> float {
        if (height() <= 0.0F) {
            return 0.0F;
        }
        return window_math().maximum_scroll_offset();
    }

    void TreeView::set_scroll_offset(const float offset) {
        pending_scroll_row_.reset();
        const float clamped = clamped_offset(offset);
        if (clamped == scroll_offset_) {
            return;
        }
        scroll_offset_ = clamped;
        if (traversal_phase()) {
            window_position();
            schedule_window_sync();
            return;
        }
        refresh_window();
    }

    void TreeView::scroll_to_row(const std::size_t display_row) {
        if (display_row >= visible_rows_.size()) {
            pending_scroll_row_.reset();
            return;
        }
        if (height() <= 0.0F) {
            pending_scroll_row_ = display_row;
            return;
        }
        pending_scroll_row_.reset();
        if (const auto target = window_math().reveal_offset(display_row, scroll_offset_)) {
            set_scroll_offset(*target);
        }
    }

    void TreeView::set_wheel_step(const float step) {
        if (!std::isfinite(step) || step < 0.0F) {
            throw std::invalid_argument("TreeView wheel step must be finite and non-negative");
        }
        wheel_step_ = step;
    }

    // ─── 激活 ──────────────────────────────────────────────────────────────

    auto TreeView::activated() const noexcept -> const reactive::Event<RowId>& {
        return activated_;
    }

    void TreeView::set_on_activated(std::function<void(RowId)> callback) {
        activation_sub_.disconnect();
        if (callback) {
            activation_sub_ = activated_.subscribe(std::move(callback));
        }
    }

    auto TreeView::has_pending_notifications() const noexcept -> bool {
        return pending_reset_ || !pending_ranges_.empty() || pending_scheduled_
            || pending_window_sync_ || pending_structure_sync_;
    }

    // ─── 通知状态机 ────────────────────────────────────────────────────────

    auto TreeView::traversal_phase() const -> bool {
        const auto* tree = get_tree();
        return tree != nullptr && tree->phase() != scene::FramePhase::idle;
    }

    void TreeView::subscribe_model() {
        if (subscribed_ || !model_) {
            return;
        }
        reset_sub_ = model_->reset_event().subscribe([this] { on_model_reset(); });
        changed_sub_ = model_->data_changed_event().subscribe([this](const RowRange range) {
            on_model_data_changed(range);
        });
        subscribed_ = true;
    }

    void TreeView::unsubscribe_model() {
        reset_sub_.disconnect();
        changed_sub_.disconnect();
        subscribed_ = false;
    }

    void TreeView::on_model_reset() {
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

    void TreeView::on_model_data_changed(const RowRange range) {
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

    void TreeView::schedule_pending() {
        if (pending_scheduled_) {
            return;
        }
        auto* tree = get_tree();
        if (tree == nullptr) {
            apply_pending();
            return;
        }
        pending_scheduled_ = true;
        std::weak_ptr<TreeView> weak = std::static_pointer_cast<TreeView>(shared_from_this());
        tree->post_layout([weak, tree] {
            if (auto view = weak.lock(); view != nullptr && view->get_tree() == tree) {
                view->apply_pending();
            }
        });
    }

    void TreeView::apply_pending() {
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
        if (pending_structure_sync_) {
            apply_structure_sync();
        }
        if (pending_window_sync_) {
            pending_window_sync_ = false;
            refresh_window();
        }
    }

    void TreeView::rebuild_all() {
        pending_scheduled_ = false;
        pending_reset_ = false;
        pending_ranges_.clear();
        pending_window_sync_ = false;
        pending_structure_sync_ = false;
        pending_select_row_.reset();
        if (model_ == nullptr) {
            node_rows_.clear();
            expanded_.clear();
            visible_rows_.clear();
            visible_index_.clear();
            current_row_.reset();
            visible_window_ = {};
            scroll_offset_ = 0.0F;
            pending_scroll_row_.reset();
            if (host_) {
                host_->reset_items();
            }
            ScopedSelectionRefresh refresh {updating_selection_};
            (void)selection_.retain(std::vector<RowId> {});
            mark_layout_dirty();
            return;
        }
        rebuild_node_index();
        const bool expansion_pruned = retain_selection_and_expansion();
        rebuild_visible_rows();
        refresh_window();
        if (expansion_pruned) {
            expansion_changed_.emit();
        }
    }

    void TreeView::apply_data_changes(const std::vector<RowRange>& ranges) {
        if (model_ == nullptr || delegate_ == nullptr || ranges.empty()) {
            return;
        }
        std::vector<ItemHost::ItemRequest> requests;
        const std::size_t end = std::min(visible_window_.end, visible_rows_.size());
        if (end > visible_window_.first) {
            requests.reserve(end - visible_window_.first);
        }
        for (std::size_t display = visible_window_.first; display < end; ++display) {
            const auto& visible = visible_rows_[display];
            if (!row_in_ranges(visible.model_row, ranges)) {
                continue;
            }
            requests.push_back(make_request(
                *model_,
                delegate_,
                visible.model_row,
                visible.row,
                display,
                item_state_for(visible.row, visible.depth)
            ));
        }
        if (!requests.empty()) {
            host_->update_items(requests);
            mark_layout_dirty();
        }
    }

    void TreeView::rebuild_node_index() {
        node_rows_.clear();
        if (model_ == nullptr) {
            return;
        }
        const std::size_t count = model_->row_count();
        node_rows_.reserve(count);
        for (std::size_t row = 0; row < count; ++row) {
            node_rows_.emplace(model_->row_id(row), row);
        }
    }

    void TreeView::rebuild_visible_rows() {
        visible_rows_.clear();
        visible_index_.clear();
        if (model_ == nullptr) {
            return;
        }
        std::unordered_set<RowId> visited;
        visited.reserve(node_rows_.size());
        const auto append =
            [this, &visited](auto&& self, const RowId row, const std::size_t depth) -> void {
            // 模型契约要求无环；visited 防御错误模型导致的无限递归。
            if (!visited.insert(row).second) {
                return;
            }
            const auto found = node_rows_.find(row);
            if (found == node_rows_.end()) {
                return;
            }
            visible_index_.emplace(row, visible_rows_.size());
            visible_rows_.push_back(
                VisibleRow {.row = row, .model_row = found->second, .depth = depth}
            );
            if (!expanded_.contains(row)) {
                return;
            }
            const std::size_t count = model_->child_count(row);
            for (std::size_t index = 0; index < count; ++index) {
                self(self, model_->child(row, index), depth + 1);
            }
        };
        for (std::size_t row = 0; row < model_->row_count(); ++row) {
            const RowId id = model_->row_id(row);
            if (!model_->parent(id).has_value()) {
                append(append, id, 0);
            }
        }
    }

    auto TreeView::retain_selection_and_expansion() -> bool {
        std::vector<RowId> universe;
        universe.reserve(node_rows_.size());
        for (const auto& [row, model_row]: node_rows_) {
            (void)model_row;
            universe.push_back(row);
        }
        const std::size_t expanded_before = expanded_.size();
        std::erase_if(expanded_, [this](const RowId row) {
            // 已消失的身份与 reset 后变成叶子的节点都不应再处于展开态。
            return !node_rows_.contains(row) || !row_has_children(row);
        });
        const bool expansion_pruned = expanded_.size() != expanded_before;
        ScopedSelectionRefresh refresh {updating_selection_};
        (void)selection_.retain(universe);
        if (current_row_.has_value() && !node_rows_.contains(*current_row_)) {
            current_row_.reset();
        }
        return expansion_pruned;
    }

    auto TreeView::contains_node(const RowId row) const -> bool {
        return node_rows_.contains(row);
    }

    auto TreeView::row_has_children(const RowId row) const -> bool {
        return model_ != nullptr && model_->has_children(row);
    }

    auto TreeView::set_expanded(const RowId row, const bool expanded) -> bool {
        if (!contains_node(row) || (expanded && !row_has_children(row))) {
            return false;
        }
        if (expanded_.contains(row) == expanded) {
            return false;
        }
        if (expanded) {
            expanded_.insert(row);
        }
        else {
            expanded_.erase(row);
        }
        if (traversal_phase()) {
            schedule_structure_sync();
        }
        else {
            rebuild_visible_rows();
            refresh_window();
        }
        expansion_changed_.emit();
        return true;
    }

    void TreeView::schedule_structure_sync() {
        pending_structure_sync_ = true;
        schedule_pending();
    }

    void TreeView::apply_structure_sync() {
        if (!pending_structure_sync_) {
            return;
        }
        pending_structure_sync_ = false;
        rebuild_visible_rows();
        refresh_window();
        if (pending_select_row_.has_value()) {
            const auto row = *pending_select_row_;
            pending_select_row_.reset();
            if (const auto display = display_row_for(row); display.has_value()) {
                scroll_to_row(*display);
            }
        }
    }

    auto TreeView::item_state_for(const RowId row, const std::size_t depth) const -> ItemState {
        return ItemState {
            .selected = selection_.contains(row),
            .current = current_row_.has_value() && *current_row_ == row,
            .hovered = false,
            .enabled = true,
            .theme_revision = ui_.theme_manager().revision(),
            .depth = depth,
            .has_children = model_ != nullptr && model_->has_children(row),
            .expanded = expanded_.contains(row),
        };
    }

    void TreeView::refresh_item_states() {
        if (model_ == nullptr || delegate_ == nullptr || pending_reset_ || visible_window_.empty())
        {
            return;
        }
        std::vector<ItemHost::ItemRequest> requests;
        const std::size_t end = std::min(visible_window_.end, visible_rows_.size());
        if (end > visible_window_.first) {
            requests.reserve(end - visible_window_.first);
        }
        for (std::size_t display = visible_window_.first; display < end; ++display) {
            const auto& visible = visible_rows_[display];
            requests.push_back(make_request(
                *model_,
                delegate_,
                visible.model_row,
                visible.row,
                display,
                item_state_for(visible.row, visible.depth)
            ));
        }
        if (!requests.empty()) {
            host_->update_items(requests);
            mark_layout_dirty();
        }
    }

    auto TreeView::row_at(const foundation::NanPoint local) const -> std::optional<std::size_t> {
        if (model_ == nullptr || pending_reset_) {
            return std::nullopt;
        }
        const auto row = window_math().hit_row(local.get_y(), scroll_offset_, visible_window_);
        if (!row.has_value() || *row >= visible_rows_.size()) {
            return std::nullopt;
        }
        return row;
    }

    auto
    TreeView::expander_at(const foundation::NanPoint local, const std::size_t display_row) const
        -> bool {
        if (display_row >= visible_rows_.size()) {
            return false;
        }
        const auto& visible = visible_rows_[display_row];
        if (!row_has_children(visible.row)) {
            return false;
        }
        const float left = static_cast<float>(visible.depth) * indent_width_;
        const float right = left + expander_gutter_;
        const float x = local.get_x();
        return x >= left && x < right;
    }

    void TreeView::move_current_to(const std::size_t display_row, const bool select_follows) {
        if (display_row >= visible_rows_.size()) {
            return;
        }
        const RowId row = visible_rows_[display_row].row;
        current_row_ = row;
        if (select_follows) {
            (void)selection_.select(row);
        }
        scroll_to_row(display_row);
        refresh_item_states();
    }

    auto TreeView::visible_order() const -> std::vector<RowId> {
        std::vector<RowId> rows;
        rows.reserve(visible_rows_.size());
        for (const auto& visible: visible_rows_) {
            rows.push_back(visible.row);
        }
        return rows;
    }

    auto TreeView::handle_key(scene::KeyEvent& key_event) -> bool {
        if (!key_event.is_press_or_repeat() || pending_reset_) {
            return false;
        }
        const auto logical = key_event.logical_key();
        if (key_event.is_pressed()) {
            if (logical == foundation::LogicalKey::enter) {
                if (!current_row_.has_value()) {
                    return false;
                }
                if (!display_row_for(*current_row_).has_value()) {
                    return false;
                }
                activated_.emit(*current_row_);
                return true;
            }
            if (logical == foundation::LogicalKey::space) {
                if (!current_row_.has_value()) {
                    return false;
                }
                if (!display_row_for(*current_row_).has_value()) {
                    return false;
                }
                (void)selection_.toggle(*current_row_);
                return true;
            }
        }
        if (model_ == nullptr || visible_rows_.empty()) {
            return false;
        }
        std::optional<std::size_t> current;
        if (current_row_.has_value()) {
            current = display_row_for(*current_row_);
        }
        const bool single = selection_.mode() == SelectionMode::single;
        if (logical == foundation::LogicalKey::left) {
            if (!current.has_value()) {
                return false;
            }
            const auto& visible = visible_rows_[*current];
            if (expanded_.contains(visible.row)) {
                (void)collapse(visible.row);
                return true;
            }
            if (const auto parent = model_->parent(visible.row); parent.has_value()) {
                if (const auto display = display_row_for(*parent); display.has_value()) {
                    move_current_to(*display, single);
                }
            }
            return true;
        }
        if (logical == foundation::LogicalKey::right) {
            if (!current.has_value()) {
                return false;
            }
            const auto& visible = visible_rows_[*current];
            if (row_has_children(visible.row)) {
                if (!expanded_.contains(visible.row)) {
                    (void)expand(visible.row);
                    return true;
                }
                const RowId first = model_->child(visible.row, 0);
                if (const auto display = display_row_for(first); display.has_value()) {
                    move_current_to(*display, single);
                }
            }
            return true;
        }
        const bool navigation_key = logical == foundation::LogicalKey::up
            || logical == foundation::LogicalKey::down || logical == foundation::LogicalKey::home
            || logical == foundation::LogicalKey::end || logical == foundation::LogicalKey::page_up
            || logical == foundation::LogicalKey::page_down;
        if (!navigation_key) {
            return false;
        }
        const std::size_t count = visible_rows_.size();
        std::size_t next = 0;
        if (current.has_value()) {
            next = *current;
            const std::size_t page = std::max<std::size_t>(1, visible_window_.count());
            switch (logical) {
                case foundation::LogicalKey::up:
                    next = *current > 0 ? *current - 1 : 0;
                    break;
                case foundation::LogicalKey::down:
                    next = std::min(*current + 1, count - 1);
                    break;
                case foundation::LogicalKey::home:
                    next = 0;
                    break;
                case foundation::LogicalKey::end:
                    next = count - 1;
                    break;
                case foundation::LogicalKey::page_up:
                    next = *current > page ? *current - page : 0;
                    break;
                case foundation::LogicalKey::page_down:
                    next = std::min(*current + page, count - 1);
                    break;
                default:
                    return false;
            }
        }
        else {
            if (logical == foundation::LogicalKey::end) {
                next = count - 1;
            }
            else if (
                logical == foundation::LogicalKey::up || logical == foundation::LogicalKey::page_up
            )
            {
                next = visible_window_.empty() ? 0 : visible_window_.end - 1;
                next = std::min(next, count - 1);
            }
            else if (!visible_window_.empty()) {
                next = std::min(visible_window_.first, count - 1);
            }
        }
        move_current_to(next, single);
        return true;
    }

    // ─── 窗口 / 视口辅助 ───────────────────────────────────────────────────

    auto TreeView::window_math() const noexcept -> internal::UniformRowWindow {
        return internal::UniformRowWindow {row_height_, std::max(0.0F, height()), row_count()};
    }

    auto TreeView::clamped_offset(const float offset) const noexcept -> float {
        if (height() <= 0.0F) {
            return 0.0F;
        }
        return window_math().clamp_offset(offset);
    }

    void TreeView::refresh_window() {
        const auto math = window_math();
        scroll_offset_ = clamped_offset(scroll_offset_);
        visible_window_ = math.visible_window(scroll_offset_);
        materialize_rows();
        window_position();
        mark_layout_dirty();
    }

    void TreeView::schedule_window_sync() {
        if (pending_window_sync_) {
            return;
        }
        pending_window_sync_ = true;
        auto* tree = get_tree();
        if (tree == nullptr) {
            apply_window_sync();
            return;
        }
        std::weak_ptr<TreeView> weak = std::static_pointer_cast<TreeView>(shared_from_this());
        tree->post_layout([weak, tree] {
            if (auto view = weak.lock(); view != nullptr && view->get_tree() == tree) {
                view->apply_window_sync();
            }
        });
    }

    void TreeView::apply_window_sync() {
        if (!pending_window_sync_ || pending_reset_ || !pending_ranges_.empty()
            || pending_scheduled_ || pending_structure_sync_)
        {
            return;
        }
        pending_window_sync_ = false;
        refresh_window();
    }

    void TreeView::materialize_rows() {
        if (model_ == nullptr || delegate_ == nullptr) {
            if (host_) {
                host_->reset_items();
            }
            return;
        }
        std::vector<ItemHost::ItemRequest> requests;
        const std::size_t end = std::min(visible_window_.end, visible_rows_.size());
        if (end > visible_window_.first) {
            requests.reserve(end - visible_window_.first);
        }
        for (std::size_t display = visible_window_.first; display < end; ++display) {
            const auto& visible = visible_rows_[display];
            requests.push_back(make_request(
                *model_,
                delegate_,
                visible.model_row,
                visible.row,
                display,
                item_state_for(visible.row, visible.depth)
            ));
        }
        host_->set_items(requests);
    }

    auto TreeView::content_x(const std::size_t depth) const noexcept -> float {
        return static_cast<float>(depth) * indent_width_ + expander_gutter_;
    }

    auto TreeView::content_width(const std::size_t depth) const noexcept -> float {
        return std::max(0.0F, std::max(0.0F, width()) - content_x(depth));
    }

    void TreeView::window_position() {
        if (host_ == nullptr) {
            return;
        }
        const float viewport_width = std::max(0.0F, width());
        const float viewport_height = std::max(0.0F, height());
        host_->layout_to(
            foundation::NanRect::from_xywh(0.0F, 0.0F, viewport_width, viewport_height)
        );
        if (model_ == nullptr || delegate_ == nullptr || pending_reset_) {
            return;
        }
        const std::size_t end = std::min(visible_window_.end, visible_rows_.size());
        for (std::size_t display = visible_window_.first; display < end; ++display) {
            const auto& visible = visible_rows_[display];
            if (auto* node = host_->node_for(CellId {.row = visible.row, .column = ColumnId {}});
                node != nullptr)
            {
                const auto rect = foundation::NanRect::from_xywh(
                    content_x(visible.depth),
                    static_cast<float>(display) * row_height_ - scroll_offset_,
                    content_width(visible.depth),
                    row_height_
                );
                (void)node->measure_layout(
                    foundation::NanLayoutConstraints::tight(rect.get_size())
                );
                node->layout_to(rect);
            }
        }
    }

    // ─── 布局 / 输入 / 绘制 / 生命周期 ─────────────────────────────────────

    auto TreeView::is_focusable() const -> bool {
        return true;
    }

    auto TreeView::semantics_properties() const -> semantics::Properties {
        const bool focused_now = get_tree() != nullptr && get_tree()->focused_node() == this;
        return {
            .role = semantics::Role::list,
            .state = {.focusable = true, .focused = focused_now},
        };
    }

    auto TreeView::on_measure(const foundation::NanLayoutConstraints constraints)
        -> foundation::NanSize {
        const float cross_width = width() > 0.0F ? width() : constraints.max_width;
        const float measured_width =
            std::isfinite(cross_width) ? cross_width : constraints.min_width;
        const double content_height =
            static_cast<double>(visible_rows_.size()) * static_cast<double>(row_height_);
        if (!std::isfinite(measured_width) || measured_width < 0.0F
            || !std::isfinite(content_height) || content_height < 0.0
            || content_height > static_cast<double>(std::numeric_limits<float>::max()))
        {
            throw std::invalid_argument(
                "TreeView content extent must fit finite layout coordinates"
            );
        }
        return constraints.constrain(
            foundation::NanSize(measured_width, static_cast<float>(content_height))
        );
    }

    auto TreeView::on_layout() -> void {
        const auto math = window_math();
        if (pending_scroll_row_.has_value()) {
            if (const auto target = math.reveal_offset(*pending_scroll_row_, scroll_offset_)) {
                scroll_offset_ = *target;
            }
            pending_scroll_row_.reset();
        }
        scroll_offset_ = clamped_offset(scroll_offset_);
        const auto window = math.visible_window(scroll_offset_);
        const bool window_changed =
            window.first != visible_window_.first || window.end != visible_window_.end;
        window_position();
        if (window_changed) {
            schedule_window_sync();
        }
    }

    auto TreeView::on_input(scene::InputEvent& event) -> bool {
        if (event.type() == scene::EventType::mouse_button) {
            auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
            if (mouse.button() != scene::MouseButtonEvent::Button::left || !mouse.is_pressed()) {
                return false;
            }
            const auto local = to_local(mouse.screen_pos());
            const auto display = row_at(local);
            if (!display.has_value()) {
                return false;
            }
            if (auto* tree = get_tree()) {
                tree->set_focus(this);
            }
            const auto& visible = visible_rows_[*display];
            current_row_ = visible.row;
            if (expander_at(local, *display)) {
                (void)toggle_expanded(visible.row);
                event.accept();
                return true;
            }
            const auto mods = mouse.modifiers();
            const auto order = visible_order();
            if (mods.shift) {
                const auto anchor = selection_.anchor().value_or(visible.row);
                // 折叠后锚点可能落在不可见子树中：回退到本次点击行，避免 shift 操作
                // 因 select_range 找不到键而变成 no-op。
                const bool anchor_visible = std::ranges::find(order, anchor) != order.end();
                (void)selection_
                    .select_range(order, anchor_visible ? anchor : visible.row, visible.row);
            }
            else if (mods.ctrl && selection_.mode() != SelectionMode::single) {
                (void)selection_.toggle(visible.row);
            }
            else {
                (void)selection_.select_range(order, visible.row, visible.row);
            }
            scroll_to_row(*display);
            refresh_item_states();
            event.accept();
            return true;
        }
        if (event.type() == scene::EventType::mouse_wheel) {
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
        if (event.type() == scene::EventType::key) {
            auto& key = static_cast<scene::KeyEvent&>(event);
            if (!handle_key(key)) {
                return false;
            }
            event.accept();
            return true;
        }
        return false;
    }

    auto TreeView::on_draw(render::DrawContext& ctx) -> void {
        if (model_ == nullptr || visible_rows_.empty()) {
            return;
        }
        const auto world = ctx.world_transform();
        const float thickness = ctx.logical_to_screen(1.2F);
        const auto color = expander_color_.with_alpha(expander_color_.alpha() * ctx.opacity());
        const std::size_t end = std::min(visible_window_.end, visible_rows_.size());
        for (std::size_t display = visible_window_.first; display < end; ++display) {
            const auto& visible = visible_rows_[display];
            if (!row_has_children(visible.row)) {
                continue;
            }
            const float center_x =
                static_cast<float>(visible.depth) * indent_width_ + expander_gutter_ * 0.5F;
            const float center_y =
                static_cast<float>(display) * row_height_ - scroll_offset_ + row_height_ * 0.5F;
            const float half = 3.5F;
            foundation::NanPoint a;
            foundation::NanPoint b;
            foundation::NanPoint c;
            if (expanded_.contains(visible.row)) {
                a = foundation::NanPoint(center_x - half, center_y - half * 0.5F);
                b = foundation::NanPoint(center_x + half, center_y - half * 0.5F);
                c = foundation::NanPoint(center_x, center_y + half * 0.5F);
            }
            else {
                a = foundation::NanPoint(center_x - half * 0.5F, center_y - half);
                b = foundation::NanPoint(center_x + half * 0.5F, center_y);
                c = foundation::NanPoint(center_x - half * 0.5F, center_y + half);
            }
            const auto pa = world.transform_point(a);
            const auto pb = world.transform_point(b);
            const auto pc = world.transform_point(c);
            ctx.device().draw_line(pa, pb, thickness, color);
            ctx.device().draw_line(pb, pc, thickness, color);
            ctx.device().draw_line(pc, pa, thickness, color);
        }
    }

    void TreeView::on_enter_tree() {
        scene::NanControl::on_enter_tree();
        subscribe_model();
    }

    void TreeView::on_ready() {
        scene::NanControl::on_ready();
        rebuild_all();
    }

    void TreeView::on_exit_tree() {
        unsubscribe_model();
        pending_scheduled_ = false;
        pending_reset_ = false;
        pending_ranges_.clear();
        pending_window_sync_ = false;
        pending_structure_sync_ = false;
        if (host_) {
            host_->release_scopes();
        }
        scene::NanControl::on_exit_tree();
    }

    void TreeView::on_theme_changed(const theme::ThemeManager&) {
        expander_color_ = theme::resolve_theme_color(
            ui_.theme(),
            theme::ThemeColor::token(theme::ColorToken::muted_foreground)
        );
        refresh_item_states();
        mark_dirty(scene::DirtyFlags::paint);
        mark_layout_dirty();
    }
} // namespace nandina::widget
