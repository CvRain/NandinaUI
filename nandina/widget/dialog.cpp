//
// widget/dialog - modal overlay (scrim + centered panel + focus trap).
//

#include "dialog.hpp"

#include "../animation/animation_host.hpp"
#include "../render/draw_context.hpp"
#include "../scene/input_event.hpp"
#include "../scene/scene_tree.hpp"
#include "../theme/theme_manager.hpp"
#include "primitives/box_painter.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace nandina::widget
{
    namespace
    {
        // GLFW 键码：Escape / Tab。
        constexpr int key_escape = 256;
        constexpr int key_tab = 258;

        void collect_focusable(scene::NanNode& node, std::vector<scene::NanNode2D*>& out) {
            if (auto* as_node2d = node.as_node2d(); as_node2d != nullptr
                && as_node2d->is_focusable() && as_node2d->is_visible_in_tree())
            {
                out.push_back(as_node2d);
            }
            for (std::size_t i = 0; i < node.child_count(); ++i) {
                if (auto* child = node.get_child(i); child != nullptr) {
                    collect_focusable(*child, out);
                }
            }
        }

        [[nodiscard]] auto
        same_text_style(const primitives::TextStyle& lhs, const primitives::TextStyle& rhs)
            -> bool {
            return lhs.color.approx_equals(rhs.color)
                && std::abs(lhs.font_size - rhs.font_size) <= foundation::nan_epsilon
                && lhs.font == rhs.font && lhs.overflow == rhs.overflow
                && lhs.max_lines == rhs.max_lines;
        }
    } // namespace

    Dialog::Dialog(theme::NanTheme theme): title_text_("") {
        system_ =
            std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        set_visible(false); // 初始关闭：隐藏面板与内容子节点。
    }

    auto Dialog::create(theme::NanTheme theme) -> std::shared_ptr<Dialog> {
        return std::make_shared<Dialog>(theme);
    }

    void Dialog::set_title(std::string title) {
        title_text_.set_text(std::move(title));
        apply_text_style();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto Dialog::title() const -> std::string_view {
        return title_text_.text();
    }

    auto Dialog::set_content(std::shared_ptr<scene::NanControl> content) -> Dialog& {
        if (!content) {
            throw std::runtime_error("Dialog::set_content: content is null");
        }
        auto current = content_.lock();
        content_ = content;
        replace_child(current.get(), std::move(content));
        mark_layout_dirty();
        return *this;
    }

    void Dialog::open() {
        if (phase_ == DialogPhase::opening || phase_ == DialogPhase::opened) {
            return;
        }
        phase_ = DialogPhase::opening;
        set_visible(true);
        // 用 long_duration + ease_out，让面板「浮现」而非「闪现」。
        fade_.set_behavior(
            animation::Behavior<float>(
                system_->tokens.motion.long_duration, animation::Easing::ease_out
            )
        );
        start_fade(1.0F);
        request_focus();
        mark_dirty(
            scene::DirtyFlags::paint | scene::DirtyFlags::layout | scene::DirtyFlags::semantics
        );
    }

    void Dialog::close() {
        if (phase_ == DialogPhase::closed || phase_ == DialogPhase::closing) {
            return;
        }
        phase_ = DialogPhase::closing;
        fade_.set_behavior(
            animation::Behavior<float>(
                system_->tokens.motion.long_duration, animation::Easing::ease_in
            )
        );
        start_fade(0.0F);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto Dialog::is_open() const -> bool {
        return phase_ == DialogPhase::opening || phase_ == DialogPhase::opened;
    }

    void Dialog::set_dismissible(const bool dismissible) {
        dismissible_ = dismissible;
    }

    auto Dialog::dismissible() const -> bool {
        return dismissible_;
    }

    void Dialog::set_on_close(std::function<void()> callback) {
        on_close_ = std::move(callback);
    }

    void Dialog::set_theme(theme::NanTheme theme) {
        system_ =
            std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        apply_text_style();
        mark_layout_dirty();
    }

    auto Dialog::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Dialog::set_override(theme::DialogRecipeRule rule) {
        override_ = std::move(rule);
        apply_text_style();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Dialog::resolved_style() const -> theme::ResolvedDialogStyle {
        auto style = theme::resolve_dialog(*system_, appearance_);
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Dialog::set_text_pipeline(primitives::TextPipeline pipeline) {
        title_text_.set_text_pipeline(std::move(pipeline));
        mark_layout_dirty();
    }

    void Dialog::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        title_text_.apply_default_text_pipeline(pipeline);
        mark_layout_dirty();
    }

    void Dialog::apply_font_context(text::FontPipelineCache& context) {
        title_text_.apply_font_context(context);
        mark_layout_dirty();
    }

    void Dialog::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_text_style();
        mark_layout_dirty();
    }

    void Dialog::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_text_style();
        mark_layout_dirty();
    }

    auto Dialog::local_opacity() const -> float {
        return NanNode2D::local_opacity() * fade_.value();
    }

    auto Dialog::z_index_hint() const -> int {
        return active() ? 1 : 0;
    }

    auto Dialog::global_bounds() const -> foundation::NanRect {
        return scene::NanControl::global_bounds();
    }

    auto Dialog::contains_point(const foundation::NanPoint /*local_point*/) const -> bool {
        return active();
    }

    auto Dialog::is_focusable() const -> bool {
        return active();
    }

    auto Dialog::on_input(scene::InputEvent& event) -> bool {
        if (event.type() == scene::EventType::key) {
            auto& key = static_cast<scene::KeyEvent&>(event);
            if (key.action() != scene::KeyEvent::Action::press) {
                return false;
            }
            if (key.keycode() == key_escape && dismissible_) {
                close();
                return true;
            }
            if (key.keycode() == key_tab) {
                trap_focus(key.modifiers().shift);
                return true;
            }
            return false;
        }
        if (event.type() == scene::EventType::mouse_button) {
            auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
            if (mouse.button() != scene::MouseButtonEvent::Button::left || !mouse.is_pressed()) {
                return false;
            }
            const auto local = to_local(mouse.screen_pos());
            if (!panel_rect().contains_point(local) && dismissible_) {
                close();
            }
            return true; // 模态：吞掉遮罩上的点击
        }
        return false;
    }

    auto Dialog::on_draw(render::DrawContext& context) -> void {
        if (!active()) {
            return;
        }
        const auto style = resolved_style();
        apply_text_style();
        (void)title_text_.measure_layout(scene::LayoutConstraints::loose());

        // 遮罩覆盖整个父容器。context.opacity() 已含 per-node opacity（淡入淡出）。
        const auto full = render::world_bounds_from_local(context.world_transform(), local_rect());
        primitives::BoxPainter::paint(
            context,
            full,
            theme::ResolvedBoxStyle {
                .fill = style.scrim,
                .border = style.scrim,
                .border_width = 0.0F,
                .radius = 0.0F,
            },
            context.opacity()
        );

        // 居中面板（随同一 fade 淡入淡出）。
        const auto panel_world =
            render::world_bounds_from_local(context.world_transform(), panel_rect());
        primitives::BoxPainter::paint(context, panel_world, style.panel, context.opacity());

        const auto title_position = foundation::NanPoint(
            panel_world.get_left() + context.logical_to_screen(style.metrics.padding_x),
            panel_world.get_top() + context.logical_to_screen(style.metrics.padding_y)
        );
        title_text_.draw_at(context, title_position);
    }

    void Dialog::on_process(const float /*dt*/) {
        // 淡入淡出由 AnimationHost 推进；这里只负责状态机完成转换。
        if (phase_ == DialogPhase::opening && !fade_.is_animating()) {
            phase_ = DialogPhase::opened;
        }
        else if (phase_ == DialogPhase::closing && !fade_.is_animating()) {
            phase_ = DialogPhase::closed;
            set_visible(false);
            if (on_close_) {
                on_close_();
            }
            mark_dirty(
                scene::DirtyFlags::paint | scene::DirtyFlags::layout | scene::DirtyFlags::semantics
            );
        }
    }

    auto Dialog::on_measure(const scene::LayoutConstraints constraints) -> foundation::NanSize {
        if (!active()) {
            return constraints.constrain(foundation::NanSize {0.0F, 0.0F});
        }
        return constraints.constrain(
            foundation::NanSize(constraints.max_width, constraints.max_height)
        );
    }

    auto Dialog::on_layout() -> void {
        if (!active()) {
            return;
        }
        const auto style = resolved_style();
        apply_text_style();
        (void)title_text_.measure_layout(scene::LayoutConstraints::loose());

        auto content = content_.lock();
        const auto panel = panel_rect();

        if (content) {
            const float title_height = title_text_.measured_size().get_height();
            const float inner_x = panel.get_left() + style.metrics.padding_x;
            const float inner_y =
                panel.get_top() + style.metrics.padding_y + title_height + style.metrics.gap;
            const float inner_width =
                std::max(0.0F, panel.get_width() - style.metrics.padding_x * 2.0F);
            const float inner_height = std::max(
                0.0F,
                panel.get_height() - style.metrics.padding_y * 2.0F - title_height
                    - style.metrics.gap
            );
            const auto measured = content->measure_layout(
                scene::LayoutConstraints {
                    .min_width = 0.0F,
                    .max_width = inner_width,
                    .min_height = 0.0F,
                    .max_height = inner_height,
                }
            );
            content->layout_to(
                foundation::NanRect::from_xywh(
                    inner_x,
                    inner_y,
                    measured.get_width(),
                    measured.get_height()
                )
            );
        }
    }

    auto Dialog::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::dialog,
            .label = std::string(title()),
            .state = {.focusable = active(), .focused = active()},
        };
    }

    void Dialog::apply_text_style() {
        const auto style = resolved_style();
        const auto& context = resolved_style_context();
        const primitives::TextStyle text_style {
            .color = context.text_color_from_context ? context.text_color : style.title.color,
            .font_size = context.font_size_from_context ? context.font_size : style.title.font_size,
            .font = context.font_from_context ? context.font : title_text_.font(),
            .overflow = primitives::TextOverflow::clip,
            .max_lines = 1,
        };
        if (!same_text_style(title_text_.style(), text_style)) {
            title_text_.set_style(text_style);
        }
    }

    auto Dialog::panel_rect() const -> foundation::NanRect {
        const auto style = resolved_style();
        constexpr float margin = 16.0F;
        const float panel_width =
            std::min(style.metrics.panel_width, std::max(0.0F, width() - margin * 2.0F));

        float content_height = 0.0F;
        if (auto content = content_.lock()) {
            content_height = content->measured_size().get_height();
        }
        const float title_height = title_text_.measured_size().get_height();
        const float gap = content_height > 0.0F ? style.metrics.gap : 0.0F;
        const float panel_height = std::max(
            style.metrics.min_height,
            style.metrics.padding_y * 2.0F + title_height + gap + content_height
        );
        return foundation::NanRect::from_center(
            local_rect().get_center(),
            foundation::NanSize(panel_width, panel_height)
        );
    }

    void Dialog::start_fade(const float target) {
        if (auto* tree = get_tree(); tree != nullptr) {
            tree->animation_host().set_target(*this, fade_, target, scene::DirtyFlags::paint);
        }
        else {
            fade_.clear_behavior();
            fade_.set_target(target);
        }
    }

    void Dialog::trap_focus(const bool backwards) {
        auto* tree = get_tree();
        if (tree == nullptr) {
            return;
        }
        std::vector<scene::NanNode2D*> nodes;
        collect_focusable(*this, nodes);
        if (nodes.empty()) {
            return;
        }
        auto* focused = tree->focused_node();
        const auto found = std::ranges::find(nodes, focused);
        if (found == nodes.end()) {
            tree->set_focus(backwards ? nodes.back() : nodes.front());
            return;
        }
        if (backwards) {
            tree->set_focus(found == nodes.begin() ? nodes.back() : *(found - 1));
        }
        else {
            const auto next = std::next(found);
            tree->set_focus(next == nodes.end() ? nodes.front() : *next);
        }
    }

} // namespace nandina::widget
