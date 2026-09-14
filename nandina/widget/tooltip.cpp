//
// widget/tooltip - hover-triggered floating hint over a trigger control.
//

#include "tooltip.hpp"

#include "primitives/box_painter.hpp"
#include "../render/draw_context.hpp"
#include "../scene/input_event.hpp"
#include "../theme/theme_manager.hpp"
#include "internal/anchored_positioner.hpp"
#include "../scene/overlay_host.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        /// Shared bubble painting for the detached (in-tree) and portal paths so the
        /// two cannot drift apart.
        void paint_tooltip_bubble(
            render::DrawContext& context,
            const foundation::NanRect& world,
            const theme::ResolvedTooltipStyle& style,
            primitives::Text& text
        ) {
            primitives::BoxPainter::paint(context, world, style.container, context.opacity());
            const float text_width = context.logical_to_screen(text.measured_text_width());
            const float text_height = context.logical_to_screen(text.measured_text_height());
            text.draw_at(
                context,
                foundation::NanPoint(
                    world.get_left() + (world.get_width() - text_width) * 0.5F,
                    world.get_top() + (world.get_height() - text_height) * 0.5F
                )
            );
        }

        /// Portal-hosted tooltip bubble. Presentational only: it must never take
        /// pointer hits, otherwise a bubble that overlaps its trigger would steal
        /// clicks from it.
        class TooltipBubble final: public scene::NanControl {
        public:
            TooltipBubble(
                std::string text,
                theme::ResolvedTooltipStyle style,
                const primitives::TextPipeline& pipeline,
                primitives::TextStyle text_style
            ):
                text_(std::move(text)), style_(std::move(style)) {
                text_.set_text_pipeline(pipeline);
                // The resolved label style must travel with the bubble, otherwise it
                // would render with the default TextStyle instead of the tooltip's
                // colour, font size and font family.
                text_.set_style(std::move(text_style));
            }

            [[nodiscard]] auto contains_point(foundation::NanPoint) const -> bool override {
                return false;
            }

            [[nodiscard]] auto measured_bubble_size() -> foundation::NanSize {
                (void)text_.measure_layout(scene::LayoutConstraints::loose());
                return foundation::NanSize(
                    text_.measured_text_width() + style_.metrics.padding_x * 2.0F,
                    style_.metrics.min_height
                );
            }

        protected:
            [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
                -> foundation::NanSize override {
                return constraints.constrain(measured_bubble_size());
            }

            void on_draw(render::DrawContext& context) override {
                const auto world =
                    render::world_bounds_from_local(context.world_transform(), local_rect());
                paint_tooltip_bubble(context, world, style_, text_);
            }

        private:
            primitives::Text text_;
            theme::ResolvedTooltipStyle style_;
        };

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
    } // namespace

    Tooltip::~Tooltip() = default;

    Tooltip::Tooltip(
        std::string text,
        std::shared_ptr<scene::NanControl> trigger,
        theme::NanTheme theme
    ):
        text_(std::move(text)) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        if (trigger) {
            set_trigger(std::move(trigger));
        }
        apply_text_style();
    }

    auto Tooltip::create(
        std::string text,
        std::shared_ptr<scene::NanControl> trigger,
        theme::NanTheme theme
    ) -> std::shared_ptr<Tooltip> {
        return std::make_shared<Tooltip>(std::move(text), std::move(trigger), theme);
    }

    void Tooltip::set_text(std::string text) {
        text_.set_text(std::move(text));
        apply_text_style();
        refresh_portal();
        mark_layout_dirty();
        mark_semantics_dirty();
    }

    auto Tooltip::text() const -> std::string_view {
        return text_.text();
    }

    auto Tooltip::set_trigger(std::shared_ptr<scene::NanControl> trigger) -> Tooltip& {
        if (!trigger) {
            throw std::runtime_error("Tooltip::set_trigger: trigger is null");
        }
        auto current = trigger_.lock();
        trigger_ = trigger;
        replace_child(current.get(), std::move(trigger));
        sync_portal();
        mark_layout_dirty();
        return *this;
    }

    void Tooltip::set_placement(const Placement placement) {
        placement_ = placement;
        sync_portal();
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Tooltip::placement() const -> Placement {
        return placement_;
    }

    void Tooltip::set_delay(const float seconds) {
        if (!std::isfinite(seconds) || seconds < 0.0F) {
            throw std::invalid_argument("tooltip delay must be finite and non-negative");
        }
        delay_ = seconds;
    }

    auto Tooltip::delay() const -> float {
        return delay_;
    }

    auto Tooltip::visible() const -> bool {
        return visible_;
    }

    void Tooltip::show() {
        if (visible_) {
            return;
        }
        visible_ = true;
        sync_portal();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    void Tooltip::hide() {
        if (!visible_) {
            return;
        }
        visible_ = false;
        close_portal();
        hover_time_ = 0.0F;
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    void Tooltip::on_exit_tree() {
        // The portal bubble lives in the overlay layer rather than under this
        // subtree, so an unmounted tooltip must release it explicitly.
        visible_ = false;
        hovered_ = false;
        hover_time_ = 0.0F;
        close_portal();
    }

    void Tooltip::set_theme(theme::NanTheme theme) {
        system_ = std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        apply_text_style();
        refresh_portal();
        mark_layout_dirty();
    }

    auto Tooltip::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Tooltip::set_override(theme::TooltipRecipeRule rule) {
        override_ = std::move(rule);
        apply_text_style();
        refresh_portal();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Tooltip::resolved_style() const -> theme::ResolvedTooltipStyle {
        auto style = theme::resolve_tooltip(*system_, appearance_);
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Tooltip::set_text_pipeline(primitives::TextPipeline pipeline) {
        text_.set_text_pipeline(std::move(pipeline));
        mark_layout_dirty();
    }

    void Tooltip::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        text_.apply_default_text_pipeline(pipeline);
        mark_layout_dirty();
    }

    void Tooltip::apply_font_context(text::FontPipelineCache& context) {
        text_.apply_font_context(context);
        mark_layout_dirty();
    }

    void Tooltip::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_text_style();
        refresh_portal();
        mark_layout_dirty();
    }

    void Tooltip::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_text_style();
        refresh_portal();
        mark_layout_dirty();
    }

    auto Tooltip::on_input(scene::InputEvent& event) -> bool {
        // 只观察悬停，不消费输入——触发控件仍需接收点击/键盘。
        if (event.type() == scene::EventType::mouse_enter) {
            hovered_ = true;
            hover_time_ = 0.0F;
            return false;
        }
        if (event.type() == scene::EventType::mouse_leave) {
            hovered_ = false;
            hide();
            return false;
        }
        return false;
    }

    void Tooltip::on_process(const float dt) {
        if (hovered_ && !visible_) {
            hover_time_ += std::max(dt, 0.0F);
            if (delay_ <= 0.0F || hover_time_ >= delay_) {
                show();
            }
        }
        if (visible_) {
            // Keep the bubble anchored while the trigger scrolls or moves.
            sync_portal();
        }
    }

    void Tooltip::sync_portal() {
        if (!visible_) {
            close_portal();
            return;
        }
        auto* host = resolve_overlay_host();
        auto trigger = trigger_.lock();
        if (host == nullptr || trigger == nullptr) {
            close_portal();
            return;
        }

        const auto viewport_size = host->viewport_size();
        const auto anchor = trigger->global_bounds();
        // Positioning needs a laid-out viewport and trigger. Either can be missing on
        // the first frame or immediately after set_trigger(); skip until they are
        // valid rather than feeding the positioner an invalid rect.
        if (!viewport_size.is_valid() || !anchor.is_valid()) {
            return;
        }
        const auto viewport = foundation::NanRect::from_origin_size(
            foundation::NanPoint::zero(),
            viewport_size
        );
        const auto options = placement_options();

        auto bubble = portal_bubble_.lock();
        if (bubble == nullptr || portal_handle_ == nullptr || !portal_handle_->mounted()) {
            close_portal();
            auto created = std::make_shared<TooltipBubble>(
                std::string(text()),
                resolved_style(),
                text_.text_pipeline(),
                apply_text_style()
            );
            // Measure against loose constraints so the size is known before the
            // overlay layer gets a chance to lay the bubble out.
            const auto bubble_size = created->measure_layout(scene::LayoutConstraints::loose());
            if (!bubble_size.is_valid()) {
                return;
            }
            const auto position =
                internal::position_anchored_overlay(anchor, bubble_size, viewport, options);
            created->set_position(position.rect.get_top_left());
            portal_bubble_ = created;
            portal_handle_ =
                std::make_unique<scene::OverlayHandle>(host->present(std::move(created)));
            portal_anchor_ = anchor;
            portal_viewport_ = viewport_size;
            portal_placement_ = placement_;
            return;
        }

        // Nothing that affects the anchor moved: skip re-shaping the text and
        // re-running the positioner.
        if (anchor == portal_anchor_ && viewport_size == portal_viewport_
            && placement_ == portal_placement_)
        {
            return;
        }
        const auto bubble_size = bubble->measure_layout(scene::LayoutConstraints::loose());
        if (!bubble_size.is_valid()) {
            return;
        }
        const auto position =
            internal::position_anchored_overlay(anchor, bubble_size, viewport, options);
        bubble->set_position(position.rect.get_top_left());
        portal_anchor_ = anchor;
        portal_viewport_ = viewport_size;
        portal_placement_ = placement_;
    }

    void Tooltip::refresh_portal() {
        if (!visible_) {
            return;
        }
        close_portal();
        sync_portal();
    }

    void Tooltip::close_portal() {
        if (portal_handle_ != nullptr) {
            portal_handle_->close();
            portal_handle_.reset();
        }
        portal_bubble_.reset();
        portal_anchor_ = foundation::NanRect {};
        portal_viewport_ = foundation::NanSize {};
        portal_placement_ = Placement::top;
    }

    auto Tooltip::placement_options() const -> internal::AnchoredPositionOptions {
        return {
            .placement = placement_ == Placement::top ? internal::OverlayPlacement::top
                                                       : internal::OverlayPlacement::bottom,
            .gap = resolved_style().metrics.gap,
        };
    }

    void Tooltip::set_overlay_service(scene::OverlayHost* host) noexcept {
        overlay_service_ = host;
    }

    auto Tooltip::resolve_overlay_host() -> scene::OverlayHost* {
        if (overlay_service_ != nullptr) {
            return overlay_service_;
        }
        // `Tooltip::create()` has no BuildContext, so fall back to the nearest
        // ancestor OverlayHost (the window installs one as the tree root). The
        // explicit `as_overlay_host()` accessor keeps this RTTI-free: a plain
        // LayerStack returns nullptr and the walk continues upward.
        for (auto* node = parent(); node != nullptr; node = node->parent()) {
            auto* node_2d = node->as_node2d();
            auto* stack = node_2d != nullptr ? node_2d->as_layer_stack() : nullptr;
            if (stack == nullptr) {
                continue;
            }
            if (auto* host = stack->as_overlay_host(); host != nullptr) {
                return host;
            }
        }
        return nullptr;
    }

    void Tooltip::on_draw(render::DrawContext& context) {
        // With a portal the bubble belongs to the overlay layer, which paints it
        // above application content; the tooltip only draws when detached.
        if (!visible_ || portal_handle_) {
            return;
        }
        const auto style = resolved_style();
        apply_text_style();
        (void)text_.measure_layout(scene::LayoutConstraints::loose());

        const float bubble_w = text_.measured_text_width() + style.metrics.padding_x * 2.0F;
        const float bubble_h = style.metrics.min_height;
        const float bubble_x = (width() - bubble_w) * 0.5F;
        const float bubble_y = placement_ == Placement::top
            ? -(style.metrics.gap + bubble_h)
            : height() + style.metrics.gap;

        const auto world = render::world_bounds_from_local(
            context.world_transform(),
            foundation::NanRect::from_xywh(bubble_x, bubble_y, bubble_w, bubble_h)
        );
        paint_tooltip_bubble(context, world, style, text_);
    }

    auto Tooltip::on_measure(const scene::LayoutConstraints constraints)
        -> foundation::NanSize {
        auto trigger = trigger_.lock();
        if (!trigger) {
            return constraints.constrain(foundation::NanSize(0.0F, 0.0F));
        }
        return constraints.constrain(trigger->measure_layout(constraints));
    }

    auto Tooltip::on_layout() -> void {
        auto trigger = trigger_.lock();
        if (!trigger) {
            return;
        }
        trigger->layout_to(local_rect());
    }

    auto Tooltip::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::tooltip,
            .label = std::string(text()),
        };
    }

    auto Tooltip::apply_text_style() -> primitives::TextStyle {
        const auto style = resolved_style();
        const auto& context = resolved_style_context();
        const primitives::TextStyle text_style {
            .color = context.text_color_from_context ? context.text_color : style.label.color,
            .font_size =
                context.font_size_from_context ? context.font_size : style.label.font_size,
            .font = context.font_from_context ? context.font : text_.font(),
            .overflow = primitives::TextOverflow::clip,
            .max_lines = 1,
        };
        if (!same_text_style(text_.style(), text_style)) {
            text_.set_style(text_style);
        }
        return text_style;
    }
} // namespace nandina::widget
