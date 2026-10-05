//
// widget/command_palette - modal command launcher (Ctrl/Cmd+K) over the MenuItem model.
//
// 本文件定义四个内部类型（不进入公开头文件）：
//   * `internal::CommandPaletteRow`——单个结果行的渲染 + 无障碍节点；
//   * `internal::CommandPaletteResultSurface`——结果列表容器（漫游 / 指针命中 / 空状态）；
//   * `internal::CommandPalettePanel`——面板（查询行 + 结果列表 + 面板底）；
//   * `internal::CommandPaletteShell`——把面板放在视口靠上位置，并声明遮罩的命中范围。
//
// 承载链路与 Dialog 一致：DismissLayer → FocusScope → Shell → Panel。焦点始终留在查询
// 输入框（同 Combobox），结果行不参与命中测试，指针由结果表面按 y 坐标解析到行下标。
//

#include "command_palette.hpp"

#include "internal/text_style_bridge.hpp"

#include "../semantics/semantics.hpp"
#include "internal/dismiss_layer.hpp"
#include "internal/focus_scope.hpp"
#include "key_codes.hpp"
#include "menu_item.hpp"
#include "primitives/box_painter.hpp"
#include "primitives/text.hpp"
#include "roving_focus.hpp"
#include "text_field.hpp"

#include "../render/draw_context.hpp"
#include "../scene/animation_host.hpp"
#include "../scene/input_event.hpp"
#include "../scene/overlay_host.hpp"
#include "../scene/scene_tree.hpp"
#include "../theme/theme_manager.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        using internal::make_text_style;
        /// 从解析后的排版 + 继承的样式上下文构造文本样式（与 Combobox / DropdownMenu 同款）。
        [[nodiscard]] auto lower_ascii(const char value) -> char {
            return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
        }

        /// 大小写不敏感（ASCII）子串匹配。只匹配 label（`menu_model.md` 规则 2：
        /// `shortcut` 是展示提示，把它纳入匹配会让 "c" 同时命中 Copy 与 Ctrl+...）。
        [[nodiscard]] auto
        contains_case_insensitive(const std::string_view haystack, const std::string_view needle)
            -> bool {
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

        /// 勾选指示的占位尺寸与间距（与 DropdownMenu 同值，两个菜单族控件保持一致）。
        constexpr float kIndicatorSize = 16.0F;
        constexpr float kIndicatorGap = 8.0F;

        [[nodiscard]] auto is_structural(const MenuItem& item) -> bool {
            return item.kind == MenuItemKind::label || item.kind == MenuItemKind::separator;
        }

        /// 遮罩与面板共用的淡入淡出行为：进场 ease_out，退场 ease_in（同 Dialog）。
        [[nodiscard]] auto fade_behavior(const theme::DesignSystem& system, const bool entering)
            -> motion::Behavior<float> {
            return motion::Behavior<float>(
                system.tokens.motion.short_duration,
                entering ? motion::Easing::ease_out : motion::Easing::ease_in
            );
        }

        [[nodiscard]] auto scrim_style(const theme::ResolvedCommandPaletteStyle& style)
            -> theme::ResolvedBoxStyle {
            // 遮罩颜色来自配方（`scrim`），不从面板色派生 —— 派生会让亮色外观下出现
            // 一层**白罩**（面板底在亮色下接近白）。默认值是固定黑。
            return theme::ResolvedBoxStyle {
                .fill = style.scrim,
                .border = style.scrim.with_alpha(0.0F),
                .border_width = 0.0F,
                .radius = 0.0F,
            };
        }
    } // namespace

    namespace internal
    {
        /**
         * 单个结果行：只负责"这一行长什么样"与"读屏听到什么"。
         *
         * 悬停 / 高亮由结果表面统一下发，指针命中由表面按 y 坐标完成（同 Combobox 的选项行）。
         * `label` / `separator` 这类结构条目也各占一行，但它们不可聚焦、不可激活。
         */
        class CommandPaletteRow final: public scene::NanControl {
        public:
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
                theme::ResolvedCommandPaletteStyle style,
                const theme::ResolvedStyleContext& context
            ) {
                style_ = std::move(style);
                context_ = context;
                has_style_ = true;
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
                // 高亮会换前景色（highlight_text 才是 accent 上的可读色），必须重算文字样式。
                apply_text_style();
                mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
            }

            [[nodiscard]] auto kind() const -> MenuItemKind {
                return kind_;
            }

            [[nodiscard]] auto is_focusable() const -> bool override {
                return false;
            }

            /// 命中一律交给结果表面：行只提供几何与语义。
            [[nodiscard]] auto contains_point(foundation::NanPoint) const -> bool override {
                return false;
            }

            [[nodiscard]] auto semantics_properties() const -> semantics::Properties override {
                if (kind_ == MenuItemKind::separator) {
                    return {.role = semantics::Role::separator};
                }
                if (kind_ == MenuItemKind::label) {
                    return {
                        .role = semantics::Role::static_text,
                        .label = label_,
                    };
                }
                const auto role = kind_ == MenuItemKind::checkbox ? semantics::Role::checkbox
                    : kind_ == MenuItemKind::radio                ? semantics::Role::radio
                                                                  : semantics::Role::list_item;
                return {
                    .role = role,
                    .label = label_,
                    .value = shortcut_,
                    .state =
                        {
                            .focusable = true,
                            .focused = highlighted_,
                            .disabled = disabled_,
                        },
                    .actions = disabled_ ? semantics::Action::none : semantics::Action::activate,
                };
            }

        protected:
            [[nodiscard]] auto on_measure(const foundation::NanLayoutConstraints constraints)
                -> foundation::NanSize override {
                const auto& m = style_.metrics;
                if (kind_ == MenuItemKind::separator) {
                    return constraints.constrain(
                        foundation::NanSize(constraints.max_width, m.separator_thickness)
                    );
                }
                if (kind_ == MenuItemKind::label) {
                    (void)label_text_.measure_layout(foundation::NanLayoutConstraints::loose());
                    return constraints.constrain(
                        foundation::NanSize(
                            label_text_.measured_text_width() + m.item_padding_x * 2.0F,
                            m.item_height
                        )
                    );
                }
                (void)label_text_.measure_layout(foundation::NanLayoutConstraints::loose());
                if (!shortcut_.empty()) {
                    (void)shortcut_text_.measure_layout(foundation::NanLayoutConstraints::loose());
                }
                return constraints.constrain(
                    foundation::NanSize(constraints.max_width, m.item_height)
                );
            }

            auto on_draw(render::DrawContext& context) -> void override {
                const auto world =
                    render::world_bounds_from_local(context.world_transform(), local_rect());
                const float opacity = context.opacity();
                const auto& m = style_.metrics;

                if (kind_ == MenuItemKind::separator) {
                    primitives::BoxPainter::paint_fill(
                        context,
                        world,
                        theme::ResolvedBoxStyle {
                            .fill = style_.separator,
                            .border = style_.separator.with_alpha(0.0F),
                            .border_width = 0.0F,
                            .radius = 0.0F,
                        },
                        opacity
                    );
                    return;
                }

                if (kind_ != MenuItemKind::label && (hovered_ || highlighted_)) {
                    // 悬停优先于键盘高亮：鼠标在行上时按 hover 面绘制（同 DropdownMenu）。
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

                const float pad_x = context.logical_to_screen(m.item_padding_x);
                const float center_y = world.get_top() + world.get_height() * 0.5F;
                const float text_height =
                    context.logical_to_screen(label_text_.measured_text_height());
                float label_x = world.get_left() + pad_x;

                // 勾选指示的槽位对 checkbox / radio 行**恒定保留**（同 DropdownMenu）：
                // 已勾选与未勾选的标签因此始终左对齐，不会随状态左右跳动。
                if (kind_ == MenuItemKind::checkbox || kind_ == MenuItemKind::radio) {
                    const float indicator =
                        context.logical_to_screen(std::max(0.0F, m.item_height) * 0.5F);
                    const float size =
                        std::min(context.logical_to_screen(kIndicatorSize), indicator);
                    const auto box =
                        foundation::NanRect::from_xywh(label_x, center_y - size * 0.5F, size, size);
                    if (checked_) {
                        paint_checked_indicator(context, box, opacity);
                    }
                    label_x += size + context.logical_to_screen(kIndicatorGap);
                }

                label_text_.draw_at(
                    context,
                    foundation::NanPoint(label_x, center_y - text_height * 0.5F)
                );

                if (kind_ == MenuItemKind::label || shortcut_.empty()) {
                    return;
                }
                const float shortcut_width =
                    context.logical_to_screen(shortcut_text_.measured_text_width());
                const float shortcut_height =
                    context.logical_to_screen(shortcut_text_.measured_text_height());
                shortcut_text_.draw_at(
                    context,
                    foundation::NanPoint(
                        world.get_right() - pad_x - shortcut_width,
                        world.get_top() + (world.get_height() - shortcut_height) * 0.5F
                    )
                );
            }

        private:
            /// 勾选指示的颜色：禁用 > 高亮 > 常规（同标签的前景色规则）。
            [[nodiscard]] auto indicator_color() const -> foundation::NanColor {
                if (disabled_) {
                    return style_.disabled_label;
                }
                return highlighted_ ? style_.highlight_text : style_.checked_indicator;
            }

            /// 对勾 / 圆点用图元绘制而不是字形：不依赖字体里有 `✓`，也就不会在不同
            /// 字体下变成豆腐块（比例取自 DropdownMenu 的同一实现）。
            void paint_checked_indicator(
                render::DrawContext& context,
                const foundation::NanRect& box,
                const float opacity
            ) {
                const auto color =
                    indicator_color().with_alpha(indicator_color().alpha() * opacity);
                if (kind_ == MenuItemKind::radio) {
                    context.device().draw_circle(box.get_center(), box.get_width() * 0.28F, color);
                    return;
                }
                const float thickness = std::max(1.0F, box.get_width() * 0.12F);
                const auto point = [&box](const float fx, const float fy) {
                    return foundation::NanPoint(
                        box.get_left() + box.get_width() * fx,
                        box.get_top() + box.get_height() * fy
                    );
                };
                context.device()
                    .draw_line(point(0.20F, 0.52F), point(0.42F, 0.74F), thickness, color);
                context.device()
                    .draw_line(point(0.42F, 0.74F), point(0.80F, 0.30F), thickness, color);
            }

            void apply_text_style() {
                if (!has_style_) {
                    // 样式还没下发（`set_rows()` 里的行状态刷新可能早于 `set_style()`）。
                    // 此时 style_ 是默认构造的、字号为 0，设上去会撞 Text 的字号校验。
                    return;
                }
                auto label_style = make_text_style(
                    context_,
                    kind_ == MenuItemKind::label ? style_.group_label : style_.item_label,
                    label_text_.font(),
                    text::TextOverflow::ellipsis
                );
                if (disabled_ && kind_ != MenuItemKind::label) {
                    label_style.color = style_.disabled_label;
                }
                else if (highlighted_ && kind_ != MenuItemKind::label) {
                    // 高亮底是 accent，配 accent_foreground 才有对比度保证。
                    label_style.color = style_.highlight_text;
                }
                if (!label_text_.style().approx_equals(label_style)) {
                    label_text_.set_style(label_style);
                }
                if (!shortcut_.empty()) {
                    auto shortcut_style = make_text_style(
                        context_,
                        style_.item_shortcut,
                        shortcut_text_.font(),
                        text::TextOverflow::ellipsis
                    );
                    if (disabled_) {
                        shortcut_style.color = style_.disabled_label;
                    }
                    else if (highlighted_) {
                        shortcut_style.color = style_.highlight_text;
                    }
                    if (!shortcut_text_.style().approx_equals(shortcut_style)) {
                        shortcut_text_.set_style(shortcut_style);
                    }
                }
            }

            MenuItemKind kind_ = MenuItemKind::action;
            std::string label_;
            std::string shortcut_;
            bool disabled_ = false;
            bool checked_ = false;
            bool hovered_ = false;
            bool highlighted_ = false;
            theme::ResolvedCommandPaletteStyle style_;
            theme::ResolvedStyleContext context_;
            bool has_style_ = false;
            primitives::Text label_text_;
            primitives::Text shortcut_text_;
        };

        /**
         * 结果列表容器：持有**副本**（过滤 + 分组保留 + 上限裁剪之后的结果），因此组件
         * 换掉条目模型时这里不会留下悬垂引用。
         *
         * 表面自身不可聚焦：焦点必须留在查询输入框上，否则用户无法继续输入过滤。
         * 漫游不启用 typeahead —— 查询本身就是过滤器，否则输入的字母会既进输入框又跳高亮。
         */
        class CommandPaletteResultSurface final: public scene::NanControl {
        public:
            explicit CommandPaletteResultSurface(std::function<void(std::string_view)> on_activate):
                on_activate_(std::move(on_activate)) {
                // 菜单语义：焦点移动不改值，激活由 Enter / 点击显式触发。
                focus_.set_movement(RovingMovement::focus_only);
                focus_.set_orientation(RovingOrientation::vertical);
            }

            /// 就地替换结果行（不重建浮层，因此不会打断查询与焦点）。
            void
            set_rows(std::vector<MenuItem> rows, const std::size_t hidden, const bool searching) {
                rows_ = std::move(rows);
                hidden_ = hidden;
                searching_ = searching;
                // 可见子节点数多于行数时多出的是待删除的旧行，这里按需补齐 / 裁剪。
                while (child_count() > rows_.size()) {
                    if (auto* last = get_child(child_count() - 1); last != nullptr) {
                        remove_and_delete(*last);
                    }
                }
                while (child_count() < rows_.size()) {
                    add_child(std::make_shared<CommandPaletteRow>());
                }
                for (std::size_t i = 0; i < rows_.size(); ++i) {
                    if (auto* row = dynamic_row(i); row != nullptr) {
                        row->sync(rows_[i]);
                    }
                }
                sync_hint();
                sync_focus();
                mark_layout_dirty();
                mark_semantics_dirty();
                mark_dirty(scene::DirtyFlags::paint);
            }

            void set_style(
                theme::ResolvedCommandPaletteStyle style,
                const theme::ResolvedStyleContext& context
            ) {
                style_ = std::move(style);
                context_ = context;
                for (std::size_t i = 0; i < child_count(); ++i) {
                    if (auto* row = dynamic_row(i); row != nullptr) {
                        row->set_style(style_, context_);
                    }
                }
                if (!hint_text_.style().approx_equals(hint_style())) {
                    hint_text_.set_style(hint_style());
                }
                mark_layout_dirty();
                mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
            }

            void set_disabled(const bool disabled) {
                disabled_ = disabled;
                mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
            }

            [[nodiscard]] auto active_index() const -> int {
                return focus_.active_index();
            }

            [[nodiscard]] auto active_id() const -> std::string_view {
                const int index = focus_.active_index();
                if (index < 0 || static_cast<std::size_t>(index) >= rows_.size()) {
                    return {};
                }
                return rows_[static_cast<std::size_t>(index)].id;
            }

            [[nodiscard]] auto rows() const -> const std::vector<MenuItem>& {
                return rows_;
            }

            [[nodiscard]] auto hidden_count() const -> std::size_t {
                return hidden_;
            }

            /// 让高亮落在首 / 末个可聚焦条目。空结果时为 -1。
            void set_active_edge(const bool from_end) {
                if (rows_.empty()) {
                    focus_.set_active_index(-1);
                    refresh_row_states();
                    return;
                }
                focus_.set_active_index(from_end ? static_cast<int>(rows_.size()) - 1 : 0);
                // set_active_index 不做可聚焦性校验，这里自己纠正到最近的可聚焦条目。
                if (active_index() >= 0 && !is_focusable_at(active_index())) {
                    const int step = from_end ? -1 : 1;
                    for (int i = active_index();
                         i >= 0 && static_cast<std::size_t>(i) < rows_.size();
                         i += step)
                    {
                        if (is_focusable_at(i)) {
                            focus_.set_active_index(i);
                            break;
                        }
                    }
                }
                refresh_row_states();
            }

            void set_active_index(const int index) {
                focus_.set_active_index(index);
                refresh_row_states();
            }

            /// 高亮沿列表移动；返回 false 表示这个键不归漫游管（Enter / Escape 等）。
            auto handle_navigation_key(const scene::KeyEvent& event) -> bool {
                const auto intent = focus_.handle_key(event);
                if (!intent.has_value()) {
                    return false;
                }
                refresh_row_states();
                return true;
            }

            auto on_input(scene::InputEvent& event) -> bool override {
                if (event.type() != scene::EventType::mouse_button) {
                    return false;
                }
                auto& pointer = static_cast<scene::MouseButtonEvent&>(event);
                const auto local = to_local(pointer.screen_pos());
                if (!contains_point(local)) {
                    return false;
                }
                const int index = row_index_at(local.get_y());
                if (index < 0) {
                    return false;
                }
                if (pointer.is_pressed()
                    && pointer.button() == scene::MouseButtonEvent::Button::left)
                {
                    hovered_ = index;
                    focus_.set_active_index(index);
                    refresh_row_states();
                    if (disabled_ || !is_activatable_at(index)) {
                        event.accept();
                        return true;
                    }
                    if (on_activate_) {
                        on_activate_(rows_[static_cast<std::size_t>(index)].id);
                    }
                    event.accept();
                    return true;
                }
                if (!pointer.is_pressed()) {
                    // 释放事件不改变高亮；它由按下时的那一次决定。
                    return true;
                }
                if (hovered_ != index) {
                    hovered_ = index;
                    focus_.set_active_index(index);
                    refresh_row_states();
                }
                event.accept();
                return true;
            }

            auto on_draw(render::DrawContext& context) -> void override {
                if (hint_.empty()) {
                    return;
                }
                const auto world =
                    render::world_bounds_from_local(context.world_transform(), local_rect());
                const float reserved = context.logical_to_screen(extra_height());
                const float pad_x = context.logical_to_screen(style_.metrics.item_padding_x);
                const float text_height =
                    context.logical_to_screen(hint_text_.measured_text_height());
                hint_text_.draw_at(
                    context,
                    foundation::NanPoint(
                        world.get_left() + pad_x,
                        world.get_bottom() - reserved + (reserved - text_height) * 0.5F
                    )
                );
            }

            [[nodiscard]] auto semantics_properties() const -> semantics::Properties override {
                return {
                    .role = semantics::Role::list,
                    .label = rows_.empty() ? std::string {} : std::string {"命令结果"},
                    .state = {.focusable = false, .disabled = disabled_},
                };
            }

        protected:
            [[nodiscard]] auto on_measure(const foundation::NanLayoutConstraints constraints)
                -> foundation::NanSize override {
                float height = 0.0F;
                float width = 0.0F;
                for (std::size_t i = 0; i < child_count(); ++i) {
                    auto* row = dynamic_row(i);
                    if (row == nullptr) {
                        continue;
                    }
                    const auto measured = row->measure_layout(
                        foundation::NanLayoutConstraints {
                            .min_width = 0.0F,
                            .max_width = constraints.max_width,
                            .min_height = 0.0F,
                            .max_height = constraints.max_height,
                        }
                    );
                    height += measured.get_height();
                    width = std::max(width, measured.get_width());
                }
                const auto extra = extra_height();
                if (extra > 0.0F) {
                    height += extra;
                }
                return constraints.constrain(foundation::NanSize(width, height));
            }

            void on_layout() override {
                float y = 0.0F;
                for (std::size_t i = 0; i < child_count(); ++i) {
                    auto* row = dynamic_row(i);
                    if (row == nullptr) {
                        continue;
                    }
                    const auto measured = row->measured_size();
                    row->measure_layout(
                        foundation::NanLayoutConstraints::tight(
                            foundation::NanSize(width(), measured.get_height())
                        )
                    );
                    row->layout_to(
                        foundation::NanRect::from_xywh(0.0F, y, width(), measured.get_height())
                    );
                    y += measured.get_height();
                }
                if (!hint_.empty()) {
                    (void)hint_text_.measure_layout(foundation::NanLayoutConstraints::loose());
                }
            }

        private:
            [[nodiscard]] auto dynamic_row(const std::size_t index) const -> CommandPaletteRow* {
                auto* node = get_child(index);
                return node != nullptr ? static_cast<CommandPaletteRow*>(node->as_control())
                                       : nullptr;
            }

            [[nodiscard]] auto is_focusable_at(const int index) const -> bool {
                if (index < 0 || static_cast<std::size_t>(index) >= rows_.size()) {
                    return false;
                }
                return menu_item_is_focusable(rows_[static_cast<std::size_t>(index)]);
            }

            [[nodiscard]] auto is_activatable_at(const int index) const -> bool {
                if (index < 0 || static_cast<std::size_t>(index) >= rows_.size()) {
                    return false;
                }
                return menu_item_is_activatable(rows_[static_cast<std::size_t>(index)]);
            }

            /// 指针 y → 行下标；落在结构条目或空白上时返回 -1。
            [[nodiscard]] auto row_index_at(const float local_y) const -> int {
                float y = 0.0F;
                for (std::size_t i = 0; i < child_count(); ++i) {
                    auto* row = dynamic_row(i);
                    if (row == nullptr) {
                        continue;
                    }
                    const float next = y + row->size().get_height();
                    if (local_y >= y && local_y < next) {
                        return is_focusable_at(static_cast<int>(i)) ? static_cast<int>(i) : -1;
                    }
                    y = next;
                }
                return -1;
            }

            /// 空结果 / 上限提示文本。测量与绘制都用它，避免两处各算一遍。
            void sync_hint() {
                std::string text;
                if (rows_.empty()) {
                    text = searching_ ? "没有匹配的命令" : "输入以搜索命令";
                }
                else if (hidden_ > 0) {
                    // 不假装能滚动：如实告诉用户还有多少条，以及怎么继续缩小范围。
                    text = "还有 " + std::to_string(hidden_) + " 条，继续输入以缩小范围";
                }
                if (hint_ == text) {
                    return;
                }
                hint_ = std::move(text);
                hint_text_.set_text(hint_);
                if (!hint_.empty() && !hint_text_.style().approx_equals(hint_style())) {
                    hint_text_.set_style(hint_style());
                }
            }

            void sync_focus() {
                // typeahead 一律返回空串：查询本身就是过滤器（见文件头说明）。
                focus_.sync(
                    rows_.size(),
                    [this](const std::size_t i) { return menu_item_is_focusable(rows_[i]); },
                    [](std::size_t) -> std::string_view { return {}; }
                );
                if (active_index() < 0 && !rows_.empty()) {
                    for (std::size_t i = 0; i < rows_.size(); ++i) {
                        if (menu_item_is_focusable(rows_[i])) {
                            focus_.set_active_index(static_cast<int>(i));
                            break;
                        }
                    }
                }
                refresh_row_states();
            }

            void refresh_row_states() {
                const int active = focus_.active_index();
                for (std::size_t i = 0; i < child_count(); ++i) {
                    auto* row = dynamic_row(i);
                    if (row == nullptr) {
                        continue;
                    }
                    row->set_state(static_cast<int>(i) == hovered_, static_cast<int>(i) == active);
                }
                mark_semantics_dirty();
                mark_dirty(scene::DirtyFlags::paint);
            }

            [[nodiscard]] auto hint_style() const -> text::TextStyle {
                return make_text_style(
                    context_,
                    style_.empty,
                    hint_text_.font(),
                    text::TextOverflow::ellipsis
                );
            }

            /// 空结果提示 / "还有 N 条" 提示占用的高度（都是单行）。
            [[nodiscard]] auto extra_height() const -> float {
                if (rows_.empty()) {
                    return style_.metrics.item_height;
                }
                if (hidden_ > 0) {
                    return style_.metrics.item_height;
                }
                return 0.0F;
            }

            std::function<void(std::string_view)> on_activate_;
            std::vector<MenuItem> rows_;
            std::size_t hidden_ = 0;
            bool searching_ = false;
            bool disabled_ = false;
            int hovered_ = -1;
            RovingFocus focus_;
            std::string hint_;
            primitives::Text hint_text_;
            theme::ResolvedCommandPaletteStyle style_;
            theme::ResolvedStyleContext context_;
        };

        /**
         * 面板：面板底 + 查询行 + 结果列表。
         *
         * 面板底由本控件绘制（不像 Combobox 那样借 Popover 的外壳），因此 `panel` 造型与
         * `metrics.panel_*` 都落在这里。查询行是内嵌的 TextField，它的样式由 CommandPalette
         * 转成实例覆盖下发，这里只负责摆放。
         */
        class CommandPalettePanel final: public scene::NanControl {
        public:
            CommandPalettePanel(
                std::shared_ptr<TextField> field,
                std::shared_ptr<CommandPaletteResultSurface> surface
            ):
                field_(std::move(field)),
                surface_(std::move(surface)) {
                add_child(field_);
                add_child(surface_);
            }

            void set_style(theme::ResolvedCommandPaletteStyle style) {
                style_ = std::move(style);
                mark_layout_dirty();
                mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
            }

            [[nodiscard]] auto resolved_style() const -> const theme::ResolvedCommandPaletteStyle& {
                return style_;
            }

            [[nodiscard]] auto semantics_properties() const -> semantics::Properties override {
                return {
                    .role = semantics::Role::dialog,
                    .label = std::string {"命令面板"},
                };
            }

        protected:
            [[nodiscard]] auto on_measure(const foundation::NanLayoutConstraints constraints)
                -> foundation::NanSize override {
                const auto& m = style_.metrics;
                const float inner_width = std::max(
                    0.0F,
                    std::min(m.panel_width, constraints.max_width) - m.panel_padding * 2.0F
                );
                const auto query = field_->measure_layout(
                    foundation::NanLayoutConstraints {
                        .min_width = 0.0F,
                        .max_width = inner_width,
                        .min_height = 0.0F,
                        .max_height = constraints.max_height,
                    }
                );
                const auto list = surface_->measure_layout(
                    foundation::NanLayoutConstraints {
                        .min_width = 0.0F,
                        .max_width = inner_width,
                        .min_height = 0.0F,
                        .max_height = constraints.max_height,
                    }
                );
                const float height =
                    m.panel_padding * 2.0F + query.get_height() + m.gap + list.get_height();
                return constraints.constrain(
                    foundation::NanSize(m.panel_width + m.panel_padding * 2.0F, height)
                );
            }

            void on_layout() override {
                const auto& m = style_.metrics;
                const float inner_width = std::max(0.0F, width() - m.panel_padding * 2.0F);
                float y = m.panel_padding;
                const auto query_height = field_->measured_size().get_height();
                field_->measure_layout(
                    foundation::NanLayoutConstraints::tight(foundation::NanSize(inner_width, query_height))
                );
                field_->layout_to(
                    foundation::NanRect::from_xywh(m.panel_padding, y, inner_width, query_height)
                );
                y += query_height + m.gap;
                const float list_height = std::max(0.0F, height() - m.panel_padding - y);
                surface_->measure_layout(
                    foundation::NanLayoutConstraints::tight(foundation::NanSize(inner_width, list_height))
                );
                surface_->layout_to(
                    foundation::NanRect::from_xywh(m.panel_padding, y, inner_width, list_height)
                );
            }

            auto on_draw(render::DrawContext& context) -> void override {
                const auto world =
                    render::world_bounds_from_local(context.world_transform(), local_rect());
                primitives::BoxPainter::paint(context, world, style_.panel, context.opacity());
            }

        private:
            std::shared_ptr<TextField> field_;
            std::shared_ptr<CommandPaletteResultSurface> surface_;
            theme::ResolvedCommandPaletteStyle style_;
        };

        /**
         * 外壳：把面板放在视口靠上的位置，并把面板矩形交给遮罩作为命中范围。
         *
         * 为什么绕这一层：DismissLayer 只提供"居中"或"保持内容自身位置"两种摆放。命令
         * 面板需要"水平居中 + 距顶固定偏移"，于是让外壳测量成**整层高度**、宽度等于面板
         * 宽度 —— 居中在纵向就成了空操作，面板由外壳自己按 `panel_top_offset` 放在顶部。
         *
         * 命中范围必须显式给出：外壳是整层高、只有面板宽，若交给 DismissLayer 用
         * `global_bounds()` 判断，面板下方那条空白也会被当成"面板内部"，点它不会关闭。
         */
        class CommandPaletteShell final: public scene::NanControl {
        public:
            /// 捕获阶段的按键入口。面板以浮层承载时，**组件节点不在焦点链上**（焦点在
            /// 浮层里的查询框上），所以组件自己的 `on_input_capture` 收不到按键；由外壳
            /// ——它在浮层里、又是查询框的祖先——把按键转回组件。
            ///
            /// 树内回退时组件与外壳都在同一条链上，组件先收到并 `accept()`，外壳不会再
            /// 拿到同一个按键，因此不会重复处理。
            std::function<bool(scene::InputEvent&)> on_key;

            auto on_input_capture(scene::InputEvent& event) -> bool override {
                return on_key ? on_key(event) : false;
            }

            void set_panel(std::shared_ptr<CommandPalettePanel> panel) {
                panel_ = std::move(panel);
                add_child(panel_);
            }

            void set_dismiss_layer(DismissLayer* layer) {
                dismiss_layer_ = layer;
            }

            void set_top_offset(const float offset) {
                top_offset_ = std::max(0.0F, offset);
                mark_layout_dirty();
            }

            [[nodiscard]] auto panel() const -> CommandPalettePanel* {
                return panel_.get();
            }

            [[nodiscard]] auto is_focusable() const -> bool override {
                return false;
            }

        protected:
            [[nodiscard]] auto on_measure(const foundation::NanLayoutConstraints constraints)
                -> foundation::NanSize override {
                const auto measured = panel_->measure_layout(constraints);
                // 高度吃掉整层：居中因此只在水平方向生效（见类注释）。
                return constraints.constrain(
                    foundation::NanSize(measured.get_width(), constraints.max_height)
                );
            }

            void on_layout() override {
                const auto measured = panel_->measured_size();
                const float panel_width = std::min(measured.get_width(), width());
                const float panel_height = measured.get_height();
                panel_->measure_layout(
                    foundation::NanLayoutConstraints::tight(foundation::NanSize(panel_width, panel_height))
                );
                // 水平居中（外壳宽度 = 面板宽度，理论上 offset 为 0；保留计算以防约束收窄）。
                const float x = std::max(0.0F, (width() - panel_width) * 0.5F);
                panel_->layout_to(
                    foundation::NanRect::from_xywh(x, top_offset_, panel_width, panel_height)
                );
                if (dismiss_layer_ != nullptr) {
                    dismiss_layer_->set_hit_bounds(panel_->global_bounds());
                }
            }

        private:
            std::shared_ptr<CommandPalettePanel> panel_;
            DismissLayer* dismiss_layer_ = nullptr;
            float top_offset_ = 0.0F;
        };
    } // namespace internal

    using internal::CommandPalettePanel;
    using internal::CommandPaletteResultSurface;
    using internal::CommandPaletteRow;
    using internal::CommandPaletteShell;

    CommandPalette::CommandPalette(
        std::vector<MenuItem> items,
        std::string placeholder,
        theme::NanTheme theme
    ):
        items_(std::move(items)) {
        system_ =
            std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;

        text_field_ = std::make_shared<TextField>(std::string {}, std::move(placeholder), theme);
        text_field_->set_semantics_composition(semantics::Composition::expose);
        text_field_->set_on_change([this](const std::string_view text) {
            handle_query_changed(text);
        });

        surface_ = std::make_shared<CommandPaletteResultSurface>([this](const std::string_view id) {
            activate_id(id);
        });
        panel_ = std::make_shared<CommandPalettePanel>(text_field_, surface_);
        shell_ = std::make_shared<CommandPaletteShell>();
        shell_->on_key = [this](scene::InputEvent& event) { return on_input_capture(event); };
        shell_->set_panel(panel_);

        focus_scope_ = std::make_shared<internal::FocusScope>();
        focus_scope_->set_content(shell_);
        dismiss_layer_ = std::make_shared<internal::DismissLayer>();
        dismiss_layer_->set_content_centered(true);
        dismiss_layer_->set_content(focus_scope_);
        dismiss_layer_->set_visible(false);
        shell_->set_dismiss_layer(dismiss_layer_.get());

        sync_field_style();
        apply_style();
        refresh_results();
        set_visible(false);
    }

    CommandPalette::~CommandPalette() = default;

    auto CommandPalette::create(
        std::vector<MenuItem> items,
        std::string placeholder,
        theme::NanTheme theme
    ) -> std::shared_ptr<CommandPalette> {
        return std::make_shared<CommandPalette>(std::move(items), std::move(placeholder), theme);
    }

    // ─── 条目 ────────────────────────────────────────────────────────────

    void CommandPalette::set_items(std::vector<MenuItem> items) {
        items_ = std::move(items);
        refresh_results();
    }

    auto CommandPalette::items() const -> const std::vector<MenuItem>& {
        return items_;
    }

    auto CommandPalette::item_count() const -> std::size_t {
        return items_.size();
    }

    auto CommandPalette::filtered_ids() const -> std::vector<std::string> {
        std::vector<std::string> ids;
        for (const auto& item: results_) {
            if (menu_item_is_focusable(item)) {
                ids.push_back(item.id);
            }
        }
        return ids;
    }

    void CommandPalette::set_selection_mode(const MenuSelectionMode mode) {
        selection_mode_ = mode;
    }

    auto CommandPalette::selection_mode() const -> MenuSelectionMode {
        return selection_mode_;
    }

    auto CommandPalette::checked_ids() const -> std::vector<std::string> {
        std::vector<std::string> ids;
        for (const auto& item: items_) {
            if (item.checked
                && (item.kind == MenuItemKind::checkbox || item.kind == MenuItemKind::radio))
            {
                ids.push_back(item.id);
            }
        }
        return ids;
    }

    // ─── 查询 ────────────────────────────────────────────────────────────

    void CommandPalette::set_query(std::string query) {
        if (text_field_->value() == query) {
            return;
        }
        text_field_->set_value(std::move(query));
        refresh_results();
    }

    auto CommandPalette::query() const -> std::string_view {
        return text_field_->value();
    }

    void CommandPalette::clear_query() {
        set_query({});
    }

    void CommandPalette::set_placeholder(std::string placeholder) {
        text_field_->set_placeholder(std::move(placeholder));
        mark_layout_dirty();
    }

    auto CommandPalette::placeholder() const -> std::string_view {
        return text_field_->placeholder();
    }

    // ─── 结果上限 ────────────────────────────────────────────────────────

    void CommandPalette::set_max_visible_results(const std::size_t count) {
        max_visible_results_ = std::max<std::size_t>(1, count);
        refresh_results();
    }

    auto CommandPalette::max_visible_results() const -> std::size_t {
        return max_visible_results_;
    }

    auto CommandPalette::hidden_result_count() const -> std::size_t {
        return hidden_results_;
    }

    // ─── 开关与高亮 ──────────────────────────────────────────────────────

    void CommandPalette::open() {
        if (disabled_ || phase_ == MountPhase::opening || phase_ == MountPhase::opened) {
            return;
        }
        if (mount_mode_ == MountMode::unmounted) {
            mount_mode_ = resolve_overlay_host() != nullptr ? MountMode::overlay : MountMode::tree;
        }
        // 每次打开都从空查询开始：命令面板是"随手唤起"的浮层，保留上一次的输入会让
        // 第二次打开看到的是一份被过滤过的列表。
        clear_query();
        surface_->set_active_edge(false);

        // 可见性先于挂载确定（同 Dialog）：树内回退要求自身可见，FocusScope 入树时才能
        // 收集到可聚焦控件；浮层承载时本节点只是页面里的锚点，必须保持不可见。
        set_visible(mount_mode_ == MountMode::tree);
        dismiss_layer_->set_visible(true);
        if (!mount()) {
            dismiss_layer_->set_visible(false);
            set_visible(false);
            return;
        }
        phase_ = MountPhase::opening;
        apply_style();

        // 焦点交给查询框：这是面板唯一接收文本的入口。
        focus_restore_pending_ = true;
        if (auto* tree = get_tree(); tree != nullptr) {
            (void)restore_query_focus();
        }

        auto& fade = dismiss_layer_->fade();
        fade.clear_behavior();
        fade.set_target(0.0F);
        fade.set_behavior(fade_behavior(*system_, true));
        start_fade(1.0F);
        mark_dirty(
            scene::DirtyFlags::paint | scene::DirtyFlags::layout | scene::DirtyFlags::semantics
        );
    }

    void CommandPalette::close() {
        if (!active() || phase_ == MountPhase::closing) {
            return;
        }
        phase_ = MountPhase::closing;
        dismiss_layer_->fade().set_behavior(fade_behavior(*system_, false));
        start_fade(0.0F);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    void CommandPalette::toggle() {
        is_open() ? close() : open();
    }

    auto CommandPalette::is_open() const -> bool {
        return phase_ == MountPhase::opening || phase_ == MountPhase::opened;
    }

    auto CommandPalette::active_index() const -> int {
        return surface_->active_index();
    }

    auto CommandPalette::active_id() const -> std::string_view {
        return surface_->active_id();
    }

    // ─── 回调 / 事件 ─────────────────────────────────────────────────────

    void CommandPalette::set_on_select(std::function<void(std::string_view)> callback) {
        on_select_ = std::move(callback);
    }

    auto CommandPalette::item_selected() const -> const reactive::Event<std::string>& {
        return item_selected_;
    }

    void CommandPalette::set_on_query_change(std::function<void(std::string_view)> callback) {
        on_query_change_ = std::move(callback);
    }

    void CommandPalette::set_on_close(std::function<void()> callback) {
        on_close_ = std::move(callback);
    }

    void CommandPalette::set_disabled(const bool disabled) {
        disabled_ = disabled;
        text_field_->set_disabled(disabled);
        surface_->set_disabled(disabled);
        if (disabled && active()) {
            close();
        }
        mark_semantics_dirty();
    }

    auto CommandPalette::disabled() const -> bool {
        return disabled_;
    }

    // ─── 主题 ────────────────────────────────────────────────────────────

    void CommandPalette::set_theme(theme::NanTheme theme) {
        theme_view_ = theme;
        system_ =
            std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        sync_field_style();
        apply_style();
        mark_layout_dirty();
    }

    auto CommandPalette::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void CommandPalette::set_override(theme::CommandPaletteRecipeRule rule) {
        override_ = std::move(rule);
        sync_field_style();
        apply_style();
        mark_dirty(
            scene::DirtyFlags::paint | scene::DirtyFlags::layout | scene::DirtyFlags::semantics
        );
    }

    auto CommandPalette::resolved_style() const -> theme::ResolvedCommandPaletteStyle {
        auto style = theme::resolve_command_palette(*system_, appearance_);
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void CommandPalette::on_style_context_changed(const theme::ResolvedStyleContext& context) {
        sync_field_style();
        apply_style();
        surface_->set_style(resolved_style(), context);
        mark_layout_dirty();
    }

    void CommandPalette::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        sync_field_style();
        apply_style();
        mark_layout_dirty();
    }

    // ─── 生命周期与输入 ──────────────────────────────────────────────────

    auto CommandPalette::z_index_hint() const -> int {
        // 浮层承载时层级由 OverlayLevel 决定；树内回退要靠 z 序压过后续兄弟。
        return active() && mount_mode_ != MountMode::overlay ? 1 : 0;
    }

    auto CommandPalette::on_measure(const foundation::NanLayoutConstraints constraints)
        -> foundation::NanSize {
        // 浮层承载时本节点只是页面里的锚点，不占位；树内回退时铺满父容器作为遮罩范围。
        if (!active() || mount_mode_ == MountMode::overlay) {
            return constraints.constrain(foundation::NanSize {0.0F, 0.0F});
        }
        return constraints.constrain(
            foundation::NanSize(constraints.max_width, constraints.max_height)
        );
    }

    void CommandPalette::on_layout() {
        if (!active() || dismiss_layer_->parent() != this) {
            return;
        }
        dismiss_layer_->layout_to(local_rect());
    }

    auto CommandPalette::on_input_capture(scene::InputEvent& event) -> bool {
        if (!active() || event.type() != scene::EventType::key) {
            return false;
        }
        auto& key = static_cast<scene::KeyEvent&>(event);
        if (!key.is_pressed()) {
            return false;
        }
        const int code = key.keycode();

        // Escape 归 DismissLayer（它在浮层里包着面板，先收到捕获事件），这里不重复处理。
        if (code == keys::enter) {
            if (const auto id = surface_->active_id(); !id.empty()) {
                activate_id(id);
            }
            event.accept();
            return true;
        }
        if (code == keys::up || code == keys::down || code == keys::home || code == keys::end
            || code == keys::page_up || code == keys::page_down)
        {
            if (surface_->handle_navigation_key(key)) {
                event.accept();
                return true;
            }
        }
        // 其余（可打印字符、Backspace、左右键、Tab、空格…）一律交还 TextField / FocusScope。
        return false;
    }

    void CommandPalette::on_process(const float /*dt*/) {
        if (focus_restore_pending_ && active()) {
            (void)restore_query_focus();
        }
        if (!active()) {
            return;
        }
        auto& fade = dismiss_layer_->fade();
        if (phase_ == MountPhase::opening && !fade.is_animating()) {
            phase_ = MountPhase::opened;
        }
        else if (phase_ == MountPhase::closing && !fade.is_animating()) {
            phase_ = MountPhase::closed;
            dismiss_layer_->set_visible(false);
            unmount();
            set_visible(false);
            if (on_close_) {
                on_close_();
            }
            mark_dirty(
                scene::DirtyFlags::paint | scene::DirtyFlags::layout | scene::DirtyFlags::semantics
            );
        }
    }

    void CommandPalette::on_exit_tree() {
        phase_ = MountPhase::closed;
        unmount();
        set_visible(false);
        scene::NanControl::on_exit_tree();
    }

    auto CommandPalette::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::generic,
            .label = std::string {"命令面板"},
            .state = {.focusable = false, .disabled = disabled_},
        };
    }

    auto CommandPalette::on_semantics_action(const semantics::ActionRequest& request) -> bool {
        if (disabled_ || request.action != semantics::Action::activate) {
            return false;
        }
        if (const auto id = surface_->active_id(); !id.empty()) {
            activate_id(id);
            return true;
        }
        return false;
    }

    // ─── 内部 ────────────────────────────────────────────────────────────

    void CommandPalette::set_overlay_service(scene::OverlayHost* host) noexcept {
        overlay_service_ =
            host != nullptr ? host->weak_self() : std::weak_ptr<scene::OverlayHost> {};
    }

    auto CommandPalette::resolve_overlay_host() -> std::shared_ptr<scene::OverlayHost> {
        if (auto injected = overlay_service_.lock()) {
            return injected;
        }
        for (auto* node = parent(); node != nullptr; node = node->parent()) {
            auto* node_2d = node->as_node2d();
            auto* stack = node_2d != nullptr ? node_2d->as_layer_stack() : nullptr;
            if (stack == nullptr) {
                continue;
            }
            if (auto* host = stack->as_overlay_host(); host != nullptr) {
                return host->weak_self().lock();
            }
        }
        return nullptr;
    }

    auto CommandPalette::mount() -> bool {
        if (mount_mode_ == MountMode::overlay) {
            if (portal_handle_ != nullptr) {
                return true;
            }
            auto host = resolve_overlay_host();
            if (host == nullptr) {
                return false;
            }
            install_dismiss_callback();
            portal_handle_ = std::make_unique<scene::OverlayHandle>(host->present(
                dismiss_layer_,
                scene::OverlayOptions {
                    .level = scene::OverlayLevel::modal,
                    .block_below = true,
                }
            ));
            return true;
        }

        // detached 回退：面板留在树内，靠 z 序与铺满父容器维持模态语义。
        if (dismiss_layer_->parent() == this) {
            return true;
        }
        install_dismiss_callback();
        add_child(dismiss_layer_);
        mark_layout_dirty();
        return true;
    }

    void CommandPalette::unmount() {
        if (portal_handle_ != nullptr) {
            portal_handle_->close();
            portal_handle_.reset();
        }
    }

    void CommandPalette::install_dismiss_callback() {
        // 关闭请求回到本面板；弱引用保证浮层比面板活得久时不会悬空。
        auto weak = std::weak_ptr<CommandPalette>(
            std::static_pointer_cast<CommandPalette>(shared_from_this())
        );
        dismiss_layer_->set_callback([weak](const internal::DismissReason) {
            if (auto palette = weak.lock(); palette != nullptr && !palette->disabled_) {
                palette->close();
            }
        });
    }

    auto CommandPalette::restore_query_focus() -> bool {
        auto* tree = get_tree();
        if (tree == nullptr) {
            return false;
        }
        // FocusScope 在托管时会接管初始焦点，这里把焦点明确交回查询框。
        auto* focused = tree->focused_node();
        if (focused == text_field_.get()) {
            focus_restore_pending_ = false;
            return true;
        }
        if (text_field_->get_tree() == nullptr) {
            return false;
        }
        tree->set_focus(text_field_.get());
        if (tree->focused_node() != text_field_.get()) {
            return false;
        }
        focus_restore_pending_ = false;
        return true;
    }

    void CommandPalette::start_fade(const float target) {
        if (auto* tree = dismiss_layer_->get_tree(); tree != nullptr) {
            tree->animation_host().set_target(
                *dismiss_layer_,
                dismiss_layer_->fade(),
                target,
                scene::DirtyFlags::paint
            );
            return;
        }
        dismiss_layer_->fade().clear_behavior();
        dismiss_layer_->fade().set_target(target);
    }

    void CommandPalette::refresh_results() {
        results_.clear();
        hidden_results_ = 0;
        const std::string_view needle = text_field_->value();

        // 分组保留：`label` 开一组、`separator` 结束一组。整组都无匹配时，该组连同它的
        // 标题与分隔线一起消失 —— 否则过滤之后会剩下孤零零的组标题。
        std::vector<MenuItem> group; // 含组标题与组内条目
        std::size_t group_matches = 0;

        const auto flush_group = [&] {
            if (group_matches == 0) {
                group.clear();
                return;
            }
            for (auto& item: group) {
                results_.push_back(std::move(item));
            }
            group.clear();
        };

        for (auto& item: items_) {
            if (item.kind == MenuItemKind::separator) {
                flush_group();
                if (!results_.empty()) {
                    // 分隔线只用来隔开两个非空组；紧邻的重复分隔线被折叠掉。
                    if (results_.back().kind != MenuItemKind::separator) {
                        results_.push_back(item);
                    }
                }
                group_matches = 0;
                continue;
            }
            if (item.kind == MenuItemKind::label) {
                flush_group();
                group.clear();
                group.push_back(item);
                group_matches = 0;
                continue;
            }
            if (!contains_case_insensitive(item.label, needle)) {
                continue;
            }
            if (item.kind == MenuItemKind::submenu) {
                // 命令面板不接受子菜单：它要的是"一次激活就执行"。子菜单条目按可执行
                // 条目处理会误导，因此直接跳过（不是静默：模型校验见 tests）。
                continue;
            }
            ++group_matches;
            group.push_back(item);
        }
        flush_group();
        // 结尾不保留悬空的分隔线。
        while (!results_.empty() && results_.back().kind == MenuItemKind::separator) {
            results_.pop_back();
        }

        // 上限裁剪：只保留前 N 个可聚焦条目，结构条目随最后一个保留条目一起收尾。
        if (const auto focusable = filtered_ids().size(); focusable > max_visible_results_) {
            hidden_results_ = focusable - max_visible_results_;
            std::size_t kept = 0;
            std::size_t cut = results_.size();
            for (std::size_t i = 0; i < results_.size(); ++i) {
                if (!menu_item_is_focusable(results_[i])) {
                    continue;
                }
                if (kept == max_visible_results_) {
                    cut = i;
                    break;
                }
                ++kept;
            }
            results_.resize(cut);
            while (!results_.empty() && results_.back().kind == MenuItemKind::separator) {
                results_.pop_back();
            }
        }

        surface_->set_rows(results_, hidden_results_, !needle.empty());
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    void CommandPalette::sync_field_style() {
        const auto style = resolved_style();
        // 查询框在面板内部是"无壳"的：外壳与边框由面板承担，否则会出现双层描边。
        // 高度沿用 TextField 自己的度量，面板只按测得高度摆放。
        text_field_->set_override(
            theme::TextFieldRecipeRule {
                .container_fill = theme::ThemeColor::literal(style.panel.fill.with_alpha(0.0F)),
                .container_border = theme::ThemeColor::literal(style.panel.border.with_alpha(0.0F)),
                .container_border_width = theme::ThemeScalar::literal(0.0F),
                .container_radius = theme::ThemeScalar::literal(0.0F),
                .value_color = theme::ThemeColor::literal(style.query.color),
                .placeholder_color = theme::ThemeColor::literal(style.placeholder.color),
                .font_size = theme::ThemeScalar::literal(style.query.font_size),
            }
        );
    }

    void CommandPalette::apply_style() {
        const auto style = resolved_style();
        panel_->set_style(style);
        surface_->set_style(style, resolved_style_context());
        shell_->set_top_offset(style.metrics.panel_top_offset);
        dismiss_layer_->set_scrim(scrim_style(style));
    }

    void CommandPalette::handle_query_changed(const std::string_view query) {
        refresh_results();
        surface_->set_active_edge(false);
        if (on_query_change_) {
            on_query_change_(query);
        }
    }

    void CommandPalette::activate_id(const std::string_view id) {
        if (disabled_ || id.empty()) {
            return;
        }
        // `id` 通常指向结果表面持有的行（`active_id()` / 行点击都如此），而下面
        // `refresh_results()` 会重灌结果、释放那些行。先取一份自己的副本，否则后面
        // 每一次读取 id 都是 use-after-free（ASan 在勾选条目那条路径上抓到过）。
        const std::string owned {id};
        auto* item = find_menu_item(items_, owned);
        if (item == nullptr || !menu_item_is_activatable(*item)) {
            return;
        }

        if (item->kind == MenuItemKind::checkbox || item->kind == MenuItemKind::radio) {
            MenuSelection selection {selection_mode_};
            if (selection.toggle(items_, owned)) {
                refresh_results();
            }
        }

        if (on_select_) {
            on_select_(owned);
        }
        item_selected_.emit(owned);

        // 动作条目激活后关闭整块面板；勾选类条目保持打开，便于连续调整（同 DropdownMenu）。
        if (item->kind != MenuItemKind::checkbox && item->kind != MenuItemKind::radio) {
            close();
        }
    }
} // namespace nandina::widget
