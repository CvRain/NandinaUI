//
// widget/combobox - filterable text input with a dropdown list of options.
//
// 本文件定义两个内部类型（不进入公开头文件）：
//   * `internal::ComboboxOptionNode`——单个选项行的渲染 + 无障碍节点；
//   * `internal::ComboboxListSurface`——过滤后条目列表的容器，持有漫游与指针命中。
//
// 两者都由 Combobox 私有持有：列表表面是 Popover 的 content，选项行是它的子节点。
// 选项行不参与命中测试（`contains_point` 返回 false）：指针一律由列表表面按 y 坐标
// 解析到条目下标，这样悬停 / 点击与键盘高亮走同一条 `active_index` 路径。
//
// 与 DropdownMenu 的 `internal::MenuSurface` 是同一套模式（见该文件的说明）：两者都
// 消费 `MenuItem` 模型与共享的 `RovingFocus`。这里没有把 MenuSurface 直接复用，因为
// 它绑定动作菜单的语义（勾选指示、子菜单箭头、MenuSelection），而 Combobox 的列表只
// 有「标签 + 禁用 + 高亮」三件事，多出来的分支只会让两边都更难读。列表表面也不持有
// 焦点：焦点必须留在输入框上，否则输入无法继续过滤（见 combobox.hpp 的焦点模型）。
//

#include "combobox.hpp"

#include "key_codes.hpp"
#include "popover.hpp"
#include "primitives/box_painter.hpp"
#include "primitives/text.hpp"
#include "roving_focus.hpp"
#include "text_field.hpp"

#include "../render/draw_context.hpp"
#include "../scene/input_event.hpp"
#include "../scene/overlay_host.hpp"
#include "../scene/scene_tree.hpp"
#include "../theme/theme_manager.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        [[nodiscard]] auto near(const float lhs, const float rhs) -> bool {
            return std::abs(lhs - rhs) <= foundation::nan_epsilon;
        }

        [[nodiscard]] auto
        same_text_style(const primitives::TextStyle& lhs, const primitives::TextStyle& rhs)
            -> bool {
            return lhs.color.approx_equals(rhs.color) && near(lhs.font_size, rhs.font_size)
                && lhs.font == rhs.font && lhs.overflow == rhs.overflow
                && lhs.max_lines == rhs.max_lines;
        }

        /// 从解析后的排版 + 继承的样式上下文构造文本样式（与 DropdownMenu 同款）。
        [[nodiscard]] auto make_text_style(
            const theme::ResolvedStyleContext& context,
            const theme::ResolvedTypeStyle& type,
            const text::FontRequest& fallback_font
        ) -> primitives::TextStyle {
            return primitives::TextStyle {
                .color = context.text_color_from_context ? context.text_color : type.color,
                .font_size = context.font_size_from_context ? context.font_size : type.font_size,
                .font = context.font_from_context ? context.font : fallback_font,
                .overflow = primitives::TextOverflow::ellipsis,
                .max_lines = 1,
            };
        }

        [[nodiscard]] auto lower_ascii(const char value) -> char {
            return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
        }

        /// 大小写不敏感（ASCII）子串匹配。过滤规则见 menu_model.md 的 typeahead 约定：
        /// 只匹配 label，`shortcut` 不参与。
        [[nodiscard]] auto contains_case_insensitive(
            const std::string_view haystack,
            const std::string_view needle
        ) -> bool {
            if (needle.empty()) {
                return true;
            }
            if (haystack.size() < needle.size()) {
                return false;
            }
            for (std::size_t start = 0; start + needle.size() <= haystack.size(); ++start) {
                bool matched = true;
                for (std::size_t i = 0; i < needle.size(); ++i) {
                    if (lower_ascii(haystack[start + i]) != lower_ascii(needle[i])) {
                        matched = false;
                        break;
                    }
                }
                if (matched) {
                    return true;
                }
            }
            return false;
        }
    } // namespace

    namespace internal
    {
        /**
         * 单个选项行：只负责“这一行长什么样”与“读屏听到什么”。
         *
         * 高亮 / 悬停由 ComboboxListSurface 统一下发，指针命中由列表表面按 y 坐标完成。
         */
        class ComboboxOptionNode final: public scene::NanControl {
        public:
            /// 从模型同步展示字段（禁用可以在不重建节点的情况下变化）。
            void sync(const MenuItem& item) {
                disabled_ = item.disabled;
                if (label_ != item.label) {
                    label_ = item.label;
                    label_text_.set_text(label_);
                }
                mark_layout_dirty();
                mark_semantics_dirty();
                mark_dirty(scene::DirtyFlags::paint);
            }

            void set_style(
                theme::ResolvedComboboxStyle style,
                const theme::ResolvedStyleContext& context
            ) {
                style_ = std::move(style);
                context_ = context;
                apply_text_style();
                mark_layout_dirty();
                mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
            }

            void set_state(const bool hovered, const bool highlighted) {
                if (hovered_ == hovered && highlighted_ == highlighted) {
                    return;
                }
                hovered_ = hovered;
                highlighted_ = highlighted;
                mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
            }

            [[nodiscard]] auto is_focusable() const -> bool override {
                return false;
            }

            /// 命中一律交给列表表面：选项行只提供几何与语义。
            [[nodiscard]] auto contains_point(foundation::NanPoint) const -> bool override {
                return false;
            }

            [[nodiscard]] auto semantics_properties() const -> semantics::Properties override {
                return {
                    .role = semantics::Role::list_item,
                    .label = label_,
                    .state = {
                        .focusable = false,
                        .focused = highlighted_,
                        .disabled = disabled_,
                    },
                    .actions = disabled_ ? semantics::Action::none : semantics::Action::activate,
                };
            }

        protected:
            [[nodiscard]] auto on_measure(const scene::LayoutConstraints constraints)
                -> foundation::NanSize override {
                (void)label_text_.measure_layout(scene::LayoutConstraints::loose());
                const auto& m = style_.metrics;
                return constraints.constrain(
                    foundation::NanSize(
                        label_text_.measured_text_width() + m.list_padding_x * 2.0F,
                        m.item_height
                    )
                );
            }

            auto on_draw(render::DrawContext& context) -> void override {
                const auto world =
                    render::world_bounds_from_local(context.world_transform(), local_rect());
                const float opacity = context.opacity();
                const auto& m = style_.metrics;

                // 悬停优先于键盘高亮：鼠标在条目上时按 hover 面绘制（同 DropdownMenu）。
                if (hovered_ || highlighted_) {
                    primitives::BoxPainter::paint_fill(
                        context,
                        world,
                        theme::ResolvedBoxStyle {
                            .fill = hovered_ ? style_.hover_fill : style_.focus_fill,
                            .border = style_.focus_fill.with_alpha(0.0F),
                            .border_width = 0.0F,
                            .radius = m.item_radius,
                        },
                        opacity
                    );
                }

                const float pad_x = context.logical_to_screen(m.list_padding_x);
                const float text_height =
                    context.logical_to_screen(label_text_.measured_text_height());
                label_text_.draw_at(
                    context,
                    foundation::NanPoint(
                        world.get_left() + pad_x,
                        world.get_top() + (world.get_height() - text_height) * 0.5F
                    )
                );
            }

        private:
            void apply_text_style() {
                auto style = make_text_style(context_, style_.option, label_text_.font());
                if (disabled_) {
                    style.color = style_.disabled_label;
                }
                if (!same_text_style(label_text_.style(), style)) {
                    label_text_.set_style(style);
                }
            }

            std::string label_;
            bool disabled_ = false;
            bool hovered_ = false;
            bool highlighted_ = false;
            theme::ResolvedComboboxStyle style_;
            theme::ResolvedStyleContext context_;
            primitives::Text label_text_;
        };

        /**
         * 过滤后条目列表：Popover 的 content。
         *
         * 持有**副本**（`set_items` 进来的是过滤结果），因此 Combobox 换掉条目模型时这里
         * 不会留下悬垂引用。表面自身不可聚焦：焦点必须留在输入框上，否则用户无法继续
         * 输入过滤；点击选项行通过 `focus_delegate()` 仍然落在输入框。
         */
        class ComboboxListSurface final: public scene::NanControl {
        public:
            explicit ComboboxListSurface(std::function<void(std::string_view)> on_activate):
                on_activate_(std::move(on_activate)) {
                focus_.set_movement(RovingMovement::selection_only);
                focus_.set_orientation(RovingOrientation::vertical);
            }

            void set_items(std::vector<MenuItem> items) {
                items_ = std::move(items);
                rebuild_nodes();
                reset_highlight();
                mark_dirty(
                    scene::DirtyFlags::paint | scene::DirtyFlags::layout
                    | scene::DirtyFlags::semantics
                );
            }

            [[nodiscard]] auto items() const -> const std::vector<MenuItem>& {
                return items_;
            }

            void set_style(
                theme::ResolvedComboboxStyle style,
                const theme::ResolvedStyleContext& context
            ) {
                style_ = std::move(style);
                context_ = context;
                style_ready_ = true;
                for (auto& node: item_nodes_) {
                    node->set_style(style_, context_);
                }
                mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
            }

            /// 点击选项行时把焦点交还给输入框（表面自身不可聚焦）。
            void set_focus_delegate(std::weak_ptr<scene::NanControl> delegate) {
                delegate_ = std::move(delegate);
            }

            /// 高亮落到第一个可聚焦条目（打开 / 重新过滤时调用）。
            void reset_highlight() {
                set_highlight_edge(/*from_end=*/false);
            }

            /// 高亮落到首个或末个可聚焦条目。关闭状态下按 Down / Up 打开时用它：
            /// 这一次按键只负责"打开并落位"，不再叠加一次移动。
            void set_highlight_edge(const bool from_end) {
                sync_roving();
                focus_.set_active_index(from_end ? last_focusable() : first_focusable());
                sync_highlight_states();
            }

            void clear_hover() {
                if (hovered_ < 0) {
                    return;
                }
                hovered_ = -1;
                sync_highlight_states();
            }

            [[nodiscard]] auto active_index() const -> int {
                return focus_.active_index();
            }

            /// 把导航键交给共享漫游设施；返回是否消费。
            auto handle_key(const scene::KeyEvent& event) -> bool {
                if (!event.is_pressed()) {
                    return false;
                }
                if (!focus_.handle_key(event).has_value()) {
                    return false;
                }
                // 键盘导航结束指针悬停：此后的高亮用 focus 配色。
                hovered_ = -1;
                sync_highlight_states();
                return true;
            }

            [[nodiscard]] auto is_focusable() const -> bool override {
                return false;
            }

            [[nodiscard]] auto focus_delegate() const -> scene::NanNode2D* override {
                return delegate_.lock().get();
            }

            auto on_input(scene::InputEvent& event) -> bool override {
                switch (event.type()) {
                    case scene::EventType::focus_enter:
                        mark_semantics_dirty();
                        return false;
                    case scene::EventType::focus_leave:
                        mark_semantics_dirty();
                        return false;
                    case scene::EventType::mouse_move:
                        return handle_mouse_move(static_cast<scene::MouseMoveEvent&>(event));
                    case scene::EventType::mouse_leave:
                        clear_hover();
                        return false;
                    case scene::EventType::mouse_button:
                        return handle_mouse_button(static_cast<scene::MouseButtonEvent&>(event));
                    default:
                        return false;
                }
            }

        protected:
            [[nodiscard]] auto on_measure(const scene::LayoutConstraints constraints)
                -> foundation::NanSize override {
                const auto& m = style_.metrics;
                float content_width = 0.0F;
                float content_height = m.list_padding_y * 2.0F;
                for (auto& node: item_nodes_) {
                    const auto measured = node->measure_layout(scene::LayoutConstraints::loose());
                    content_width = std::max(content_width, measured.get_width());
                    content_height += measured.get_height();
                }
                return constraints.constrain(
                    foundation::NanSize(
                        std::max(m.min_width, content_width + m.list_padding_x * 2.0F),
                        content_height
                    )
                );
            }

            void on_layout() override {
                const auto& m = style_.metrics;
                const float content_width = std::max(0.0F, width() - m.list_padding_x * 2.0F);
                float y = m.list_padding_y;
                for (auto& node: item_nodes_) {
                    const auto measured = node->measure_layout(scene::LayoutConstraints::loose());
                    const float row_height = measured.get_height();
                    node->layout_to(
                        foundation::NanRect::from_xywh(m.list_padding_x, y, content_width, row_height)
                    );
                    y += row_height;
                }
            }

            [[nodiscard]] auto semantics_properties() const -> semantics::Properties override {
                // 与 DropdownMenu 的 MenuSurface 同款：语义角色返回列表容器，条目行分别
                // 用 Role::list_item 暴露标签与禁用状态。
                return {
                    .role = semantics::Role::list,
                    .label = "options",
                    .state = {.focusable = false},
                };
            }

        private:
            void rebuild_nodes() {
                for (auto& node: item_nodes_) {
                    if (node != nullptr && node->parent() == this) {
                        (void)remove_child(*node);
                    }
                }
                item_nodes_.clear();
                item_nodes_.reserve(items_.size());
                for (const auto& item: items_) {
                    auto node = std::make_shared<ComboboxOptionNode>();
                    node->sync(item);
                    if (style_ready_) {
                        node->set_style(style_, context_);
                    }
                    add_child(node);
                    item_nodes_.push_back(std::move(node));
                }
            }

            void sync_roving() {
                const auto& items = items_;
                focus_.sync(
                    items.size(),
                    [&items](const std::size_t index) {
                        return menu_item_is_focusable(items[index]);
                    },
                    [&items](const std::size_t index) -> std::string_view {
                        return menu_item_typeahead_text(items[index]);
                    }
                );
            }

            void sync_highlight_states() {
                const int active = focus_.active_index();
                for (std::size_t i = 0; i < item_nodes_.size(); ++i) {
                    item_nodes_[i]->set_state(
                        static_cast<int>(i) == hovered_,
                        static_cast<int>(i) == active
                    );
                }
            }

            [[nodiscard]] auto first_focusable() const -> int {
                for (std::size_t i = 0; i < items_.size(); ++i) {
                    if (menu_item_is_focusable(items_[i])) {
                        return static_cast<int>(i);
                    }
                }
                return -1;
            }

            [[nodiscard]] auto last_focusable() const -> int {
                for (std::size_t i = items_.size(); i > 0; --i) {
                    if (menu_item_is_focusable(items_[i - 1])) {
                        return static_cast<int>(i - 1);
                    }
                }
                return -1;
            }

            [[nodiscard]] auto hit_index(const float local_y) const -> int {
                for (std::size_t i = 0; i < item_nodes_.size(); ++i) {
                    const float top = item_nodes_[i]->position().get_y();
                    const float bottom = top + item_nodes_[i]->height();
                    if (local_y >= top && local_y <= bottom) {
                        return static_cast<int>(i);
                    }
                }
                return -1;
            }

            void activate_index(const int index) {
                if (index < 0 || static_cast<std::size_t>(index) >= items_.size()) {
                    return;
                }
                const auto& item = items_[static_cast<std::size_t>(index)];
                if (!menu_item_is_activatable(item)) {
                    return;
                }
                // 拷贝 id：用户回调里可能 set_items() 并重建整个列表。
                const std::string id = item.id;
                if (on_activate_) {
                    on_activate_(id);
                }
            }

            auto handle_mouse_move(scene::MouseMoveEvent& event) -> bool {
                const auto local = to_local(event.screen_pos());
                const int index = hit_index(local.get_y());
                if (index < 0) {
                    return false;
                }
                const auto target = static_cast<std::size_t>(index);
                if (!menu_item_is_focusable(items_[target])) {
                    return false;
                }
                hovered_ = index;
                focus_.set_active_index(index);
                sync_highlight_states();
                event.accept();
                return true;
            }

            auto handle_mouse_button(scene::MouseButtonEvent& event) -> bool {
                if (!event.is_pressed() || event.button() != scene::MouseButtonEvent::Button::left)
                {
                    return false;
                }
                const auto local = to_local(event.screen_pos());
                const int index = hit_index(local.get_y());
                if (index < 0) {
                    return false;
                }
                if (menu_item_is_focusable(items_[static_cast<std::size_t>(index)])) {
                    hovered_ = index;
                    focus_.set_active_index(index);
                    sync_highlight_states();
                }
                // 禁用条目在这里是 no-op（activate_index 会拒绝）。
                activate_index(index);
                event.accept();
                return true;
            }

            std::vector<MenuItem> items_;
            std::vector<std::shared_ptr<ComboboxOptionNode>> item_nodes_;
            RovingFocus focus_;
            std::weak_ptr<scene::NanControl> delegate_;
            int hovered_ = -1;
            bool style_ready_ = false;
            theme::ResolvedComboboxStyle style_;
            theme::ResolvedStyleContext context_;
            std::function<void(std::string_view)> on_activate_;
        };
    } // namespace internal

    Combobox::Combobox(
        std::vector<MenuItem> items,
        std::string value,
        std::string placeholder,
        theme::NanTheme theme
    ):
        items_(std::move(items)) {
        system_ =
            std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;

        // 可编辑文本：光标 / 选区 / 输入法 / 文本绘制全部复用 TextField。它的语义节点
        // 隐藏：可访问性的输入面是 Combobox 自己（否则读屏会听到两层输入框）。
        text_field_ = std::make_shared<TextField>(std::move(value), std::move(placeholder), theme);
        text_field_->set_semantics_composition(semantics::Composition::hidden);
        text_field_->set_on_change([this](const std::string_view text) {
            handle_field_text_changed(text);
        });

        // 浮层基座：面板配方由 Popover 自己解析，本组件只负责输入框与条目列表。
        popover_ = std::make_shared<Popover>(nullptr, nullptr, theme);
        surface_ = std::make_shared<internal::ComboboxListSurface>([this](const std::string_view id) {
            activate_id(id);
        });
        surface_->set_focus_delegate(text_field_);

        // 先给出真实配方再灌条目：条目节点在重建时就能拿到有效的字号 / 度量。
        sync_field_style();
        sync_surface_style();
        refresh_filter();
        popover_->set_content(surface_);
        popover_->set_trigger(text_field_);
        sync_popover_gap();
        popover_->set_on_close([this] { handle_closed(); });
        add_child(popover_);

        // 初始文本与选中的一致性：value 恰好等于某个可激活条目的 label 时即选中它。
        (void)sync_selected_from_text();
        if (!selected_id_.empty()) {
            committed_label_ = std::string(text());
        }
    }

    Combobox::~Combobox() = default;

    auto Combobox::create(
        std::vector<MenuItem> items,
        std::string value,
        std::string placeholder,
        theme::NanTheme theme
    ) -> std::shared_ptr<Combobox> {
        return std::make_shared<Combobox>(
            std::move(items), std::move(value), std::move(placeholder), theme
        );
    }

    // ─── 条目 ────────────────────────────────────────────────────────────

    void Combobox::set_items(std::vector<MenuItem> items) {
        items_ = std::move(items);
        // 旧的选中 id 可能已不存在；文本与选中保持一致，能对上就重新选中。
        (void)sync_selected_from_text();
        refresh_filter();
        // 列表表面是持久对象，过滤结果就地替换；面板尺寸由脏标记在下一轮布局中重算，
        // 不需要重建浮层 —— 重建会让 FocusScope 抢走输入焦点。
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto Combobox::items() const -> const std::vector<MenuItem>& {
        return items_;
    }

    auto Combobox::item_count() const -> std::size_t {
        return items_.size();
    }

    auto Combobox::filtered_ids() const -> std::vector<std::string> {
        std::vector<std::string> ids;
        ids.reserve(surface_->items().size());
        for (const auto& item: surface_->items()) {
            ids.push_back(item.id);
        }
        return ids;
    }

    // ─── 文本 / 值 ───────────────────────────────────────────────────────

    void Combobox::set_text(std::string text) {
        text_field_->set_value(std::move(text));
        change_emitted_ = false;
        (void)sync_selected_from_text();
        refresh_filter();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto Combobox::text() const -> std::string_view {
        return text_field_->value();
    }

    void Combobox::set_placeholder(std::string placeholder) {
        text_field_->set_placeholder(std::move(placeholder));
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto Combobox::placeholder() const -> std::string_view {
        return text_field_->placeholder();
    }

    auto Combobox::selected_id() const -> std::string_view {
        return selected_id_;
    }

    auto Combobox::set_selected_id(const std::string_view id) -> bool {
        const auto* item = find_menu_item(items_, id);
        if (item == nullptr || !menu_item_is_activatable(*item)) {
            return false;
        }
        // 先取副本：set_value 之后还会经过过滤与回调路径，不能持有条目指针。
        const std::string label = item->label;
        const std::string stable_id = item->id;
        text_field_->set_value(label);
        selected_id_ = stable_id;
        committed_label_ = label;
        change_emitted_ = false;
        refresh_filter();
        mark_layout_dirty();
        mark_semantics_dirty();
        return true;
    }

    void Combobox::clear_selection() {
        text_field_->set_value({});
        selected_id_.clear();
        committed_label_.clear();
        change_emitted_ = false;
        refresh_filter();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    void Combobox::set_allow_custom_value(const bool allow) {
        allow_custom_value_ = allow;
    }

    auto Combobox::allow_custom_value() const -> bool {
        return allow_custom_value_;
    }

    // ─── 浮层 ────────────────────────────────────────────────────────────

    void Combobox::open() {
        if (disabled_ || popover_->is_open()) {
            return;
        }
        refresh_filter();
        sync_popover_gap();
        popover_->open();
        // 托管完成后 FocusScope 会把焦点拿走（它需要一个焦点落点）。列表行刻意不可聚焦，
        // 因此这里把焦点交还输入框；托管尚未完成时（锚点还没布局）由 on_process 重试。
        focus_restore_pending_ = !restore_field_focus();
        mark_semantics_dirty();
    }

    void Combobox::close() {
        popover_->close();
    }

    auto Combobox::is_open() const -> bool {
        return popover_->is_open();
    }

    auto Combobox::active_index() const -> int {
        return surface_->active_index();
    }

    // ─── 回调 / 事件 ─────────────────────────────────────────────────────

    void Combobox::set_on_change(
        std::function<void(std::string_view text, std::string_view id)> callback
    ) {
        on_change_ = std::move(callback);
    }

    auto Combobox::item_selected() const -> const reactive::Event<std::string>& {
        return item_selected_;
    }

    void Combobox::set_on_close(std::function<void()> callback) {
        on_close_ = std::move(callback);
    }

    void Combobox::set_disabled(const bool disabled) {
        if (disabled_ == disabled) {
            return;
        }
        disabled_ = disabled;
        text_field_->set_disabled(disabled);
        if (disabled_) {
            close();
        }
        sync_field_style();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto Combobox::disabled() const -> bool {
        return disabled_;
    }

    // ─── 主题 ────────────────────────────────────────────────────────────

    void Combobox::set_theme(theme::NanTheme theme) {
        system_ =
            std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        // 输入框与面板同属一个控件：显式整份主题一并下发，避免外壳跟随系统而面板不跟随。
        text_field_->set_theme(theme);
        popover_->set_theme(theme);
        sync_field_style();
        sync_surface_style();
        mark_layout_dirty();
    }

    auto Combobox::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Combobox::set_override(theme::ComboboxRecipeRule rule) {
        override_ = std::move(rule);
        sync_field_style();
        sync_surface_style();
        mark_dirty(
            scene::DirtyFlags::paint | scene::DirtyFlags::layout | scene::DirtyFlags::semantics
        );
    }

    auto Combobox::resolved_style() const -> theme::ResolvedComboboxStyle {
        auto style = theme::resolve_combobox(*system_, appearance_, visual_state());
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    auto Combobox::visual_state() const -> theme::ComboboxVisualState {
        if (disabled_) {
            return theme::ComboboxVisualState::disabled;
        }
        if (focused_) {
            return theme::ComboboxVisualState::focused;
        }
        return theme::ComboboxVisualState::normal;
    }

    void Combobox::on_style_context_changed(const theme::ResolvedStyleContext& /*context*/) {
        sync_field_style();
        sync_surface_style();
        mark_layout_dirty();
    }

    void Combobox::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        sync_field_style();
        sync_surface_style();
        mark_layout_dirty();
    }

    // ─── 输入 / 生命周期 ─────────────────────────────────────────────────

    auto Combobox::on_input_capture(scene::InputEvent& event) -> bool {
        if (disabled_) {
            return false;
        }
        switch (event.type()) {
            case scene::EventType::focus_enter:
                focused_ = true;
                sync_field_style();
                mark_semantics_dirty();
                return false;
            case scene::EventType::focus_leave:
                focused_ = false;
                sync_field_style();
                mark_semantics_dirty();
                // 打开浮层期间 FocusScope 会短暂接管焦点，那不是用户失焦。
                if (!popover_->is_open()) {
                    commit_unmatched();
                }
                return false;
            case scene::EventType::key:
                return handle_capture_key(static_cast<scene::KeyEvent&>(event));
            default:
                return false;
        }
    }

    auto Combobox::handle_capture_key(scene::KeyEvent& event) -> bool {
        if (!event.is_pressed()) {
            return false;
        }
        const int code = event.keycode();
        const bool alt = event.modifiers().alt;

        if (code == keys::down || code == keys::up) {
            if (alt) {
                // Alt+Down / Alt+Up：显式开合，不移动高亮。
                code == keys::down ? open() : close();
                event.accept();
                return true;
            }
            if (!popover_->is_open()) {
                // 关闭状态下这一次按键只负责"打开并落在首/末项"。不再叠加一次移动，
                // 否则从关闭按 Down 会落到第 2 个匹配项，与平台惯例（Down 打开并
                // 高亮第一项、Up 打开并高亮最后一项）不符。
                open();
                surface_->set_highlight_edge(code == keys::up);
                event.accept();
                return true;
            }
            (void)surface_->handle_key(event);
            event.accept();
            return true;
        }

        if (code == keys::enter) {
            handle_capture_enter();
            event.accept();
            return true;
        }

        if (code == keys::escape) {
            if (popover_->is_open()) {
                close();
                event.accept();
                return true;
            }
            // 关闭状态下让 Escape 继续冒泡（对话框等外层可以据此关闭）。
            return false;
        }

        if (code == keys::home || code == keys::end || code == keys::page_up
            || code == keys::page_down)
        {
            if (popover_->is_open() && surface_->handle_key(event)) {
                event.accept();
                return true;
            }
            // 关闭时 Home / End 仍是输入框的光标移动，交给 TextField。
            return false;
        }

        if (code == keys::space) {
            // 消费空格键码，阻止 Popover 的「触发键开合」把输入空格当成开关；空格字符
            // 本身由平台的 TextInputEvent 送达（EditableText 只从 text_input 插入文本）。
            event.accept();
            return true;
        }

        // 可打印字符、Backspace、左右键、Ctrl+A 等一律落到 TextField。
        return false;
    }

    void Combobox::handle_capture_enter() {
        if (popover_->is_open()) {
            const int active = surface_->active_index();
            const auto& visible = surface_->items();
            if (active >= 0 && static_cast<std::size_t>(active) < visible.size()) {
                activate_id(visible[static_cast<std::size_t>(active)].id);
            }
            return;
        }
        // 关闭状态下的 Enter 是“提交当前文本”：
        //   * 文本恰好等于某个可激活条目的 label -> 提交这次匹配（程序化 set_text 静默，
        //     事件在这里补发；用户输入路径已被 notify_change 去重）；
        //   * 允许自定义值 -> 带出未匹配文本；
        //   * 否则是 no-op（绝不因此打开浮层）。
        if (sync_selected_from_text()) {
            notify_change(text_field_->value(), selected_id_);
            return;
        }
        if (allow_custom_value_) {
            notify_change(text_field_->value(), {});
        }
    }

    void Combobox::on_process(const float /*dt*/) {
        if (!popover_->is_open()) {
            focus_restore_pending_ = false;
            return;
        }
        // 首次打开时锚点可能尚未布局，浮层会在后续帧才托管成功；那时 FocusScope 才
        // 抢走焦点，所以恢复动作也要跟着重试。
        if (focus_restore_pending_ && restore_field_focus()) {
            focus_restore_pending_ = false;
        }
    }

    void Combobox::on_exit_tree() {
        surface_->clear_hover();
        focus_restore_pending_ = false;
        focused_ = false;
        scene::NanControl::on_exit_tree();
    }

    auto Combobox::z_index_hint() const -> int {
        return popover_ != nullptr ? popover_->z_index_hint() : 0;
    }

    // ─── 布局 / 语义 ─────────────────────────────────────────────────────

    auto Combobox::on_measure(const scene::LayoutConstraints constraints)
        -> foundation::NanSize {
        // Popover 的测量就是触发控件（输入框）的自然尺寸；再用配方的首选宽度 / 高度把
        // 空内容时的尺寸兜住，否则没有文本 / 占位时控件会塌成一条线。
        const auto field_size = popover_->measure_layout(scene::LayoutConstraints::loose());
        const auto style = resolved_style();
        return constraints.constrain(
            foundation::NanSize(
                std::max(field_size.get_width(), style.metrics.preferred_width),
                std::max(field_size.get_height(), style.metrics.height)
            )
        );
    }

    void Combobox::on_layout() {
        popover_->layout_to(local_rect());
    }

    auto Combobox::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::combobox,
            // Role 有 combobox（不同于菜单族的 Role 缺失），但 State 没有 `expanded`
            // 字段：展开状态沿用 Popover / DropdownMenu 的表达（`state.checked` 承载展开，
            // `value` 保留当前文本）。
            .label = std::string(text_field_->placeholder()),
            .value = std::string(text_field_->value()),
            .state = {
                .focusable = !disabled_,
                .focused = focused_,
                .disabled = disabled_,
                .checked = popover_->is_open(),
            },
            .actions = disabled_
                ? semantics::Action::none
                : (semantics::Action::focus | semantics::Action::set_value
                   | semantics::Action::activate),
        };
    }

    auto Combobox::on_semantics_action(const semantics::ActionRequest& request) -> bool {
        if (disabled_) {
            return false;
        }
        if (request.action == semantics::Action::activate) {
            open();
            return true;
        }
        if (request.action == semantics::Action::set_value) {
            set_text(request.value);
            return true;
        }
        return false;
    }

    // ─── 内部 ────────────────────────────────────────────────────────────

    void Combobox::set_overlay_service(scene::OverlayHost* host) noexcept {
        popover_->set_overlay_service(host);
    }

    void Combobox::refresh_filter() {
        const auto query = text_field_->value();
        std::vector<MenuItem> visible;
        visible.reserve(items_.size());
        for (const auto& item: items_) {
            // 结构性条目（separator / label）不列；disabled 条目仍列出（可聚焦、不可激活）。
            if (!menu_item_is_focusable(item)) {
                continue;
            }
            if (!contains_case_insensitive(item.label, query)) {
                continue;
            }
            // 副本而不是引用：`set_items` 换掉 items_ 的存储时列表不会留下悬垂引用。
            visible.push_back(item);
        }
        surface_->set_items(std::move(visible));
        sync_surface_style();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    void Combobox::sync_field_style() {
        const auto style = resolved_style();
        // 输入框外壳与文本都由组合的 TextField 绘制：把解析后的配方转成它的实例覆盖，
        // 这样 ComboboxRecipe 的每个字段都有真实视觉落点（不画第二层壳，避免双层描边）。
        text_field_->set_override(
            theme::TextFieldRecipeRule {
                .container_fill = theme::ThemeColor::literal(style.input.fill),
                .container_border = theme::ThemeColor::literal(style.input.border),
                .container_border_width = theme::ThemeScalar::literal(style.input.border_width),
                .container_radius = theme::ThemeScalar::literal(style.input.radius),
                .value_color = theme::ThemeColor::literal(style.value.color),
                .placeholder_color = theme::ThemeColor::literal(style.placeholder.color),
                .selection_color = theme::ThemeColor::literal(style.selection),
                .focus_ring_color = theme::ThemeColor::literal(style.focus.color),
                .focus_ring_width = theme::ThemeScalar::literal(style.focus.width),
                .font_size = theme::ThemeScalar::literal(style.value.font_size),
                .metrics_height = theme::ThemeScalar::literal(style.metrics.height),
                .metrics_padding_x = theme::ThemeScalar::literal(style.metrics.padding_x),
            }
        );
    }

    void Combobox::sync_surface_style() {
        surface_->set_style(resolved_style(), resolved_style_context());
    }

    void Combobox::sync_popover_gap() {
        const float gap = resolved_style().metrics.gap;
        popover_->set_gap(std::isfinite(gap) ? std::max(0.0F, gap) : 0.0F);
    }

    auto Combobox::restore_field_focus() -> bool {
        auto* tree = get_tree();
        if (tree == nullptr || !text_field_->is_inside_tree()) {
            return false;
        }
        tree->set_focus(text_field_.get());
        return tree->focused_node() == text_field_.get();
    }

    void Combobox::handle_field_text_changed(const std::string_view /*text*/) {
        const auto current = text_field_->value();
        (void)sync_selected_from_text();
        notify_change(current, selected_id_);
        refresh_filter();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    void Combobox::activate_id(const std::string_view id) {
        const auto* found = find_menu_item(items_, id);
        if (found == nullptr || !menu_item_is_activatable(*found)) {
            return;
        }
        // 先取副本：用户回调里可能 set_items() 并替换整个容器。
        const std::string label = found->label;
        const std::string stable_id = found->id;

        text_field_->set_value(label); // 静默：事件由下面的 notify_change 统一发出
        selected_id_ = stable_id;
        committed_label_ = label;
        change_emitted_ = false;
        notify_change(label, stable_id);
        item_selected_.emit(stable_id);
        // 选中即关闭；关闭路径不会再提交未匹配文本（此时文本正好等于标签）。
        close();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    void Combobox::handle_closed() {
        surface_->clear_hover();
        commit_unmatched();
        if (on_close_) {
            on_close_();
        }
        mark_semantics_dirty();
    }

    void Combobox::commit_unmatched() {
        if (sync_selected_from_text()) {
            // 文本正好等于某个可激活条目的 label：选中状态已就位，无需收尾。
            return;
        }
        if (allow_custom_value_) {
            // 允许自由文本：不强制匹配，未匹配文本由 on_change 带出（与输入时同一个
            // 值对会被 notify_change 去重）。
            const auto current = text_field_->value();
            if (!current.empty()) {
                notify_change(current, {});
            }
            return;
        }
        restore_committed_text();
    }

    auto Combobox::sync_selected_from_text() -> bool {
        const auto value = text_field_->value();
        if (value.empty()) {
            selected_id_.clear();
            return false;
        }
        for (const auto& item: items_) {
            if (menu_item_is_activatable(item) && item.label == value) {
                selected_id_ = item.id;
                return true;
            }
        }
        selected_id_.clear();
        return false;
    }

    void Combobox::restore_committed_text() {
        if (text_field_->value() == committed_label_) {
            return;
        }
        // 静默回退：这不是用户的编辑，不该再发一次 on_change。
        text_field_->set_value(committed_label_);
        change_emitted_ = false;
        (void)sync_selected_from_text();
        refresh_filter();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    void Combobox::notify_change(const std::string_view text, const std::string_view id) {
        // 输入与失焦 / 关闭收尾走同一条出口：值对相同就不重复通知。
        if (change_emitted_ && last_change_text_ == text && last_change_id_ == id) {
            return;
        }
        last_change_text_ = std::string(text);
        last_change_id_ = std::string(id);
        change_emitted_ = true;
        if (on_change_) {
            on_change_(text, id);
        }
    }
} // namespace nandina::widget
