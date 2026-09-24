//
// widget/dropdown_menu - menu anchored to a trigger, built on Popover + MenuItem.
//
// 本文件定义两个内部类型（不进入公开头文件）：
//   * `internal::MenuItemNode`——单个条目的渲染 + 无障碍节点；
//   * `internal::MenuSurface`——条目列表容器，持有条目模型、键盘漫游与指针命中。
//
// 两者都由 DropdownMenu 私有持有：MenuSurface 是 Popover 的 content，MenuItemNode
// 是 MenuSurface 的子节点。条目节点不参与命中测试（`contains_point` 返回 false）：
// 指针一律由 MenuSurface 按 y 坐标解析到条目索引，这样悬停 / 点击与键盘高亮走同一条
// `active_index` 路径，不会出现两套“当前项”。
//

#include "dropdown_menu.hpp"

#include "key_codes.hpp"
#include "popover.hpp"
#include "primitives/box_painter.hpp"
#include "primitives/text.hpp"
#include "roving_focus.hpp"

#include "../render/draw_context.hpp"
#include "../scene/input_event.hpp"
#include "../scene/overlay_host.hpp"
#include "../scene/scene_tree.hpp"
#include "../theme/theme_manager.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace nandina::widget
{
    namespace
    {
        /// 条目内容几何常量：勾选 / 单选指示器的方形边长与它到文本的间距。
        /// 配方没有这一档标量 token（同 Select 的箭头臂长），属于组件内部几何。
        constexpr float kIndicatorSize = 16.0F;
        constexpr float kIndicatorGap = 8.0F;
        /// submenu 尾部指示（右向 chevron）的臂长。
        constexpr float kArrowArm = 4.0F;

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

        /// 从解析后的排版 + 继承的样式上下文构造文本样式（与 Select 同款）。
        [[nodiscard]] auto make_text_style(
            const theme::ResolvedStyleContext& context,
            const theme::ResolvedTypeStyle& type,
            const text::FontRequest& fallback_font
        ) -> primitives::TextStyle {
            return primitives::TextStyle {
                .color = context.text_color_from_context ? context.text_color : type.color,
                .font_size = context.font_size_from_context ? context.font_size : type.font_size,
                .font = context.font_from_context ? context.font : fallback_font,
                .overflow = primitives::TextOverflow::clip,
                .max_lines = 1,
            };
        }
    } // namespace

    namespace internal
    {
        /**
         * 单个菜单条目的视图与无障碍节点。
         *
         * 只负责“这一行长什么样”与“读屏听到什么”，不处理输入：高亮 / 悬停状态由
         * MenuSurface 统一下发（`set_state`），指针命中由 MenuSurface 按 y 坐标完成。
         */
        class MenuItemNode final: public scene::NanControl {
        public:
            explicit MenuItemNode(const MenuItemKind kind): kind_(kind) {}

            /// 从模型同步展示字段（勾选 / 禁用可以在不重建节点的情况下变化）。
            void sync(const MenuItem& item) {
                kind_ = item.kind;
                disabled_ = item.disabled;
                checked_ = item.checked;
                if (label_ != item.label) {
                    label_ = item.label;
                    label_text_.set_text(label_);
                }
                if (shortcut_ != item.shortcut) {
                    shortcut_ = item.shortcut;
                    shortcut_text_.set_text(shortcut_);
                }
                mark_layout_dirty();
                mark_semantics_dirty();
                mark_dirty(scene::DirtyFlags::paint);
            }

            void set_style(
                theme::ResolvedDropdownMenuStyle style,
                const theme::ResolvedStyleContext& context
            ) {
                style_ = std::move(style);
                context_ = context;
                apply_text_styles();
                mark_layout_dirty();
                mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
            }

            /// 指针悬停 / 键盘高亮是两条独立的状态位：鼠标进入时两者同时为真（按悬停
            /// 配色），键盘导航清掉悬停后只剩高亮（按 focus 配色）。
            void set_state(const bool hovered, const bool highlighted) {
                if (hovered_ == hovered && highlighted_ == highlighted) {
                    return;
                }
                hovered_ = hovered;
                highlighted_ = highlighted;
                mark_dirty(scene::DirtyFlags::paint);
            }

            [[nodiscard]] auto is_focusable() const -> bool override {
                return false;
            }

            /// 命中一律交给 MenuSurface：条目节点只提供几何与语义。
            [[nodiscard]] auto contains_point(foundation::NanPoint) const -> bool override {
                return false;
            }

            [[nodiscard]] auto semantics_properties() const -> semantics::Properties override {
                if (kind_ == MenuItemKind::separator) {
                    return {.role = semantics::Role::separator};
                }
                if (kind_ == MenuItemKind::label) {
                    return {.role = semantics::Role::static_text, .label = label_};
                }

                semantics::Role role = semantics::Role::list_item;
                std::optional<bool> checked;
                if (kind_ == MenuItemKind::checkbox) {
                    role = semantics::Role::checkbox;
                    checked = checked_;
                }
                else if (kind_ == MenuItemKind::radio) {
                    role = semantics::Role::radio;
                    checked = checked_;
                }

                return {
                    .role = role,
                    .label = label_,
                    .hint = shortcut_,
                    .state =
                        {
                            .focusable = false,
                            .focused = highlighted_,
                            .disabled = disabled_,
                            .checked = checked,
                        },
                    .actions = disabled_ ? semantics::Action::none : semantics::Action::activate,
                };
            }

        protected:
            [[nodiscard]] auto on_measure(const scene::LayoutConstraints constraints)
                -> foundation::NanSize override {
                const auto& m = style_.metrics;
                if (kind_ == MenuItemKind::separator) {
                    return constraints.constrain(
                        foundation::NanSize(
                            0.0F,
                            std::max(0.0F, m.separator_thickness) + std::max(0.0F, m.gap) * 2.0F
                        )
                    );
                }

                (void)label_text_.measure_layout(scene::LayoutConstraints::loose());
                float width = leading_width();
                width += label_text_.measured_text_width();
                width += trailing_width();
                if (!shortcut_.empty()) {
                    (void)shortcut_text_.measure_layout(scene::LayoutConstraints::loose());
                    width += m.padding_x + shortcut_text_.measured_text_width();
                }
                return constraints.constrain(foundation::NanSize(width, m.item_height));
            }

            auto on_draw(render::DrawContext& context) -> void override {
                const auto world =
                    render::world_bounds_from_local(context.world_transform(), local_rect());
                const float opacity = context.opacity();
                const auto& m = style_.metrics;

                if (kind_ == MenuItemKind::separator) {
                    const float thickness =
                        context.logical_to_screen(std::max(0.0F, m.separator_thickness));
                    if (thickness > 0.0F) {
                        const auto line = foundation::NanRect::from_xywh(
                            world.get_left(),
                            world.get_top() + (world.get_height() - thickness) * 0.5F,
                            world.get_width(),
                            thickness
                        );
                        context.device().draw_rect(
                            line,
                            style_.separator.with_alpha(style_.separator.alpha() * opacity)
                        );
                    }
                    return;
                }

                // 悬停优先于键盘高亮：鼠标在条目上时按 hover 面绘制。
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

                const float pad_x = context.logical_to_screen(m.padding_x);
                const float center_y = world.get_top() + world.get_height() * 0.5F;
                float x = world.get_left() + pad_x;

                const float indicator =
                    context.logical_to_screen(std::max(0.0F, m.item_height) * 0.5F);
                if (kind_ == MenuItemKind::checkbox || kind_ == MenuItemKind::radio) {
                    const float size =
                        std::min(context.logical_to_screen(kIndicatorSize), indicator);
                    const auto box =
                        foundation::NanRect::from_xywh(x, center_y - size * 0.5F, size, size);
                    if (checked_) {
                        paint_checked_indicator(context, box, opacity);
                    }
                    x += size + context.logical_to_screen(kIndicatorGap);
                }

                const float text_height =
                    context.logical_to_screen(label_text_.measured_text_height());
                label_text_.draw_at(
                    context,
                    foundation::NanPoint(x, center_y - text_height * 0.5F)
                );

                const float arrow_span = kind_ == MenuItemKind::submenu
                    ? context.logical_to_screen(kArrowArm * 2.0F + kIndicatorGap)
                    : 0.0F;

                if (!shortcut_.empty()) {
                    const float shortcut_width =
                        context.logical_to_screen(shortcut_text_.measured_text_width());
                    const float shortcut_height =
                        context.logical_to_screen(shortcut_text_.measured_text_height());
                    shortcut_text_.draw_at(
                        context,
                        foundation::NanPoint(
                            world.get_right() - pad_x - arrow_span - shortcut_width,
                            center_y - shortcut_height * 0.5F
                        )
                    );
                }

                if (kind_ == MenuItemKind::submenu) {
                    paint_submenu_arrow(context, world, pad_x, center_y, opacity);
                }
            }

        private:
            [[nodiscard]] auto leading_width() const -> float {
                if (kind_ != MenuItemKind::checkbox && kind_ != MenuItemKind::radio) {
                    return 0.0F;
                }
                return kIndicatorSize + kIndicatorGap;
            }

            [[nodiscard]] auto trailing_width() const -> float {
                return kind_ == MenuItemKind::submenu ? kArrowArm * 2.0F + kIndicatorGap : 0.0F;
            }

            auto apply_text_styles() -> void {
                auto label_style = kind_ == MenuItemKind::label
                    ? make_text_style(context_, style_.group_label, label_text_.font())
                    : make_text_style(context_, style_.item_label, label_text_.font());
                if (disabled_ && kind_ != MenuItemKind::label) {
                    label_style.color = style_.disabled_label;
                }
                if (!same_text_style(label_text_.style(), label_style)) {
                    label_text_.set_style(label_style);
                }

                const auto shortcut_style =
                    make_text_style(context_, style_.item_shortcut, shortcut_text_.font());
                if (!same_text_style(shortcut_text_.style(), shortcut_style)) {
                    shortcut_text_.set_style(shortcut_style);
                }
            }

            void paint_checked_indicator(
                render::DrawContext& context,
                const foundation::NanRect& box,
                const float opacity
            ) {
                const auto color =
                    style_.checked_indicator.with_alpha(style_.checked_indicator.alpha() * opacity);
                if (kind_ == MenuItemKind::radio) {
                    context.device().draw_circle(box.get_center(), box.get_width() * 0.28F, color);
                    return;
                }
                // 对勾：两段折线，比例取自视觉中心。
                const float thickness = std::max(1.0F, box.get_width() * 0.12F);
                context.device().draw_line(
                    foundation::NanPoint(
                        box.get_left() + box.get_width() * 0.20F,
                        box.get_top() + box.get_height() * 0.52F
                    ),
                    foundation::NanPoint(
                        box.get_left() + box.get_width() * 0.42F,
                        box.get_top() + box.get_height() * 0.74F
                    ),
                    thickness,
                    color
                );
                context.device().draw_line(
                    foundation::NanPoint(
                        box.get_left() + box.get_width() * 0.42F,
                        box.get_top() + box.get_height() * 0.74F
                    ),
                    foundation::NanPoint(
                        box.get_left() + box.get_width() * 0.80F,
                        box.get_top() + box.get_height() * 0.26F
                    ),
                    thickness,
                    color
                );
            }

            void paint_submenu_arrow(
                render::DrawContext& context,
                const foundation::NanRect& world,
                const float pad_x,
                const float center_y,
                const float opacity
            ) {
                const float arm = context.logical_to_screen(kArrowArm);
                const float cx = world.get_right() - pad_x - arm;
                const auto color = style_.item_shortcut.color.with_alpha(
                    style_.item_shortcut.color.alpha() * opacity
                );
                const float thickness = std::max(1.0F, arm * 0.35F);
                context.device().draw_line(
                    foundation::NanPoint(cx - arm * 0.5F, center_y - arm),
                    foundation::NanPoint(cx + arm * 0.5F, center_y),
                    thickness,
                    color
                );
                context.device().draw_line(
                    foundation::NanPoint(cx + arm * 0.5F, center_y),
                    foundation::NanPoint(cx - arm * 0.5F, center_y + arm),
                    thickness,
                    color
                );
            }

            MenuItemKind kind_ = MenuItemKind::action;
            std::string label_;
            std::string shortcut_;
            bool disabled_ = false;
            bool checked_ = false;
            bool hovered_ = false;
            bool highlighted_ = false;
            theme::ResolvedDropdownMenuStyle style_;
            theme::ResolvedStyleContext context_;
            primitives::Text label_text_;
            primitives::Text shortcut_text_;
        };

        /**
         * 菜单表面：Popover 的 content，持有条目模型并消费 `MenuModel` 的规则。
         *
         * 键盘模型 = Select 弹出列表：容器自身可聚焦（打开时 FocusScope 把焦点交给
         * 它），`RovingFocus` 用 selection_only 走位，`active_index()` 就是高亮项。
         * 方向键只移动高亮，不改任何条目的 `checked` —— 勾选是 Enter / Space / 点击
         * 这类显式激活的结果（见 docs/references/menu_model.md）。
         */
        class MenuSurface final: public scene::NanControl {
        public:
            explicit MenuSurface(
                std::function<void(std::string_view)> on_activate,
                std::function<void(std::string_view)> on_hover = {},
                std::function<void()> on_close_level = {}
            ):
                on_activate_(std::move(on_activate)),
                on_hover_(std::move(on_hover)),
                on_close_level_(std::move(on_close_level)) {
                focus_.set_movement(RovingMovement::selection_only);
                focus_.set_orientation(RovingOrientation::vertical);
            }

            // ─── 模型 ────────────────────────────────────────────────────

            void set_items(std::vector<MenuItem> items) {
                items_ = std::move(items);
                rebuild_nodes();
                reset_highlight();
                mark_dirty(
                    scene::DirtyFlags::paint | scene::DirtyFlags::layout
                    | scene::DirtyFlags::semantics
                );
            }

            void set_on_hover(std::function<void(std::string_view)> callback) {
                on_hover_ = std::move(callback);
            }

            void set_on_close_level(std::function<void()> callback) {
                on_close_level_ = std::move(callback);
            }

            [[nodiscard]] auto items() const -> const std::vector<MenuItem>& {
                return items_;
            }

            /// 内部可写入口：DropdownMenu 的勾选变更（MenuSelection）需要原地改条目。
            [[nodiscard]] auto items_mutable() -> std::vector<MenuItem>& {
                return items_;
            }

            [[nodiscard]] auto item_control(const std::string_view id) const
                -> std::shared_ptr<scene::NanControl> {
                for (std::size_t index = 0; index < items_.size(); ++index) {
                    if (items_[index].id == id && index < item_nodes_.size()) {
                        return item_nodes_[index];
                    }
                }
                return nullptr;
            }

            /// 勾选 / 禁用被外部改动后把展示重新同步到节点（不重建节点，不做布局变化）。
            void refresh_item_states() {
                for (std::size_t i = 0; i < item_nodes_.size(); ++i) {
                    item_nodes_[i]->sync(items_[i]);
                }
                mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
            }

            void set_style(
                theme::ResolvedDropdownMenuStyle style,
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

            // ─── 状态 ────────────────────────────────────────────────────

            /// 高亮落到第一个可聚焦条目（打开时调用），并把成员表与当前条目对齐。
            void reset_highlight() {
                sync_roving();
                focus_.set_active_index(first_focusable());
                sync_highlight_states();
            }

            void clear_hover() {
                if (hovered_ < 0) {
                    return;
                }
                hovered_ = -1;
                sync_highlight_states();
            }

            void reset_typeahead() {
                focus_.reset_typeahead();
            }

            void advance_time(const float dt) {
                focus_.advance_time(dt);
            }

            [[nodiscard]] auto active_index() const -> int {
                return focus_.active_index();
            }

            // ─── 输入 ────────────────────────────────────────────────────

            [[nodiscard]] auto is_focusable() const -> bool override {
                return true;
            }

            auto on_input(scene::InputEvent& event) -> bool override {
                switch (event.type()) {
                    case scene::EventType::focus_enter:
                        focused_ = true;
                        mark_semantics_dirty();
                        return false;
                    case scene::EventType::focus_leave:
                        focused_ = false;
                        mark_semantics_dirty();
                        return false;
                    case scene::EventType::mouse_move:
                        return handle_mouse_move(static_cast<scene::MouseMoveEvent&>(event));
                    case scene::EventType::mouse_leave:
                        clear_hover();
                        return false;
                    case scene::EventType::mouse_button:
                        return handle_mouse_button(static_cast<scene::MouseButtonEvent&>(event));
                    case scene::EventType::key:
                        return handle_key(static_cast<scene::KeyEvent&>(event));
                    case scene::EventType::text_input:
                        return handle_text(static_cast<scene::TextInputEvent&>(event));
                    default:
                        return false;
                }
            }

        protected:
            [[nodiscard]] auto on_measure(const scene::LayoutConstraints constraints)
                -> foundation::NanSize override {
                const auto& m = style_.metrics;
                float content_width = 0.0F;
                float content_height = m.padding_y * 2.0F;
                for (auto& node: item_nodes_) {
                    const auto measured = node->measure_layout(scene::LayoutConstraints::loose());
                    content_width = std::max(content_width, measured.get_width());
                    content_height += measured.get_height();
                }
                return constraints.constrain(
                    foundation::NanSize(
                        std::max(m.min_width, content_width + m.padding_x * 2.0F),
                        content_height
                    )
                );
            }

            void on_layout() override {
                const auto& m = style_.metrics;
                const float content_width = std::max(0.0F, width() - m.padding_x * 2.0F);
                float y = m.padding_y;
                for (auto& node: item_nodes_) {
                    const auto measured = node->measure_layout(scene::LayoutConstraints::loose());
                    const float row_height = measured.get_height();
                    node->layout_to(
                        foundation::NanRect::from_xywh(m.padding_x, y, content_width, row_height)
                    );
                    y += row_height;
                }
            }

            [[nodiscard]] auto semantics_properties() const -> semantics::Properties override {
                // 语义角色没有 menu / menuitem（见 semantics::Role）：列表面用 Role::list，
                // 条目节点分别用 list_item / checkbox / radio / separator / static_text。
                return {
                    .role = semantics::Role::list,
                    .label = "menu",
                    .state = {.focusable = true, .focused = focused_},
                    .actions = semantics::Action::focus,
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
                    auto node = std::make_shared<MenuItemNode>(item.kind);
                    node->sync(item);
                    if (style_ready_) {
                        node->set_style(style_, context_);
                    }
                    add_child(node);
                    item_nodes_.push_back(std::move(node));
                }
            }

            void sync_roving() {
                // 规则直接来自 menu_model.md：可聚焦性用 menu_item_is_focusable（disabled
                // 仍可聚焦），typeahead 文本只取 label（shortcut 不参与匹配）。
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
                    // 结构性条目（separator / label）不参与高亮。
                    return false;
                }
                hovered_ = index;
                focus_.set_active_index(index);
                sync_highlight_states();
                if (on_hover_) {
                    on_hover_(items_[target].id);
                }
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
                const auto target = static_cast<std::size_t>(index);
                if (menu_item_is_focusable(items_[target])) {
                    hovered_ = index;
                    focus_.set_active_index(index);
                    sync_highlight_states();
                    if (on_hover_) {
                        on_hover_(items_[target].id);
                    }
                }
                // 结构性条目 / 禁用条目在这里是 no-op（activate_index 会拒绝）。
                activate_index(index);
                event.accept();
                return true;
            }

            auto handle_key(scene::KeyEvent& event) -> bool {
                if (!event.is_pressed()) {
                    return false;
                }
                if (event.keycode() == keys::enter || event.keycode() == keys::space) {
                    const int active = focus_.active_index();
                    if (active < 0) {
                        return false;
                    }
                    activate_index(active);
                    event.accept();
                    return true;
                }
                if (event.keycode() == keys::right) {
                    const int active = focus_.active_index();
                    if (active >= 0
                        && items_[static_cast<std::size_t>(active)].kind == MenuItemKind::submenu)
                    {
                        activate_index(active);
                        event.accept();
                        return true;
                    }
                }
                if (event.keycode() == keys::left && on_close_level_) {
                    on_close_level_();
                    event.accept();
                    return true;
                }
                if (const auto intent = focus_.handle_key(event); intent.has_value()) {
                    // 键盘导航结束指针悬停：此后的高亮用 focus 配色。
                    hovered_ = -1;
                    sync_highlight_states();
                    event.accept();
                    return true;
                }
                return false;
            }

            auto handle_text(scene::TextInputEvent& event) -> bool {
                const auto intent = focus_.handle_text(event);
                if (!intent.has_value()) {
                    return false;
                }
                hovered_ = -1;
                sync_highlight_states();
                event.accept();
                return true;
            }

            std::vector<MenuItem> items_;
            std::vector<std::shared_ptr<MenuItemNode>> item_nodes_;
            RovingFocus focus_;
            int hovered_ = -1;
            bool focused_ = false;
            bool style_ready_ = false;
            theme::ResolvedDropdownMenuStyle style_;
            theme::ResolvedStyleContext context_;
            std::function<void(std::string_view)> on_activate_;
            std::function<void(std::string_view)> on_hover_;
            std::function<void()> on_close_level_;
        };
    } // namespace internal

    DropdownMenu::DropdownMenu(
        std::shared_ptr<scene::NanControl> trigger,
        std::vector<MenuItem> items,
        theme::NanTheme theme
    ) {
        system_ =
            std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;

        // 浮层基座：面板配方由 Popover 自己解析，本组件只负责条目列表。
        popover_ = std::make_shared<Popover>(nullptr, nullptr, theme);
        surface_ = std::make_shared<internal::MenuSurface>([this](const std::string_view id) {
            handle_activate(id);
        });
        surface_->set_on_hover([this](const std::string_view id) { handle_hover(id); });
        surface_->set_on_close_level([this] {
            if (nested_) {
                close();
            }
        });
        // 先给出真实配方再灌条目：条目节点在重建时就能拿到有效的字号 / 度量。
        sync_surface_style();
        surface_->set_items(std::move(items));
        popover_->set_content(surface_);
        if (trigger) {
            popover_->set_trigger(std::move(trigger));
        }
        popover_->set_on_close([this] { handle_closed(); });
        add_child(popover_);
    }

    DropdownMenu::~DropdownMenu() = default;

    auto DropdownMenu::create(
        std::shared_ptr<scene::NanControl> trigger,
        std::vector<MenuItem> items,
        theme::NanTheme theme
    ) -> std::shared_ptr<DropdownMenu> {
        return std::make_shared<DropdownMenu>(std::move(trigger), std::move(items), theme);
    }

    auto DropdownMenu::set_trigger(std::shared_ptr<scene::NanControl> trigger) -> DropdownMenu& {
        popover_->set_trigger(std::move(trigger));
        mark_layout_dirty();
        return *this;
    }

    auto DropdownMenu::trigger() const -> std::shared_ptr<scene::NanControl> {
        return popover_->trigger();
    }

    void DropdownMenu::set_items(std::vector<MenuItem> items) {
        if (submenu_ != nullptr) {
            submenu_->close();
        }
        submenu_parent_id_.clear();
        surface_->set_items(std::move(items));
        // 打开状态下重新呈现：面板必须按新的条目列表重新测量，重建浮层顺带重建
        // FocusScope，避免焦点停在被替换掉的子树上。
        reflow_if_open();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto DropdownMenu::items() const -> const std::vector<MenuItem>& {
        return surface_->items();
    }

    auto DropdownMenu::item_count() const -> std::size_t {
        return surface_->items().size();
    }

    void DropdownMenu::set_selection_mode(const MenuSelectionMode mode) {
        selection_.set_mode(mode);
        if (submenu_ != nullptr) {
            submenu_->set_selection_mode(mode);
        }
    }

    auto DropdownMenu::selection_mode() const -> MenuSelectionMode {
        return selection_.mode();
    }

    auto DropdownMenu::checked_ids() const -> std::vector<std::string> {
        return MenuSelection::checked_ids(surface_->items());
    }

    auto DropdownMenu::set_checked(const std::string_view id, const bool checked) -> bool {
        if (!MenuSelection::set_checked(surface_->items_mutable(), id, checked)) {
            return false;
        }
        surface_->refresh_item_states();
        mark_semantics_dirty();
        return true;
    }

    void DropdownMenu::open() {
        if (popover_->is_open()) {
            return;
        }
        popover_->open();
        // 每次打开都落在第一个可聚焦条目，并清掉上一次的 typeahead 缓冲。
        surface_->reset_typeahead();
        surface_->reset_highlight();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    void DropdownMenu::close() {
        popover_->close();
    }

    void DropdownMenu::toggle() {
        if (popover_->is_open()) {
            close();
        }
        else {
            open();
        }
    }

    auto DropdownMenu::is_open() const -> bool {
        return popover_->is_open();
    }

    auto DropdownMenu::active_index() const -> int {
        return surface_->active_index();
    }

    void DropdownMenu::set_on_select(std::function<void(std::string_view)> callback) {
        on_select_ = std::move(callback);
    }

    auto DropdownMenu::item_selected() const -> const reactive::Event<std::string>& {
        return item_selected_;
    }

    void DropdownMenu::set_on_submenu(std::function<void(std::string_view)> callback) {
        on_submenu_ = std::move(callback);
    }

    void DropdownMenu::set_on_close(std::function<void()> callback) {
        on_close_ = std::move(callback);
    }

    void DropdownMenu::set_placement(const internal::OverlayPlacement placement) {
        popover_->set_placement(placement);
    }

    void DropdownMenu::set_alignment(const internal::OverlayAlignment alignment) {
        popover_->set_alignment(alignment);
    }

    void DropdownMenu::set_gap(const float gap) {
        popover_->set_gap(gap);
    }

    void DropdownMenu::set_theme(theme::NanTheme theme) {
        system_ =
            std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        // 面板与条目同属一个菜单：显式整份主题一并下发给 Popover，避免面板跟随系统而
        // 条目不跟随（或反之）的分裂。
        popover_->set_theme(theme);
        if (submenu_ != nullptr) {
            submenu_->set_theme(theme);
        }
        sync_surface_style();
        mark_layout_dirty();
    }

    auto DropdownMenu::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void DropdownMenu::set_override(theme::DropdownMenuRecipeRule rule) {
        override_ = rule;
        if (submenu_ != nullptr) {
            submenu_->set_override(std::move(rule));
        }
        sync_surface_style();
        mark_dirty(
            scene::DirtyFlags::paint | scene::DirtyFlags::layout | scene::DirtyFlags::semantics
        );
    }

    auto DropdownMenu::resolved_style() const -> theme::ResolvedDropdownMenuStyle {
        auto style = theme::resolve_dropdown_menu(*system_, appearance_);
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void DropdownMenu::on_style_context_changed(const theme::ResolvedStyleContext& context) {
        surface_->set_style(resolved_style(), context);
        if (submenu_ != nullptr) {
            submenu_->on_style_context_changed(context);
            submenu_->popover_->on_style_context_changed(context);
        }
        mark_layout_dirty();
    }

    void DropdownMenu::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        if (submenu_ != nullptr) {
            submenu_->on_theme_changed(manager);
            submenu_->popover_->on_theme_changed(manager);
        }
        sync_surface_style();
        mark_layout_dirty();
    }

    auto DropdownMenu::z_index_hint() const -> int {
        return popover_ != nullptr ? popover_->z_index_hint() : 0;
    }

    auto DropdownMenu::on_input(scene::InputEvent& event) -> bool {
        // 触发控件留在 Popover 子树里，自己处理点击；菜单打开后键盘焦点在 MenuSurface
        // 上，键盘事件不会经过本节点。这里只观察，不消费。
        (void)event;
        return false;
    }

    void DropdownMenu::on_process(const float dt) {
        // Surface 挂在浮层里，但它的 typeahead 时钟统一由本组件推进（同 Select 的做法），
        // 这样即使浮层尚未挂载也能一致地衰减缓冲。
        surface_->advance_time(dt);
        if (submenu_ != nullptr && submenu_->is_open()) {
            submenu_->on_process(dt);
        }
    }

    void DropdownMenu::on_exit_tree() {
        // Popover 自己的 on_exit_tree 会释放浮层；这里只复位本组件的瞬态。
        surface_->reset_typeahead();
        surface_->clear_hover();
        if (submenu_ != nullptr) {
            submenu_->close();
        }
        scene::NanControl::on_exit_tree();
    }

    auto DropdownMenu::on_measure(const scene::LayoutConstraints constraints)
        -> foundation::NanSize {
        return constraints.constrain(popover_->measure_layout(constraints));
    }

    void DropdownMenu::on_layout() {
        popover_->layout_to(local_rect());
    }

    auto DropdownMenu::semantics_properties() const -> semantics::Properties {
        // 与 Popover 同款：非模态容器的状态就是展开 / 收起。条目语义由浮层里的
        // MenuSurface + MenuItemNode 暴露（菜单打开时它们才在场景树里）。
        return {
            .role = semantics::Role::generic,
            .value = popover_->is_open() ? "expanded" : "collapsed",
            .state = {.checked = popover_->is_open()},
        };
    }

    void DropdownMenu::set_overlay_service(scene::OverlayHost* host) noexcept {
        overlay_service_ =
            host != nullptr ? host->weak_self() : std::weak_ptr<scene::OverlayHost> {};
        popover_->set_overlay_service(host);
        if (submenu_ != nullptr) {
            submenu_->set_overlay_service(host);
        }
    }

    void DropdownMenu::set_external_anchor_rect(
        const std::shared_ptr<scene::NanControl>& owner,
        const foundation::NanRect anchor
    ) noexcept {
        popover_->set_external_anchor_rect(owner, anchor);
    }

    void DropdownMenu::handle_hover(const std::string_view id) {
        const auto* item = find_menu_item(surface_->items(), id);
        if (item == nullptr || item->kind != MenuItemKind::submenu || item->children.empty()
            || !menu_item_is_activatable(*item))
        {
            if (submenu_ != nullptr) {
                submenu_->close();
            }
            return;
        }
        if (submenu_ != nullptr && submenu_->is_open() && submenu_parent_id_ == id) {
            return;
        }
        auto anchor = surface_->item_control(id);
        if (anchor == nullptr) {
            return;
        }
        if (submenu_ == nullptr) {
            submenu_ = std::make_shared<DropdownMenu>(nullptr, item->children, theme_view_);
            submenu_->nested_ = true;
            submenu_->parent_menu_ = this;
            // 子菜单组件本身不挂进场景树，无法自动收到 ThemeManager 与样式上下文传播；
            // 直接继承当前层已经解析到的运行时快照，避免自定义组件配方在第二层退回默认值。
            submenu_->system_ = system_;
            submenu_->appearance_ = appearance_;
            submenu_->system_explicit_ = system_explicit_;
            submenu_->theme_view_ = theme_view_;
            submenu_->popover_->inherit_runtime_style_from(*popover_);
            submenu_->on_style_context_changed(resolved_style_context());
            auto overlay = overlay_service_.lock();
            if (overlay == nullptr) {
                overlay = popover_->resolve_overlay_host();
            }
            submenu_->set_overlay_service(overlay.get());
            submenu_->set_selection_mode(selection_.mode());
            submenu_->set_placement(internal::OverlayPlacement::right);
            submenu_->set_alignment(internal::OverlayAlignment::start);
            submenu_->set_gap(4.0F);
            if (override_) {
                submenu_->set_override(*override_);
            }
            submenu_->set_on_select([this](const std::string_view child_id) {
                sync_submenu_items();
                notify_select(child_id);
            });
            submenu_->set_on_submenu([this](const std::string_view child_id) {
                if (on_submenu_) {
                    on_submenu_(child_id);
                }
            });
            submenu_->set_on_close([this] {
                sync_submenu_items();
                submenu_parent_id_.clear();
            });
        }
        else {
            submenu_->close();
            submenu_->set_items(item->children);
        }
        submenu_parent_id_ = std::string(id);
        submenu_->popover_->set_external_anchor(anchor);
        submenu_->popover_->set_pointer_passthrough_outside(true);
        submenu_->open();
    }

    void DropdownMenu::sync_submenu_items() {
        if (submenu_ == nullptr || submenu_parent_id_.empty()) {
            return;
        }
        auto* parent = find_menu_item(surface_->items_mutable(), submenu_parent_id_);
        if (parent != nullptr && parent->kind == MenuItemKind::submenu) {
            parent->children = submenu_->items();
        }
    }

    void DropdownMenu::close_menu_tree() {
        auto* root = this;
        while (root->parent_menu_ != nullptr) {
            root = root->parent_menu_;
        }
        root->close();
    }

    void DropdownMenu::handle_activate(const std::string_view id) {
        auto& items = surface_->items_mutable();
        const MenuItem* item = find_menu_item(items, id);
        if (item == nullptr || !menu_item_is_activatable(*item)) {
            return;
        }

        switch (item->kind) {
            case MenuItemKind::action:
                notify_select(id);
                // 任意深度的动作菜单都关闭整棵菜单。
                close_menu_tree();
                break;
            case MenuItemKind::checkbox:
            case MenuItemKind::radio:
                // toggle 只在 mode 允许且条目可勾选时改变状态；无论是否改变都视为一次
                // 激活（mode == none 时它是纯动作菜单，仍应通知 on_select）。
                (void)selection_.toggle(items, id);
                surface_->refresh_item_states();
                notify_select(id);
                // 勾选 / 单选菜单**保持打开**：用户可以连续改多项，单选的互斥由
                // MenuSelection 在同层 radio 之间完成。
                break;
            case MenuItemKind::submenu:
                handle_hover(id);
                if (on_submenu_) {
                    on_submenu_(id);
                }
                break;
            case MenuItemKind::separator:
            case MenuItemKind::label:
            default:
                break;
        }
        mark_semantics_dirty();
    }

    void DropdownMenu::handle_closed() {
        if (submenu_ != nullptr) {
            submenu_->close();
        }
        surface_->reset_typeahead();
        surface_->clear_hover();
        if (on_close_) {
            on_close_();
        }
        mark_semantics_dirty();
    }

    void DropdownMenu::sync_surface_style() {
        surface_->set_style(resolved_style(), resolved_style_context());
    }

    void DropdownMenu::reflow_if_open() {
        if (!popover_->is_open()) {
            return;
        }
        auto* tree = get_tree();
        if (tree != nullptr && tree->phase() != scene::FramePhase::idle) {
            // 任何帧阶段内的浮层重建都必须排到安全提交点：输入派发期间，整条冒泡路径
            // 被 `_bubble_input` 用 shared_ptr 钉住（防止回调销毁路径上的节点），此时
            // 面板仍挂在旧的 FocusScope 之下，重新 present 会撞上“内容仍有父节点”。
            // 用弱引用排队；节点若已销毁则什么都不做。
            std::weak_ptr<Popover> popover = popover_;
            std::weak_ptr<internal::MenuSurface> surface = surface_;
            tree->defer_tree_mutation([popover, surface] {
                const auto live_popover = popover.lock();
                const auto live_surface = surface.lock();
                if (live_popover != nullptr && live_surface != nullptr && live_popover->is_open()) {
                    live_popover->set_content(live_surface);
                }
            });
            return;
        }
        popover_->set_content(surface_);
    }

    void DropdownMenu::notify_select(const std::string_view id) {
        if (on_select_) {
            on_select_(id);
        }
        item_selected_.emit(std::string(id));
    }
} // namespace nandina::widget
