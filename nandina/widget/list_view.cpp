#include "list_view.hpp"

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
            const std::size_t row,
            const RowId id,
            const ItemState state
        ) -> ItemHost::ItemRequest {
            return ItemHost::ItemRequest {
                // 1D 列表没有列：空 ColumnId 只作为 ItemHost 的键，ItemBinding 中 column 为 nullptr。
                .id = CellId {.row = id, .column = ColumnId {}},
                .model = &model,
                .model_row = row,
                .display_index = ModelIndex {.row = row, .column = 0},
                .row = id,
                .column = nullptr,
                .state = state,
                .delegate = delegate,
            };
        }
    } // namespace

    ListView::ListView(
        const widget::BuildContext& ui,
        const SelectionMode selection_mode,
        const float row_height
    ):
        ui_(ui),
        selection_(selection_mode),
        row_height_(row_height) {
        if (!std::isfinite(row_height_) || row_height_ <= 0.0F) {
            throw std::invalid_argument("ListView row height must be finite and positive");
        }
        set_overflow(scene::ControlOverflow::clip);
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

    ListView::~ListView() = default;

    auto ListView::create(
        const widget::BuildContext& ui,
        const SelectionMode selection_mode,
        const float row_height
    ) -> std::shared_ptr<ListView> {
        return std::make_shared<ListView>(ui, selection_mode, row_height);
    }

    // ─── 模型 / delegate ────────────────────────────────────────────────────

    void ListView::set_model(std::shared_ptr<IRowModel> model) {
        if (model.get() == model_.get()) {
            return;
        }
        if (delegate_ && model) {
            const void* accepted = delegate_->accepted_model_type_token();
            if (accepted != nullptr && accepted != model->model_type_token()) {
                throw std::invalid_argument("ListView delegate model type mismatch");
            }
        }
        unsubscribe_model();
        pending_scheduled_ = false;
        if (host_) {
            // 不同模型可能复用 RowId，但 create 期的 delegate 状态与旧行无关。
            host_->reset_items();
        }
        model_ = std::move(model);
        pending_reset_ = false;
        pending_ranges_.clear();
        subscribe_model();
        rebuild_all();
    }

    auto ListView::model() const noexcept -> IRowModel* {
        return model_.get();
    }

    void ListView::set_delegate(std::shared_ptr<IItemDelegate> delegate) {
        if (delegate == delegate_) {
            return;
        }
        if (delegate && model_) {
            const void* accepted = delegate->accepted_model_type_token();
            if (accepted != nullptr && accepted != model_->model_type_token()) {
                throw std::invalid_argument("ListView delegate model type mismatch");
            }
        }
        delegate_ = std::move(delegate);
        // 旧 delegate 创建的控件不得由新 delegate 更新：整窗重建（含 scope）。
        if (host_) {
            host_->reset_items();
        }
        refresh_window();
        mark_layout_dirty();
    }

    auto ListView::delegate() const noexcept -> const std::shared_ptr<IItemDelegate>& {
        return delegate_;
    }

    // ─── 行高 / 查询 ────────────────────────────────────────────────────────

    void ListView::set_row_height(const float row_height) {
        if (!std::isfinite(row_height) || row_height <= 0.0F) {
            throw std::invalid_argument("ListView row height must be finite and positive");
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

    auto ListView::row_height() const noexcept -> float {
        return row_height_;
    }

    auto ListView::row_count() const noexcept -> std::size_t {
        return model_ != nullptr ? model_->row_count() : 0;
    }

    auto ListView::item_count() const noexcept -> std::size_t {
        return host_ != nullptr ? host_->item_count() : 0;
    }

    auto ListView::row_node(const RowId row) const -> scene::NanControl* {
        if (host_ == nullptr) {
            return nullptr;
        }
        return host_->node_for(CellId {.row = row, .column = ColumnId {}});
    }

    auto ListView::display_row_for(const RowId row) const -> std::optional<std::size_t> {
        const auto found = row_index_.find(row);
        if (found == row_index_.end()) {
            return std::nullopt;
        }
        return found->second;
    }

    auto ListView::row_id_for_display(const std::size_t display_row) const
        -> std::optional<RowId> {
        if (model_ == nullptr || display_row >= model_->row_count()) {
            return std::nullopt;
        }
        return model_->row_id(display_row);
    }

    // ─── 选择 ──────────────────────────────────────────────────────────────

    auto ListView::selection() const noexcept -> const SelectionModel<RowId>& {
        return selection_;
    }

    auto ListView::select(const RowId row) -> bool {
        const auto found = row_index_.find(row);
        if (found == row_index_.end()) {
            return false;
        }
        current_row_ = row;
        // 返回“身份有效”，与 DataTable 程序化入口一致；合法但已选中仍是 true。
        (void)selection_.select(row);
        scroll_to_row(found->second);
        refresh_item_states();
        return true;
    }

    void ListView::clear_selection() {
        (void)selection_.clear();
    }

    auto ListView::selection_changed() const noexcept -> const reactive::Event<>& {
        return selection_.changed();
    }

    void ListView::set_on_selection_changed(std::function<void()> callback) {
        user_selection_sub_.disconnect();
        if (callback) {
            user_selection_sub_ = selection_.changed().subscribe(std::move(callback));
        }
    }

    // ─── 纵向视口 ──────────────────────────────────────────────────────────

    auto ListView::scroll_offset() const noexcept -> float {
        return scroll_offset_;
    }

    auto ListView::maximum_scroll_offset() const noexcept -> float {
        if (height() <= 0.0F) {
            return 0.0F;
        }
        return window_math().maximum_scroll_offset();
    }

    void ListView::set_scroll_offset(const float offset) {
        // 显式 offset / 滚轮操作取消尚未落位的 scroll_to_row 目标。
        pending_scroll_row_.reset();
        const float clamped = clamped_offset(offset);
        if (clamped == scroll_offset_) {
            return;
        }
        scroll_offset_ = clamped;
        if (traversal_phase()) {
            // 当前帧先把已物化行平移到位；窗口的增删物化延迟到 post_layout。
            window_position();
            schedule_window_sync();
            return;
        }
        refresh_window();
    }

    void ListView::scroll_to_row(const std::size_t display_row) {
        if (model_ == nullptr || display_row >= model_->row_count()) {
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

    void ListView::set_wheel_step(const float step) {
        if (!std::isfinite(step) || step < 0.0F) {
            throw std::invalid_argument("ListView wheel step must be finite and non-negative");
        }
        wheel_step_ = step;
    }

    // ─── 激活 ──────────────────────────────────────────────────────────────

    auto ListView::activated() const noexcept -> const reactive::Event<RowId>& {
        return activated_;
    }

    void ListView::set_on_activated(std::function<void(RowId)> callback) {
        activation_sub_.disconnect();
        if (callback) {
            activation_sub_ = activated_.subscribe(std::move(callback));
        }
    }

    auto ListView::has_pending_notifications() const noexcept -> bool {
        return pending_reset_ || !pending_ranges_.empty() || pending_scheduled_
            || pending_window_sync_;
    }

    // ─── 通知状态机 ────────────────────────────────────────────────────────

    auto ListView::traversal_phase() const -> bool {
        const auto* tree = get_tree();
        return tree != nullptr && tree->phase() != scene::FramePhase::idle;
    }

    void ListView::subscribe_model() {
        if (subscribed_ || !model_) {
            return;
        }
        reset_sub_ = model_->reset_event().subscribe([this] { on_model_reset(); });
        changed_sub_ = model_->data_changed_event().subscribe([this](const RowRange range) {
            on_model_data_changed(range);
        });
        subscribed_ = true;
    }

    void ListView::unsubscribe_model() {
        reset_sub_.disconnect();
        changed_sub_.disconnect();
        subscribed_ = false;
    }

    void ListView::on_model_reset() {
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

    void ListView::on_model_data_changed(const RowRange range) {
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

    void ListView::schedule_pending() {
        if (pending_scheduled_) {
            return;
        }
        auto* tree = get_tree();
        if (tree == nullptr) {
            apply_pending();
            return;
        }
        pending_scheduled_ = true;
        std::weak_ptr<ListView> weak = std::static_pointer_cast<ListView>(shared_from_this());
        tree->post_layout([weak, tree] {
            if (auto list = weak.lock(); list != nullptr && list->get_tree() == tree) {
                list->apply_pending();
            }
        });
    }

    void ListView::apply_pending() {
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
        // ledger was rebuilt; apply it now against the new row set.
        if (pending_window_sync_) {
            pending_window_sync_ = false;
            refresh_window();
        }
    }

    void ListView::rebuild_all() {
        // 结构性重建读取模型当前快照；旧范围的排队动作属于上一代数据，直接丢弃。
        pending_scheduled_ = false;
        pending_reset_ = false;
        pending_ranges_.clear();
        pending_window_sync_ = false;
        ordered_rows_.clear();
        row_index_.clear();
        if (model_ == nullptr) {
            visible_window_ = {};
            scroll_offset_ = 0.0F;
            pending_scroll_row_.reset();
            current_row_.reset();
            if (host_) {
                host_->reset_items();
            }
            updating_selection_ = true;
            (void)selection_.retain(std::vector<RowId> {});
            updating_selection_ = false;
            mark_layout_dirty();
            return;
        }
        const std::size_t count = model_->row_count();
        ordered_rows_.reserve(count);
        row_index_.reserve(count);
        for (std::size_t row = 0; row < count; ++row) {
            const RowId id = model_->row_id(row);
            ordered_rows_.push_back(id);
            row_index_.emplace(id, row);
        }
        retain_selection();
        refresh_window();
    }

    void ListView::apply_data_changes(const std::vector<RowRange>& ranges) {
        if (model_ == nullptr || delegate_ == nullptr || ranges.empty()) {
            return;
        }
        std::vector<ItemHost::ItemRequest> requests;
        const std::size_t end = std::min(visible_window_.end, model_->row_count());
        if (end > visible_window_.first) {
            requests.reserve(end - visible_window_.first);
        }
        for (std::size_t row = visible_window_.first; row < end; ++row) {
            if (!row_in_ranges(row, ranges)) {
                continue;
            }
            const RowId id = model_->row_id(row);
            requests.push_back(make_request(*model_, delegate_, row, id, item_state_for(id)));
        }
        if (!requests.empty()) {
            host_->update_items(requests);
            mark_layout_dirty();
        }
    }

    void ListView::retain_selection() {
        updating_selection_ = true;
        (void)selection_.retain(ordered_rows_);
        updating_selection_ = false;
        if (current_row_.has_value() && !row_index_.contains(*current_row_)) {
            current_row_.reset();
        }
    }

    auto ListView::item_state_for(const RowId row) const -> ItemState {
        return ItemState {
            .selected = selection_.contains(row),
            .current = current_row_.has_value() && *current_row_ == row,
            .hovered = false,
            .enabled = true,
            .theme_revision = ui_.theme_manager().revision(),
        };
    }

    void ListView::refresh_item_states() {
        if (model_ == nullptr || delegate_ == nullptr || visible_window_.empty()) {
            return;
        }
        std::vector<ItemHost::ItemRequest> requests;
        const std::size_t end = std::min(visible_window_.end, model_->row_count());
        if (end > visible_window_.first) {
            requests.reserve(end - visible_window_.first);
        }
        for (std::size_t row = visible_window_.first; row < end; ++row) {
            const RowId id = model_->row_id(row);
            requests.push_back(make_request(*model_, delegate_, row, id, item_state_for(id)));
        }
        if (!requests.empty()) {
            host_->update_items(requests);
            mark_layout_dirty();
        }
    }

    auto ListView::row_at(const foundation::NanPoint local) const -> std::optional<RowId> {
        if (model_ == nullptr) {
            return std::nullopt;
        }
        const auto row = window_math().hit_row(local.get_y(), scroll_offset_, visible_window_);
        if (!row.has_value() || *row >= model_->row_count()) {
            return std::nullopt;
        }
        return model_->row_id(*row);
    }

    auto ListView::handle_key(scene::KeyEvent& key_event) -> bool {
        if (!key_event.is_press_or_repeat()) {
            return false;
        }
        const auto logical = key_event.logical_key();
        if (key_event.is_pressed()) {
            if (logical == foundation::LogicalKey::enter) {
                if (!current_row_.has_value()) {
                    return false;
                }
                activated_.emit(*current_row_);
                return true;
            }
            if (logical == foundation::LogicalKey::space) {
                if (!current_row_.has_value()) {
                    return false;
                }
                (void)selection_.toggle(*current_row_);
                return true;
            }
        }
        if (model_ == nullptr || row_count() == 0) {
            return false;
        }
        const bool directional = logical == foundation::LogicalKey::up
            || logical == foundation::LogicalKey::down;
        const bool navigation_key = directional || logical == foundation::LogicalKey::home
            || logical == foundation::LogicalKey::end || logical == foundation::LogicalKey::page_up
            || logical == foundation::LogicalKey::page_down;
        if (!navigation_key) {
            return false;
        }
        const std::size_t count = row_count();
        std::optional<std::size_t> current;
        if (current_row_.has_value()) {
            if (const auto found = row_index_.find(*current_row_); found != row_index_.end()) {
                current = found->second;
            }
        }
        if (!current.has_value()) {
            // 播种键盘游标：方向键从可见窗口边缘开始；multiple 模式只移动、不选中。
            std::size_t seed = 0;
            if (logical == foundation::LogicalKey::end) {
                seed = count - 1;
            }
            else if (logical == foundation::LogicalKey::up
                     || logical == foundation::LogicalKey::page_up)
            {
                seed = visible_window_.empty() ? 0 : visible_window_.end - 1;
                seed = std::min(seed, count - 1);
            }
            else if (!visible_window_.empty()) {
                seed = std::min(visible_window_.first, count - 1);
            }
            const RowId id = model_->row_id(seed);
            current_row_ = id;
            if (selection_.mode() == SelectionMode::single) {
                (void)selection_.select(id);
            }
            scroll_to_row(seed);
            refresh_item_states();
            return true;
        }
        const std::size_t page = std::max<std::size_t>(1, visible_window_.count());
        std::size_t next = *current;
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
        const RowId id = model_->row_id(next);
        current_row_ = id;
        if (selection_.mode() == SelectionMode::single) {
            (void)selection_.select(id);
        }
        scroll_to_row(next);
        refresh_item_states();
        return true;
    }

    // ─── 窗口 / 视口辅助 ───────────────────────────────────────────────────

    auto ListView::window_math() const noexcept -> internal::UniformRowWindow {
        return internal::UniformRowWindow {row_height_, std::max(0.0F, height()), row_count()};
    }

    auto ListView::clamped_offset(const float offset) const noexcept -> float {
        if (height() <= 0.0F) {
            return 0.0F;
        }
        return window_math().clamp_offset(offset);
    }

    void ListView::refresh_window() {
        const auto math = window_math();
        scroll_offset_ = clamped_offset(scroll_offset_);
        visible_window_ = math.visible_window(scroll_offset_);
        materialize_rows();
        window_position();
        mark_layout_dirty();
    }

    void ListView::schedule_window_sync() {
        if (pending_window_sync_) {
            return;
        }
        auto* tree = get_tree();
        if (tree == nullptr) {
            apply_window_sync();
            return;
        }
        pending_window_sync_ = true;
        std::weak_ptr<ListView> weak = std::static_pointer_cast<ListView>(shared_from_this());
        tree->post_layout([weak, tree] {
            if (auto list = weak.lock(); list != nullptr && list->get_tree() == tree) {
                list->apply_window_sync();
            }
        });
    }

    void ListView::apply_window_sync() {
        // 同一帧内模型回调可能还排在窗口动作之后：先重建账本，避免数据收缩时读旧索引。
        if (pending_reset_ || !pending_ranges_.empty() || pending_scheduled_) {
            return;
        }
        pending_window_sync_ = false;
        refresh_window();
    }

    void ListView::materialize_rows() {
        if (model_ == nullptr || delegate_ == nullptr) {
            if (host_) {
                host_->reset_items();
            }
            return;
        }
        std::vector<ItemHost::ItemRequest> requests;
        const std::size_t end = std::min(visible_window_.end, model_->row_count());
        if (end > visible_window_.first) {
            requests.reserve(end - visible_window_.first);
        }
        for (std::size_t row = visible_window_.first; row < end; ++row) {
            const RowId id = model_->row_id(row);
            requests.push_back(make_request(*model_, delegate_, row, id, item_state_for(id)));
        }
        host_->set_items(requests);
    }

    void ListView::window_position() {
        if (host_ == nullptr) {
            return;
        }
        const float viewport_width = std::max(0.0F, width());
        const float viewport_height = std::max(0.0F, height());
        host_->layout_to(
            foundation::NanRect::from_xywh(0.0F, 0.0F, viewport_width, viewport_height)
        );
        if (model_ == nullptr || delegate_ == nullptr) {
            return;
        }
        const std::size_t end = std::min(visible_window_.end, model_->row_count());
        for (std::size_t row = visible_window_.first; row < end; ++row) {
            const RowId id = model_->row_id(row);
            if (auto* node = host_->node_for(CellId {.row = id, .column = ColumnId {}});
                node != nullptr)
            {
                const auto rect = foundation::NanRect::from_xywh(
                    0.0F,
                    static_cast<float>(row) * row_height_ - scroll_offset_,
                    viewport_width,
                    row_height_
                );
                (void)node->measure_layout(
                    foundation::NanLayoutConstraints::tight(rect.get_size())
                );
                node->layout_to(rect);
            }
        }
    }

    // ─── 布局 / 输入 / 生命周期 ─────────────────────────────────────────────

    auto ListView::is_focusable() const -> bool {
        return true;
    }

    auto ListView::semantics_properties() const -> semantics::Properties {
        const bool focused_now = get_tree() != nullptr && get_tree()->focused_node() == this;
        return {
            .role = semantics::Role::list,
            .state = {.focusable = true, .focused = focused_now},
        };
    }

    auto ListView::on_measure(const foundation::NanLayoutConstraints constraints)
        -> foundation::NanSize {
        const float cross_width = width() > 0.0F ? width() : constraints.max_width;
        const float measured_width = std::isfinite(cross_width) ? cross_width : constraints.min_width;
        const double content_height =
            static_cast<double>(row_count()) * static_cast<double>(row_height_);
        if (!std::isfinite(measured_width) || measured_width < 0.0F
            || !std::isfinite(content_height) || content_height < 0.0
            || content_height > static_cast<double>(std::numeric_limits<float>::max()))
        {
            throw std::invalid_argument(
                "ListView content extent must fit finite layout coordinates"
            );
        }
        return constraints.constrain(
            foundation::NanSize(measured_width, static_cast<float>(content_height))
        );
    }

    auto ListView::on_layout() -> void {
        const auto math = window_math();
        // 首布局前挂起的滚动目标在这里按真实视口高度落位。
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
            // 布局遍历期不能增删节点；post_layout 物化后由第二次布局落位。
            schedule_window_sync();
        }
    }

    auto ListView::on_input(scene::InputEvent& event) -> bool {
        if (event.type() == scene::EventType::mouse_button) {
            auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
            if (mouse.button() != scene::MouseButtonEvent::Button::left || !mouse.is_pressed()) {
                return false;
            }
            const auto row = row_at(to_local(mouse.screen_pos()));
            if (!row.has_value()) {
                return false;
            }
            if (auto* tree = get_tree()) {
                tree->set_focus(this);
            }
            current_row_ = *row;
            const auto found = row_index_.find(*row);
            const std::size_t display = found == row_index_.end() ? 0 : found->second;
            const auto mods = mouse.modifiers();
            if (mods.shift) {
                const auto anchor = selection_.anchor().value_or(*row);
                (void)selection_.select_range(ordered_rows_, anchor, *row);
            }
            else if (mods.ctrl && selection_.mode() != SelectionMode::single) {
                (void)selection_.toggle(*row);
            }
            else {
                // 普通点击 = 独占选中：multiple 模式下也清掉其它行。
                (void)selection_.select_range(ordered_rows_, *row, *row);
            }
            scroll_to_row(display);
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

    void ListView::on_enter_tree() {
        scene::NanControl::on_enter_tree();
        subscribe_model();
    }

    void ListView::on_ready() {
        scene::NanControl::on_ready();
        rebuild_all();
    }

    void ListView::on_exit_tree() {
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

    void ListView::on_theme_changed(const theme::ThemeManager&) {
        // 作者 delegate 通过新的 ItemState.theme_revision 感知主题变化；控件自身的主题
        // 由场景树传播更新。
        refresh_item_states();
        mark_layout_dirty();
    }
} // namespace nandina::widget
