//
// widget/virtual_list_view — uniform-row virtualized data list with shared selection.
//
// 数据组件的虚拟化 / 选择依据落地点：只物化视口内可见行（每行等高），滚出视口的
// 行销毁、滚回视口的行按 key 重新创建；选中状态由 SelectionModel 持有，行身份即
// 稳定 key。规则与边界见 docs/references/data_components_and_virtualization.md。
//
// 结构与 ForEach / ListView 的分工：
//   * ForEach 全量键控对账；ListView 只是它的 typed 包装。
//   * VirtualListView 内部持有一个 ForEach 窗口区域，但只把可见窗口交给
//     region->set_items()：数据源全量保留在 source_items_ 中，窗口外的行不占控件、
//     不占 effect scope。
//   * 不直接继承 ForEach：Flex::add 每次都会对自身做 loose relayout，会把自尺寸
//     虚拟列表撑到内容高并立刻全量物化；窗口区域作为子控件吸收这份 relayout，
//     列表自身始终按父布局给的视口盒工作。
//
// 本类自身就是滚动视口（垂直单列），不套 ScrollView：滚动偏移驱动窗口重算，
// 父布局给它的盒子就是视口。变高行、水平滚动、行内交互控件与无障碍语义属于
// 后续边界（见文档）。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_VIRTUAL_LIST_VIEW_HPP
#define NANDINA_EXPERIMENT_WIDGET_VIRTUAL_LIST_VIEW_HPP

#include "../reactive/event.hpp"
#include "../scene/input_event.hpp"
#include "declarative.hpp"
#include "list_view.hpp"
#include "roving_focus.hpp"
#include "selection_model.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nandina::widget
{
    /// 统一行高虚拟化 + SelectionModel 合成的数据列表。
    ///
    /// create / update 与 ForEach 同形：create 按 Item 构建行控件，行只负责展示，
    /// 本类负责行定位、滚动、点击 / 键盘选择。行控件可以带子控件，但行自身按
    /// 行高被 tight 摆放，任何内部布局都发生在 (row_width × row_height) 盒子内。
    ///
    /// 必须以 shared_ptr 持有（create() 构造）：布局期延迟物化使用
    /// shared_from_this() 挂 post_layout 动作（与 ScrollView 相同的共享所有权约定），
    /// 栈上实例会在首布局时触发 bad_weak_ptr。列表自身不限制高度：视口高度由
    /// 外部布局决定（set_height / Expanded / 父级 tight 约束）；父级给无界高度时
    /// 整个列表可见，此时物化全部行是正确语义。键类型 Key 必须可哈希
    /// （std::hash<Key> 可用：内部 unordered_map / unordered_set 按键对账与去重）。
    template<typename Item, std::equality_comparable Key, typename NodeT = scene::NanControl>
        requires std::derived_from<NodeT, scene::NanControl>
    class VirtualListView final: public scene::NanControl {
    public:
        using KeyFunction = std::function<Key(const Item&)>;
        using CreateFunction =
            std::function<std::shared_ptr<NodeT>(reactive::ReactiveScope&, const Item&)>;
        using UpdateFunction = std::function<void(NodeT&, const Item&)>;

        VirtualListView(
            reactive::Graph& graph,
            KeyFunction key,
            CreateFunction create,
            UpdateFunction update = {},
            float row_height = 32.0F
        ):
            region_(
                ForEach<Item, Key, NodeT>::create(
                    graph,
                    key,
                    std::move(create),
                    std::move(update),
                    LayoutAxis::vertical
                )
            ),
            key_(std::move(key)),
            binding_scope_(graph) {
            set_row_height(row_height);
            // 自身是滚动视口：滚出盒子的行不得绘制到视口外（与 ScrollView 同语义）。
            set_overflow(scene::ControlOverflow::clip);
            add_child(region_);
        }

        [[nodiscard]] static auto create(
            reactive::Graph& graph,
            KeyFunction key,
            CreateFunction create,
            UpdateFunction update = {},
            float row_height = 32.0F
        ) -> std::shared_ptr<VirtualListView> {
            return std::make_shared<VirtualListView>(
                graph,
                std::move(key),
                std::move(create),
                std::move(update),
                row_height
            );
        }

        // ─── 行高 / 数据源 ───────────────────────────────────────────────

        /// 统一行高：必须有限且 > 0；行内任何控件的自然高度都不改变行高。
        void set_row_height(const float row_height) {
            if (!std::isfinite(row_height) || row_height <= 0.0F) {
                throw std::invalid_argument(
                    "VirtualListView row height must be finite and positive"
                );
            }
            row_height_ = row_height;
            clamp_offset();
            sync_or_schedule();
            mark_layout_dirty();
        }

        [[nodiscard]] auto row_height() const -> float {
            return row_height_;
        }

        /// 直接推送完整数据。键必须唯一；重复键抛出 invalid_argument 且不改动任何状态。
        void set_source(const std::vector<Item>& items) {
            // 先在局部构建键表：键不唯一时抛出，任何成员都不被污染。
            std::vector<Key> keys;
            keys.reserve(items.size());
            std::unordered_map<Key, std::size_t> index;
            index.reserve(items.size());
            for (std::size_t item_index = 0; item_index < items.size(); ++item_index) {
                const auto key = key_(items[item_index]);
                if (!index.try_emplace(key, item_index).second) {
                    throw std::invalid_argument("VirtualListView keys must be unique");
                }
                keys.push_back(std::move(key));
            }
            source_items_ = items;
            source_keys_ = std::move(keys);
            source_index_ = std::move(index);
            ++source_generation_;
            refresh_label_cache();
            focus_.reset_typeahead();
            focus_.sync(source_keys_.size(), {}, label_wrapper());
            selection_.retain(source_keys_);
            clamp_offset();
            sync_or_schedule();
            mark_layout_dirty();
        }

        /// 绑定 tracked 数据源（与 ListView::set_model 同形）：源 get() 变化时
        /// 自动 set_source。绑定在退出树或控件销毁时解绑。
        template<typename Source>
            requires ListDataModelSource<Source, Item>
        void set_model(Source& source) {
            binding_ = [&source](reactive::EffectScope& scope, VirtualListView& list) {
                scope.add([&source, &list] { list.set_source(source.get()); });
            };
            if (is_inside_tree()) {
                activate_binding();
            }
        }

        /// typeahead 匹配文本回调；未设置时方向键 / Home / End 仍可用，输入不匹配。
        /// 回调返回 std::string（按值拷贝）：列表把标签快照进自身缓存，typeahead
        /// 匹配的是缓存内数据，杜绝「返回临时字符串再转 string_view」的悬垂
        /// （ASan stack-use-after-return；见 §6 红测 10）。
        void set_label_function(std::function<std::string(const Item&)> label) {
            label_ = std::move(label);
            refresh_label_cache();
            focus_.reset_typeahead();
            focus_.sync(source_keys_.size(), {}, label_wrapper());
        }

        // ─── 滚动 ────────────────────────────────────────────────────────

        void set_scroll_offset(foundation::NanPoint offset) {
            offset_.set_x(0.0F);
            offset_.set_y(offset.get_y());
            clamp_offset();
            mark_dirty(scene::DirtyFlags::paint);
            sync_or_schedule();
            position_window();
        }

        auto scroll_by(foundation::NanPoint delta) -> VirtualListView& {
            set_scroll_offset(foundation::NanPoint(0.0F, offset_.get_y() + delta.get_y()));
            return *this;
        }

        /// 滚动到让 index 行完整可见（不足视口高时贴顶 / 贴底）。
        /// 尚无视口高度（首布局前）时挂起目标索引，由 on_layout 拿到高度后落位，
        /// 避免按零高度视口把偏移跳到行底、布局后目标行反而滚出视口。
        void scroll_to_index(std::size_t index) {
            if (index >= source_keys_.size()) {
                pending_scroll_index_.reset();
                return;
            }
            if (height() <= 0.0F) {
                pending_scroll_index_ = index;
                return;
            }
            pending_scroll_index_.reset();
            const float row_top = static_cast<float>(index) * row_height_;
            const float row_bottom = row_top + row_height_;
            const float viewport_bottom = offset_.get_y() + height();
            float new_y = offset_.get_y();
            if (row_top < offset_.get_y()) {
                new_y = row_top;
            }
            else if (row_bottom > viewport_bottom) {
                new_y = row_bottom - height();
            }
            set_scroll_offset(foundation::NanPoint(0.0F, new_y));
        }

        void set_wheel_step(const float step) {
            if (!std::isfinite(step) || step < 0.0F) {
                throw std::invalid_argument(
                    "VirtualListView wheel step must be finite and non-negative"
                );
            }
            wheel_step_ = step;
        }

        [[nodiscard]] auto scroll_offset() const -> foundation::NanPoint {
            return offset_;
        }
        [[nodiscard]] auto maximum_scroll_offset() const -> foundation::NanPoint {
            return foundation::NanPoint(0.0F, std::max(0.0F, content_height() - height()));
        }
        /// 全量内容高度（行数 × 行高），与父布局看到的盒子高度无关。
        [[nodiscard]] auto content_size() const -> foundation::NanSize {
            return foundation::NanSize(std::max(0.0F, width()), content_height());
        }

        // ─── 数据查询 ────────────────────────────────────────────────────

        /// 全量行数（含未物化的行）。
        [[nodiscard]] auto item_count() const -> std::size_t {
            return source_keys_.size();
        }
        /// 第 index 行的稳定 key。
        [[nodiscard]] auto key_at(std::size_t index) const -> const Key& {
            if (index >= source_keys_.size()) {
                throw std::out_of_range("VirtualListView::key_at index out of range");
            }
            return source_keys_[index];
        }
        /// 行控件的稳定 key（该行在当前窗口内才有节点；窗口外返回 nullptr）。
        [[nodiscard]] auto row_node(const Key& key) const -> NodeT* {
            return region_->node_for(key);
        }

        // ─── 选择 ────────────────────────────────────────────────────────

        void set_selection_mode(SelectionMode mode) {
            selection_.set_mode(mode);
        }
        [[nodiscard]] auto selection_mode() const -> SelectionMode {
            return selection_.mode();
        }
        [[nodiscard]] auto is_selected(const Key& key) const -> bool {
            return selection_.contains(key);
        }
        [[nodiscard]] auto selected_count() const -> std::size_t {
            return selection_.count();
        }
        /// 选中 key 列表（首次选中顺序）。
        [[nodiscard]] auto selected_keys() const -> const std::vector<Key>& {
            return selection_.selected();
        }
        [[nodiscard]] auto first_selected() const -> std::optional<Key> {
            return selection_.first_selected();
        }

        /// 程序化选择：single 模式替换、multiple 模式追加；随后滚动到该行。
        auto select(const Key& key) -> bool {
            const auto found = source_index_.find(key);
            if (found == source_index_.end()) {
                return false;
            }
            const bool changed = selection_.select(key);
            scroll_to_index(found->second);
            return changed;
        }

        /// 范围选择（shift 点击 / 扩展路径）：在行序中选中 from 到 to 的跨度，
        /// 并总是滚动到 to（远端点）——与 select() 无条件滚动保持一致。
        auto select_range(const Key& from, const Key& to) -> bool {
            const bool changed = selection_.select_range(source_keys_, from, to);
            if (const auto index = find_index(to); index.has_value()) {
                scroll_to_index(*index);
            }
            return changed;
        }

        void clear_selection() {
            (void)selection_.clear();
        }

        /// 选中状态变化事件（无载荷：订阅方重新读取 selected_keys() / is_selected）。
        [[nodiscard]] auto selection_changed() const -> const reactive::Event<>& {
            return selection_.changed();
        }
        void set_on_selection_changed(std::function<void()> callback) {
            on_selection_changed_ = selection_.changed().subscribe(std::move(callback));
        }

        /// Enter / Space 激活当前行时触发，携带该行 key。
        [[nodiscard]] auto activated() const -> const reactive::Event<Key>& {
            return activated_;
        }
        void set_on_activate(std::function<void(const Key&)> callback) {
            on_activated_ = activated_.subscribe(std::move(callback));
        }

        // ─── 输入 / 生命周期 ─────────────────────────────────────────────

        [[nodiscard]] auto is_focusable() const -> bool override {
            return true;
        }

        auto on_input(scene::InputEvent& event) -> bool override {
            if (event.type() == scene::EventType::focus_leave) {
                // 失焦清空 typeahead 缓冲，避免回到列表后残留过期前缀
                // （与 Chip / DropdownMenu / Pagination 的既有约定一致）。
                focus_.reset_typeahead();
                return false;
            }
            if (event.type() == scene::EventType::mouse_button) {
                auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
                if (mouse.button() != scene::MouseButtonEvent::Button::left || !mouse.is_pressed())
                {
                    return false;
                }
                const auto local = to_local(mouse.screen_pos());
                const int index = hit_row(local.get_y());
                if (index < 0) {
                    return false;
                }
                handle_row_click(mouse, static_cast<std::size_t>(index), source_keys_[index]);
                event.accept();
                return true;
            }
            if (event.type() == scene::EventType::mouse_wheel) {
                auto& wheel = static_cast<scene::MouseWheelEvent&>(event);
                const float dy = -wheel.delta().get_y() * wheel_step_;
                const auto before = offset_.get_y();
                scroll_by(foundation::NanPoint(0.0F, dy));
                if (offset_.get_y() != before) {
                    event.accept();
                    return true;
                }
                return false;
            }
            if (event.type() == scene::EventType::key) {
                auto& key = static_cast<scene::KeyEvent&>(event);
                const bool handled = handle_key(key);
                if (handled) {
                    event.accept();
                }
                return handled;
            }
            if (event.type() == scene::EventType::text_input) {
                auto& text = static_cast<scene::TextInputEvent&>(event);
                const auto intent = focus_.handle_text(text);
                if (!intent.has_value()) {
                    return false;
                }
                const int index = intent->index;
                if (index >= 0) {
                    if (intent->selection_follows_focus
                        && selection_.mode() == SelectionMode::single)
                    {
                        (void)selection_.select(source_keys_[static_cast<std::size_t>(index)]);
                    }
                    scroll_to_index(static_cast<std::size_t>(index));
                }
                event.accept();
                return true;
            }
            return false;
        }

        void on_process(float dt) override {
            focus_.advance_time(dt);
        }

    protected:
        [[nodiscard]] auto on_measure(foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override {
            const float cross_width = width() > 0.0F ? width() : constraints.max_width;
            const float measured_width =
                std::isfinite(cross_width) ? cross_width : constraints.min_width;
            return constraints.constrain(foundation::NanSize(measured_width, content_height()));
        }

        auto on_layout() -> void override {
            clamp_offset();
            sync_or_schedule();
            position_window();
            if (pending_scroll_index_.has_value()) {
                const auto index = *pending_scroll_index_;
                pending_scroll_index_.reset();
                scroll_to_index(index);
            }
        }

        void on_ready() override {
            scene::NanControl::on_ready();
            activate_binding();
        }

        void on_exit_tree() override {
            binding_scope_.clear();
            binding_ = {};
            scene::NanControl::on_exit_tree();
        }

    private:
        static constexpr std::size_t k_no_window = std::numeric_limits<std::size_t>::max();

        [[nodiscard]] auto content_height() const -> float {
            return static_cast<float>(source_keys_.size()) * row_height_;
        }

        [[nodiscard]] auto has_window() const -> bool {
            return window_last_ != k_no_window;
        }

        /// 把行标签快照进自有缓存：typeahead 匹配只读缓存，不读用户回调的
        /// 临时返回值（见 set_label_function 注释）。
        void refresh_label_cache() {
            label_cache_.clear();
            label_cache_.reserve(source_items_.size());
            if (label_) {
                for (const auto& item: source_items_) {
                    label_cache_.push_back(label_(item));
                }
            }
        }

        [[nodiscard]] auto label_wrapper() const -> std::function<std::string_view(std::size_t)> {
            if (!label_) {
                return {};
            }
            return [this](const std::size_t index) -> std::string_view {
                if (index >= label_cache_.size()) {
                    return {};
                }
                return label_cache_[index];
            };
        }

        /// 把浮点行坐标夹进 [0, total-1]。
        [[nodiscard]] auto clamp_row(float row) const -> std::size_t {
            const std::size_t total = source_keys_.size();
            if (total == 0) {
                return 0;
            }
            const auto row_index = static_cast<std::size_t>(std::floor(row));
            return std::min(row_index, total - 1);
        }

        [[nodiscard]] auto compute_window() const -> std::pair<std::size_t, std::size_t> {
            const std::size_t total = source_keys_.size();
            if (total == 0) {
                return {0, k_no_window};
            }
            const float viewport = std::max(0.0F, height());
            if (viewport <= 0.0F) {
                return {0, k_no_window};
            }
            const std::size_t first = clamp_row(offset_.get_y() / row_height_);
            const std::size_t last =
                clamp_row(std::ceil((offset_.get_y() + viewport) / row_height_) - 1.0F);
            return {first, std::max(first, last)};
        }

        void clamp_offset() {
            const float maximum = std::max(0.0F, content_height() - height());
            offset_.set_y(std::clamp(offset_.get_y(), 0.0F, maximum));
        }

        [[nodiscard]] auto find_index(const Key& key) const -> std::optional<std::size_t> {
            const auto found = source_index_.find(key);
            if (found == source_index_.end()) {
                return std::nullopt;
            }
            return found->second;
        }

        /// 源 / 滚动 / 行高变化后重算窗口。布局期之外的调用直接对账；
        /// 布局期（含 input 等遍历期）内改窗口会延迟到 post_layout 再物化。
        /// 窗口区间相同但数据代次变化时也必须推送 set_items（同窗口数据替换
        /// 要让 update_ 生效 / 换键），不能只按区间比较早退。
        void sync_or_schedule() {
            const auto [first, last] = compute_window();
            if (first == window_first_ && last == window_last_
                && source_generation_ == synced_generation_)
            {
                return;
            }
            if (auto* tree = get_tree();
                tree != nullptr && tree->phase() != scene::FramePhase::idle)
            {
                schedule_sync_window();
                return;
            }
            sync_window();
        }

        /// 真正物化窗口：只把可见行交给窗口区域的 set_items（键控对账复用行控件）。
        void sync_window() {
            const auto [first, last] = compute_window();
            if (first == window_first_ && last == window_last_
                && source_generation_ == synced_generation_)
            {
                position_window();
                return;
            }
            window_first_ = first;
            window_last_ = last;
            std::vector<Item> window;
            if (has_window()) {
                window.reserve(window_last_ - window_first_ + 1);
                for (std::size_t index = window_first_; index <= window_last_; ++index) {
                    window.push_back(source_items_[index]);
                }
            }
            region_->set_items(std::move(window));
            synced_generation_ = source_generation_;
            position_window();
        }

        /// 布局期窗口变化的延迟物化：post_layout 动作触发第二次布局，行落位正确。
        void schedule_sync_window() {
            auto* tree = get_tree();
            if (tree == nullptr) {
                sync_window();
                return;
            }
            auto weak = std::weak_ptr<VirtualListView>(
                std::static_pointer_cast<VirtualListView>(shared_from_this())
            );
            tree->post_layout([weak = std::move(weak), tree] {
                if (auto list = weak.lock(); list != nullptr && list->get_tree() == tree) {
                    list->sync_window();
                }
            });
        }

        /// 摆放窗口区域（整体平移）与区域内各行（区域局部坐标，按行高对齐）。
        void position_window() {
            if (!has_window()) {
                region_->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 0.0F, 0.0F));
                return;
            }
            const float list_width = std::max(0.0F, width());
            region_->layout_to(
                foundation::NanRect::from_xywh(
                    0.0F,
                    static_cast<float>(window_first_) * row_height_ - offset_.get_y(),
                    list_width,
                    static_cast<float>(window_last_ - window_first_ + 1) * row_height_
                )
            );
            for (std::size_t index = window_first_; index <= window_last_; ++index) {
                auto* node = region_->node_for(source_keys_[index]);
                if (node != nullptr) {
                    node->layout_to(
                        foundation::NanRect::from_xywh(
                            0.0F,
                            static_cast<float>(index - window_first_) * row_height_,
                            list_width,
                            row_height_
                        )
                    );
                }
            }
        }

        [[nodiscard]] auto hit_row(float local_y) const -> int {
            if (local_y < 0.0F || local_y > height()) {
                return -1;
            }
            const int index =
                static_cast<int>(std::floor((offset_.get_y() + local_y) / row_height_));
            if (index < 0 || static_cast<std::size_t>(index) >= source_keys_.size()) {
                return -1;
            }
            // 底边（local_y == height()，视口高为行高整数倍时）按未物化的下一行
            // 命中：夹回窗口上界，避免点击底边选中不可见行。半可见行（视口高不是
            // 行高整数倍）仍在窗口内，不受影响。
            if (has_window() && static_cast<std::size_t>(index) > window_last_) {
                return static_cast<int>(window_last_);
            }
            return index;
        }

        /// 点击落地：ctrl 切换、shift 范围、普通点击独占选中；同时把焦点交给列表。
        void
        handle_row_click(scene::MouseButtonEvent& mouse, const std::size_t index, const Key& key) {
            if (auto* tree = get_tree()) {
                tree->set_focus(this);
            }
            const auto mods = mouse.modifiers();
            if (mods.ctrl && selection_.mode() != SelectionMode::single) {
                (void)selection_.toggle(key);
            }
            else if (mods.shift) {
                const auto anchor = selection_.anchor().value_or(key);
                (void)selection_.select_range(source_keys_, anchor, key);
            }
            else {
                // 普通点击 = 独占选中：multiple 模式下也清掉其它行。
                (void)selection_.select_range(source_keys_, key, key);
            }
        }

        /// 键盘落地：Enter 激活活动行、Space 切换活动行选中；方向键 / Home / End /
        /// PageUp / PageDown 走 RovingFocus，single 模式选中跟随，multiple 模式只移动活动行。
        [[nodiscard]] auto handle_key(scene::KeyEvent& key_event) -> bool {
            if (!key_event.is_press_or_repeat()) {
                return false;
            }
            if (key_event.is_pressed()) {
                const int active = focus_.active_index();
                if (key_event.logical_key() == foundation::LogicalKey::enter) {
                    if (active >= 0) {
                        activated_.emit(source_keys_[static_cast<std::size_t>(active)]);
                        return true;
                    }
                    return false;
                }
                if (key_event.logical_key() == foundation::LogicalKey::space) {
                    if (active >= 0) {
                        (void)selection_.toggle(source_keys_[static_cast<std::size_t>(active)]);
                        return true;
                    }
                    return false;
                }
            }
            const auto intent = focus_.handle_key(key_event);
            if (!intent.has_value()) {
                return false;
            }
            const int index = intent->index;
            if (index >= 0 && intent->selection_follows_focus
                && selection_.mode() == SelectionMode::single)
            {
                (void)selection_.select(source_keys_[static_cast<std::size_t>(index)]);
            }
            if (index >= 0) {
                scroll_to_index(static_cast<std::size_t>(index));
            }
            return true;
        }

        void activate_binding() {
            binding_scope_.clear();
            if (binding_) {
                binding_(binding_scope_, *this);
            }
        }

        std::shared_ptr<ForEach<Item, Key, NodeT>> region_;
        float row_height_ = 32.0F;
        float wheel_step_ = 40.0F;
        KeyFunction key_;
        std::vector<Item> source_items_;
        std::vector<Key> source_keys_;
        std::unordered_map<Key, std::size_t> source_index_;
        foundation::NanPoint offset_ {};
        std::size_t window_first_ = 0;
        std::size_t window_last_ = k_no_window;
        std::size_t source_generation_ = 0;
        std::size_t synced_generation_ = 0;
        std::optional<std::size_t> pending_scroll_index_ = std::nullopt;
        RovingFocus focus_;
        SelectionModel<Key> selection_;
        reactive::Event<Key> activated_;
        reactive::Subscription on_selection_changed_;
        reactive::Subscription on_activated_;
        std::function<void(reactive::EffectScope&, VirtualListView&)> binding_;
        reactive::EffectScope binding_scope_;
        std::function<std::string(const Item&)> label_;
        std::vector<std::string> label_cache_;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_VIRTUAL_LIST_VIEW_HPP
