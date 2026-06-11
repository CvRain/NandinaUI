module;

#include <algorithm>
#include <functional>
#include <memory>
#include <utility>

#include <thorvg-1/thorvg.h>

export module nandina.widgets.popover;

import nandina.foundation.color;
import nandina.foundation.nan_insets;
import nandina.foundation.nan_rect;
import nandina.reactive.event_signal;
import nandina.runtime.nan_event;
import nandina.runtime.nan_widget;
import nandina.theme.nan_style;
import nandina.widgets.surface;

export namespace nandina::widgets {

    using PopoverPlacement = nandina::theme::PopoverPlacement;

    class Popover : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<Popover>;

        ~Popover() override = default;

        static auto create() -> Ptr {
            return Ptr{new Popover()};
        }

        auto trigger(runtime::NanWidget::Ptr widget) -> Popover& {
            if (m_trigger) {
                m_trigger->set_hit_test_visible(false);
                m_trigger->set_visible(false);
            }
            m_trigger = add_child(std::move(widget));
            if (m_trigger) {
                m_trigger->set_hit_test_visible(false);
            }
            mark_layout_dirty();
            return *this;
        }

        auto content(runtime::NanWidget::Ptr widget) -> Popover& {
            m_panel_surface->clear_children();
            m_content = m_panel_surface->add_child(std::move(widget));
            mark_layout_dirty();
            return *this;
        }

        auto open(const bool value) -> Popover& {
            if (m_open == value) {
                return *this;
            }
            m_open = value;
            sync_open_state();
            changed_signal_.emit(m_open);
            mark_layout_dirty();
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto open() const noexcept -> bool {
            return m_open;
        }

        auto placement(const PopoverPlacement value) -> Popover& {
            m_placement = value;
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto placement() const noexcept -> PopoverPlacement {
            return m_placement;
        }

        auto gap(const float value) -> Popover& {
            m_gap = std::max(0.0f, value);
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto gap() const noexcept -> float {
            return m_gap;
        }

        auto on_open_changed(std::function<void(bool)> callback) -> Popover& {
            changed_signal_.connect(std::move(callback));
            return *this;
        }

        [[nodiscard]] auto trigger_bounds() const noexcept -> geometry::NanRect {
            return m_trigger ? m_trigger->bounds() : geometry::NanRect{};
        }

        [[nodiscard]] auto panel_bounds() const noexcept -> geometry::NanRect {
            return m_panel_surface ? m_panel_surface->bounds() : geometry::NanRect{};
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            const auto trigger_size = m_trigger ? measured_or_preferred(*m_trigger) : geometry::NanSize{};
            const auto panel_size = m_panel_surface ? measured_or_preferred(*m_panel_surface) : geometry::NanSize{};

            switch (m_placement) {
            case PopoverPlacement::top:
            case PopoverPlacement::bottom:
                return {
                    std::max(trigger_size.width(), panel_size.width()),
                    trigger_size.height() + (panel_size.height() > 0.0f ? m_gap + panel_size.height() : 0.0f),
                };
            case PopoverPlacement::left:
            case PopoverPlacement::right:
                return {
                    trigger_size.width() + (panel_size.width() > 0.0f ? m_gap + panel_size.width() : 0.0f),
                    std::max(trigger_size.height(), panel_size.height()),
                };
            }

            return trigger_size;
        }

        auto measure(const geometry::NanConstraints& constraints) -> void override {
            if (m_trigger) {
                m_trigger->measure(constraints.loosen());
            }
            if (m_panel_surface) {
                m_panel_surface->measure(constraints.loosen());
            }
            set_measured_layout_state(constraints, constraints.constrain(preferred_size()));
        }

        auto layout() -> void override {
            if (!m_trigger) {
                clear_layout_dirty();
                return;
            }

            const auto trigger_pref = measured_or_preferred(*m_trigger);
            const auto panel_pref = m_panel_surface ? measured_or_preferred(*m_panel_surface) : geometry::NanSize{};
            const auto trigger_pos = resolve_trigger_origin(trigger_pref, panel_pref);

            m_trigger->set_bounds(trigger_pos.x(), trigger_pos.y(), trigger_pref.width(), trigger_pref.height());
            m_trigger->layout();

            if (m_panel_surface) {
                const auto trigger_rect = m_trigger->bounds();
                const auto popup_pos = resolve_panel_origin(trigger_rect, panel_pref);
                m_panel_surface->set_bounds(popup_pos.x(), popup_pos.y(), panel_pref.width(), panel_pref.height());
                if (m_open) {
                    m_panel_surface->layout();
                }
            }

            clear_layout_dirty();
        }

        [[nodiscard]] auto is_interactive() const noexcept -> bool override {
            return true;
        }

    protected:
        auto on_pointer_move(const runtime::PointerMoveEvent& event) -> bool override {
            const bool inside = trigger_contains(event.x, event.y);
            if (!inside) {
                return false;
            }
            m_pointer_inside_trigger = true;
            return true;
        }

        auto on_pointer_leave(const runtime::PointerMoveEvent&) -> bool override {
            m_pointer_inside_trigger = false;
            return false;
        }

        auto on_pointer_down(const runtime::PointerButtonEvent& event) -> bool override {
            if (event.button != nandina::types::PointerButton::Left || !trigger_contains(event.x, event.y)) {
                return false;
            }
            m_pressed = true;
            return true;
        }

        auto on_pointer_up(const runtime::PointerButtonEvent& event) -> bool override {
            if (event.button != nandina::types::PointerButton::Left) {
                return false;
            }
            const bool was_pressed = m_pressed;
            m_pressed = false;
            if (was_pressed && trigger_contains(event.x, event.y)) {
                open(!m_open);
                return true;
            }
            return false;
        }

    protected:
        Popover() {
            const auto& style = theme::NanStylePrimitives::current().popover;
            m_gap = style.gap;
            m_placement = style.placement;

            auto panel = Surface::create();
            panel->set_bg_color(style.panel_bg)
                .set_corner_radius(style.corner_radius)
                .set_border_color(style.border)
                .set_border_width(style.border_width)
                .set_padding(style.panel_padding);
            m_panel_surface = panel.get();
            add_child(std::move(panel));
            sync_open_state();
        }

        [[nodiscard]] auto panel_surface() noexcept -> Surface& {
            return *m_panel_surface;
        }

    private:
        [[nodiscard]] static auto measured_or_preferred(const runtime::NanWidget& widget) noexcept -> geometry::NanSize {
            const auto ms = widget.measured_size();
            if (ms.width() > 0.0f || ms.height() > 0.0f) {
                return ms;
            }
            return widget.preferred_size();
        }

        auto sync_open_state() -> void {
            if (!m_panel_surface) {
                return;
            }
            m_panel_surface->set_visible(m_open);
            m_panel_surface->set_hit_test_visible(m_open);
        }

        [[nodiscard]] auto trigger_contains(const float px, const float py) const noexcept -> bool {
            return m_trigger && m_trigger->bounds().contains(geometry::NanPoint{px, py});
        }

        [[nodiscard]] auto resolve_panel_origin(
            const geometry::NanRect& trigger_rect,
            const geometry::NanSize& panel_size) const noexcept -> geometry::NanPoint {
            float panel_x = trigger_rect.x();
            float panel_y = trigger_rect.bottom() + m_gap;

            switch (m_placement) {
            case PopoverPlacement::bottom:
                panel_x = trigger_rect.x();
                panel_y = trigger_rect.bottom() + m_gap;
                break;
            case PopoverPlacement::top:
                panel_x = trigger_rect.x();
                panel_y = trigger_rect.y() - m_gap - panel_size.height();
                break;
            case PopoverPlacement::right:
                panel_x = trigger_rect.right() + m_gap;
                panel_y = trigger_rect.y();
                break;
            case PopoverPlacement::left:
                panel_x = trigger_rect.x() - m_gap - panel_size.width();
                panel_y = trigger_rect.y();
                break;
            }

            panel_x = std::clamp(panel_x, x(), std::max(x(), right() - panel_size.width()));
            panel_y = std::clamp(panel_y, y(), std::max(y(), bottom() - panel_size.height()));
            return {panel_x, panel_y};
        }

        [[nodiscard]] auto resolve_trigger_origin(
            const geometry::NanSize& trigger_size,
            const geometry::NanSize& panel_size) const noexcept -> geometry::NanPoint {
            float trigger_x = x();
            float trigger_y = y();

            switch (m_placement) {
            case PopoverPlacement::bottom:
                trigger_x = x();
                trigger_y = y();
                break;
            case PopoverPlacement::top:
                trigger_x = x();
                trigger_y = y() + panel_size.height() + m_gap;
                break;
            case PopoverPlacement::right:
                trigger_x = x();
                trigger_y = y();
                break;
            case PopoverPlacement::left:
                trigger_x = x() + panel_size.width() + m_gap;
                trigger_y = y();
                break;
            }

            trigger_x = std::clamp(trigger_x, x(), std::max(x(), right() - trigger_size.width()));
            trigger_y = std::clamp(trigger_y, y(), std::max(y(), bottom() - trigger_size.height()));
            return {trigger_x, trigger_y};
        }

        [[nodiscard]] auto right() const noexcept -> float {
            return x() + width();
        }

        [[nodiscard]] auto bottom() const noexcept -> float {
            return y() + height();
        }

        runtime::NanWidget* m_trigger{nullptr};
        runtime::NanWidget* m_content{nullptr};
        Surface* m_panel_surface{nullptr};
        bool m_open{false};
        bool m_pressed{false};
        bool m_pointer_inside_trigger{false};
        float m_gap{10.0f};
        PopoverPlacement m_placement{PopoverPlacement::bottom};
        reactive::EventSignal<bool> changed_signal_;
    };
}
