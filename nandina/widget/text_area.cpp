//
// widget/text_area — semantic multi-line text input shell.
//

#include "text_area.hpp"

#include "key_codes.hpp"

#include "primitives/box_painter.hpp"
#include "primitives/focus_ring_painter.hpp"
#include "../render/draw_context.hpp"
#include "../scene/input_event.hpp"
#include "../scene/scene_tree.hpp"
#include "../theme/theme_manager.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        constexpr int key_kp_enter = 335;
        /// 鼠标滚轮一格滚动的逻辑像素数；与 ScrollView 的默认 wheel_step 对齐。
        constexpr float wheel_step = 40.0F;
    } // namespace

    TextArea::TextArea(std::string value, theme::NanTheme theme):
        edit_(std::move(value)) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        edit_.set_on_change([this](std::string_view value) {
            if (on_change_) {
                on_change_(value);
            }
            value_changed_.emit(value);
        });
        apply_theme();
        (void)measure_layout(scene::LayoutConstraints::loose());
    }

    auto TextArea::create(std::string value, theme::NanTheme theme) -> std::shared_ptr<TextArea> {
        return std::make_shared<TextArea>(std::move(value), theme);
    }

    void TextArea::set_value(std::string value) {
        edit_.set_value(std::move(value));
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto TextArea::value() const -> std::string_view {
        return edit_.value();
    }

    void TextArea::set_placeholder(std::string placeholder) {
        placeholder_.set_text(std::move(placeholder));
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto TextArea::placeholder() const -> std::string_view {
        return placeholder_.text();
    }

    void TextArea::set_read_only(const bool value) {
        read_only_ = value;
        edit_.set_read_only(value);
        // read_only 选择器会改变容器填充，需要按新快照重解析。
        apply_theme();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto TextArea::read_only() const -> bool {
        return read_only_;
    }

    void TextArea::set_disabled(const bool value) {
        disabled_ = value;
        dragging_ = false;
        if (disabled_ && is_inside_tree() && get_tree()->focused_node() == this) {
            get_tree()->set_focus(nullptr);
        }
        apply_theme();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto TextArea::disabled() const -> bool {
        return disabled_;
    }

    void TextArea::set_rows(const int rows) {
        rows_explicit_ = true;
        rows_ = std::max(1, rows);
        mark_layout_dirty();
    }

    auto TextArea::rows() const -> int {
        return rows_;
    }

    void TextArea::set_on_change(std::function<void(std::string_view)> callback) {
        on_change_ = std::move(callback);
    }

    auto TextArea::value_changed() const -> const reactive::Event<std::string_view>& {
        return value_changed_;
    }

    void TextArea::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        apply_theme();
        mark_layout_dirty();
    }

    auto TextArea::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void TextArea::set_override(theme::TextAreaRecipeRule rule) {
        override_ = std::move(rule);
        apply_theme();
        mark_layout_dirty();
    }

    auto TextArea::visual_state() const -> theme::TextAreaVisualState {
        auto state = theme::TextAreaVisualState::normal;
        if (focused_) state = state | theme::TextAreaVisualState::focused;
        if (disabled_) state = state | theme::TextAreaVisualState::disabled;
        return state;
    }

    auto TextArea::resolved_style() const -> theme::ResolvedTextAreaStyle {
        auto style = theme::resolve_text_area(*system_, appearance_, visual_state(), read_only_);
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    auto TextArea::editable_text() -> primitives::EditableText& {
        return edit_;
    }

    auto TextArea::editable_text() const -> const primitives::EditableText& {
        return edit_;
    }

    auto TextArea::scroll_offset() const -> foundation::NanPoint {
        return foundation::NanPoint(0.0F, scroll_y_);
    }

    void TextArea::set_text_pipeline(primitives::TextPipeline pipeline) {
        edit_.set_text_pipeline(pipeline);
        placeholder_.set_text_pipeline(pipeline);
        mark_layout_dirty();
        (void)measure_layout(last_layout_constraints());
    }

    auto TextArea::text_pipeline() const -> primitives::TextPipeline {
        return edit_.text_pipeline();
    }

    void TextArea::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        edit_.apply_default_text_pipeline(pipeline);
        placeholder_.apply_default_text_pipeline(pipeline);
        mark_layout_dirty();
    }

    void TextArea::apply_font_context(text::FontPipelineCache& context) {
        edit_.apply_font_context(context);
        placeholder_.apply_font_context(context);
        mark_layout_dirty();
    }

    void TextArea::on_style_context_changed(const theme::ResolvedStyleContext& /*context*/) {
        apply_theme();
        mark_layout_dirty();
    }

    void TextArea::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_theme();
        mark_layout_dirty();
    }

    void TextArea::set_font(text::FontRequest request) {
        font_explicit_ = true;
        edit_.text_node().set_font(request);
        placeholder_.set_font(std::move(request));
        mark_layout_dirty();
    }

    void TextArea::set_font_family(resource::ResourceKey family) {
        font_explicit_ = true;
        edit_.text_node().set_font_family(family);
        placeholder_.set_font_family(std::move(family));
        mark_layout_dirty();
    }

    void TextArea::set_font_weight(const int weight) {
        font_explicit_ = true;
        edit_.text_node().set_font_weight(weight);
        placeholder_.set_font_weight(weight);
        mark_layout_dirty();
    }

    void TextArea::set_font_slant(const text::FontSlant slant) {
        font_explicit_ = true;
        edit_.text_node().set_font_slant(slant);
        placeholder_.set_font_slant(slant);
        mark_layout_dirty();
    }

    auto TextArea::is_focusable() const -> bool {
        return !disabled_;
    }

    auto TextArea::on_input(scene::InputEvent& event) -> bool {
        auto* clipboard = is_inside_tree() ? get_tree()->clipboard() : nullptr;
        if (event.type() == scene::EventType::focus_enter) {
            focused_ = !disabled_;
            apply_theme();
            mark_semantics_dirty();
            return edit_.handle_input(event, clipboard);
        }
        if (event.type() == scene::EventType::focus_leave) {
            focused_ = false;
            dragging_ = false;
            edit_.clear_composition();
            apply_theme();
            mark_semantics_dirty();
            return edit_.handle_input(event, clipboard);
        }
        if (disabled_) {
            return false;
        }
        if (event.type() == scene::EventType::mouse_wheel) {
            auto& wheel = static_cast<scene::MouseWheelEvent&>(event);
            const float before = scroll_y_;
            scroll_y_ = std::clamp(
                scroll_y_ - wheel.delta().get_y() * wheel_step,
                0.0F,
                maximum_scroll_y()
            );
            // 与 ScrollView 同款：到边界不消费，让父级滚动容器接手。
            if (scroll_y_ != before) {
                event.accept();
                return true;
            }
            return false;
        }
        if (event.type() == scene::EventType::mouse_button) {
            auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
            if (mouse.button() == scene::MouseButtonEvent::Button::left && mouse.is_pressed()) {
                place_caret(mouse.screen_pos(), mouse.modifiers().shift);
                dragging_ = true;
                get_tree()->set_pointer_capture(this);
                event.accept();
                return true;
            }
            if (mouse.button() == scene::MouseButtonEvent::Button::left) {
                dragging_ = false;
                get_tree()->release_pointer_capture(this);
                event.accept();
                return true;
            }
        }
        if (event.type() == scene::EventType::mouse_move && dragging_) {
            place_caret(static_cast<scene::MouseMoveEvent&>(event).screen_pos(), true);
            event.accept();
            return true;
        }
        if (event.type() == scene::EventType::key) {
            const auto& key = static_cast<scene::KeyEvent&>(event);
            if (key.is_pressed() && (key.keycode() == keys::enter || key.keycode() == key_kp_enter)) {
                // 多行输入：Enter 插入换行，不提交、不激活。
                if (!read_only_) {
                    edit_.insert_text("\n");
                }
                event.accept();
                return true;
            }
        }
        return edit_.handle_input(event, clipboard);
    }

    void TextArea::on_draw(render::DrawContext& ctx) {
        const auto world = render::world_bounds_from_local(ctx.world_transform(), local_rect());
        const auto style = resolved_style();
        primitives::BoxPainter::paint(ctx, world, style.container, ctx.opacity());
        if (style.focus.width > 0.0F && style.focus.color.alpha() > 0.0F) {
            primitives::FocusRingPainter::paint(
                ctx,
                world,
                style.focus,
                ctx.opacity(),
                /*gap=*/0.0F
            );
        }

        update_scroll(viewport_height());
        const float padding_x = ctx.logical_to_screen(padding_x_);
        const float padding_y = ctx.logical_to_screen(padding_y_);
        // 让最后一个字形在 content box 右缘得以完整显示：在内容框宽度基础上放出一个
        // 字形悬垂余量（advance 之外的墨迹 1~2px），否则近满时末字形的右侧墨迹被裁。
        const float overhang = ctx.logical_to_screen(
            primitives::glyph_overhang_allowance(edit_.style().font_size)
        );
        const auto viewport = foundation::NanRect::from_xywh(
            world.get_left() + padding_x,
            world.get_top() + padding_y,
            std::max(0.0F, world.get_width() - padding_x * 2.0F + overhang),
            std::max(0.0F, world.get_height() - padding_y * 2.0F)
        );
        auto clip = ctx.clip().push(viewport);
        if (edit_.value().empty() && !placeholder_.text().empty()) {
            placeholder_.draw_at(
                ctx,
                foundation::NanPoint(viewport.get_left(), world.get_top() + padding_y)
            );
            return;
        }
        edit_.draw_at(
            ctx,
            foundation::NanPoint(
                viewport.get_left(),
                world.get_top() + padding_y - ctx.logical_to_screen(scroll_y_)
            )
        );
    }

    auto TextArea::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto content = content_constraints(constraints);
        const auto edit_size = edit_.measure_layout(
            scene::LayoutConstraints {
                .min_width = 0.0F,
                .max_width = content.max_width,
                .min_height = 0.0F,
                .max_height = std::numeric_limits<float>::infinity(),
            }
        );
        const auto placeholder_size = placeholder_.measure_layout(content);
        const float natural_width =
            std::max(edit_size.get_width(), placeholder_size.get_width()) + padding_x_ * 2.0F;
        // 默认高度 = 可见行数 × 行高 + 垂直内边距；内容更高时由垂直滚动承载。
        const float natural_height =
            line_height_ * static_cast<float>(std::max(1, rows_)) + padding_y_ * 2.0F;
        const auto measured = constraints.constrain(foundation::NanSize(natural_width, natural_height));
        set_size(measured);
        return measured;
    }

    void TextArea::on_layout() {
        const float available_width = std::max(0.0F, width() - padding_x_ * 2.0F);
        const auto content = scene::LayoutConstraints {
            .min_width = 0.0F,
            .max_width = available_width,
            .min_height = 0.0F,
            .max_height = std::numeric_limits<float>::infinity(),
        };
        // 换行依赖最终宽度：布局后用真实 viewport 宽度重排，滚动几何才与绘制一致。
        (void)edit_.measure_layout(content);
        (void)placeholder_.measure_layout(content);
        update_scroll(viewport_height());
    }

    auto TextArea::semantics_properties() const -> semantics::Properties {
        auto actions = disabled_ ? semantics::Action::none : semantics::Action::focus;
        if (!disabled_ && !read_only_) {
            actions |= semantics::Action::set_value;
        }
        return {
            .role = semantics::Role::text_field,
            .label = std::string(placeholder()),
            .value = std::string(value()),
            .state = {
                .focusable = !disabled_,
                .focused = focused_,
                .disabled = disabled_,
                .read_only = read_only_,
            },
            .actions = actions,
        };
    }

    auto TextArea::on_semantics_action(const semantics::ActionRequest& request) -> bool {
        if (request.action != semantics::Action::set_value || disabled_ || read_only_) {
            return false;
        }
        set_value(request.value);
        return true;
    }

    void TextArea::apply_theme() {
        const auto style = resolved_style();
        padding_x_ = style.metrics.padding_x;
        padding_y_ = style.metrics.padding_y;
        line_height_ = std::max(0.0F, style.metrics.line_height);
        if (!rows_explicit_) {
            rows_ = std::max(1, static_cast<int>(std::lround(style.metrics.rows)));
        }

        const auto& context = resolved_style_context();
        const auto font = context.font_from_context && !font_explicit_ ? context.font
                                                                       : edit_.style().font;
        const auto font_size =
            context.font_size_from_context ? context.font_size : style.value.font_size;
        const auto text_color = context.text_color_from_context ? context.text_color
                                                                : style.value.color;
        const primitives::TextStyle value_style {
            .color = text_color,
            .font_size = font_size,
            .font = font,
            .overflow = primitives::TextOverflow::wrap,
            // 不设行数上限：多行布局必须覆盖全部内容，滚动由外壳负责。
            .max_lines = std::numeric_limits<int>::max(),
        };
        const primitives::TextStyle placeholder_style {
            .color = context.text_color_from_context ? context.text_color
                                                     : style.placeholder.color,
            .font_size = font_size,
            .font = font,
            .overflow = primitives::TextOverflow::ellipsis,
            .max_lines = 1,
        };
        edit_.set_style(value_style);
        edit_.set_selection_color(style.selection);
        placeholder_.set_style(placeholder_style);
    }

    auto TextArea::content_constraints(scene::LayoutConstraints constraints) const
        -> scene::LayoutConstraints {
        return {
            .min_width = 0.0F,
            .max_width = std::isfinite(constraints.max_width)
                ? std::max(0.0F, constraints.max_width - padding_x_ * 2.0F)
                : constraints.max_width,
            .min_height = 0.0F,
            .max_height = std::isfinite(constraints.max_height)
                ? std::max(0.0F, constraints.max_height - padding_y_ * 2.0F)
                : constraints.max_height,
        };
    }

    auto TextArea::content_height() const -> float {
        return edit_.text_node().layout_result().size.get_height();
    }

    auto TextArea::viewport_height() const -> float {
        return std::max(0.0F, height() - padding_y_ * 2.0F);
    }

    auto TextArea::maximum_scroll_y() const -> float {
        return std::max(0.0F, content_height() - viewport_height());
    }

    void TextArea::place_caret(const foundation::NanPoint screen_point, const bool extend) {
        const auto local = to_local(screen_point);
        // 与 TextField 同款：映射到局部坐标、补偿滚动偏移，再交给布局做命中测试。
        const auto stop = edit_.text_node().layout_result().caret_for_point(
            foundation::NanPoint(
                local.get_x() - padding_x_,
                local.get_y() - padding_y_ + scroll_y_
            )
        );
        if (extend) {
            auto selection = edit_.selection();
            selection.focus = stop.source_offset;
            selection.focus_affinity = stop.affinity;
            edit_.set_selection(selection);
        }
        else {
            edit_.set_caret(stop.source_offset, stop.affinity);
        }
    }

    void TextArea::update_scroll(const float viewport) {
        const auto geometry = edit_.caret_geometry();
        if (geometry.height <= 0.0F) {
            scroll_y_ = 0.0F;
            return;
        }
        // 垂直轴版的 TextField::update_scroll：先把插入符拉进视口，再夹到内容范围。
        if (geometry.top < scroll_y_) {
            scroll_y_ = geometry.top;
        }
        if (geometry.top + geometry.height > scroll_y_ + viewport) {
            scroll_y_ = geometry.top + geometry.height - viewport;
        }
        scroll_y_ = std::clamp(scroll_y_, 0.0F, maximum_scroll_y());
    }

} // namespace nandina::widget
