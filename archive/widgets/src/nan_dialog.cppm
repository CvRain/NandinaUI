module;

#include <algorithm>
#include <functional>
#include <memory>
#include <utility>

#include <thorvg-1/thorvg.h>

export module nandina.widgets.dialog;

import nandina.foundation.color;
import nandina.foundation.nan_rect;
import nandina.reactive.event_signal;
import nandina.runtime.nan_event;
import nandina.runtime.nan_widget;
import nandina.theme.nan_style;
import nandina.widgets.surface;

export namespace nandina::widgets {

    class Dialog final : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<Dialog>;

        ~Dialog() override = default;

        static auto create() -> Ptr {
            return Ptr{new Dialog()};
        }

        auto trigger(runtime::NanWidget::Ptr widget) -> Dialog& {
            if (m_trigger) {
                m_trigger->set_visible(false);
                m_trigger->set_hit_test_visible(false);
            }
            m_trigger = add_child(std::move(widget));
            if (m_trigger) {
                m_trigger->set_hit_test_visible(false);
            }
            sync_open_state();
            mark_layout_dirty();
            return *this;
        }

        auto content(runtime::NanWidget::Ptr widget) -> Dialog& {
            m_panel_surface->clear_children();
            m_content = m_panel_surface->add_child(std::move(widget));
            mark_layout_dirty();
            return *this;
        }

        auto open(const bool value) -> Dialog& {
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

        auto close_on_backdrop(const bool value) -> Dialog& {
            m_close_on_backdrop = value;
            return *this;
        }

        [[nodiscard]] auto close_on_backdrop() const noexcept -> bool {
            return m_close_on_backdrop;
        }

        auto close_on_escape(const bool value) -> Dialog& {
            m_close_on_escape = value;
            return *this;
        }

        [[nodiscard]] auto close_on_escape() const noexcept -> bool {
            return m_close_on_escape;
        }

        auto on_open_changed(std::function<void(bool)> callback) -> Dialog& {
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
            const auto& style = theme::NanStylePrimitives::current().dialog;

            return {
                std::max(trigger_size.width(), std::min(style.max_width, panel_size.width())),
                std::max(trigger_size.height(), std::min(style.max_height, panel_size.height())),
            };
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
            if (m_trigger) {
                const auto trigger_size = measured_or_preferred(*m_trigger);
                m_trigger->set_bounds(x(), y(), trigger_size.width(), trigger_size.height());
                if (m_trigger->visible()) {
                    m_trigger->layout();
                }
            }

            if (m_panel_surface) {
                const auto& style = theme::NanStylePrimitives::current().dialog;
                const auto preferred = measured_or_preferred(*m_panel_surface);
                const float panel_w = std::min(std::max(0.0f, width()), std::min(style.max_width, preferred.width()));
                const float panel_h = std::min(std::max(0.0f, height()), std::min(style.max_height, preferred.height()));
                const float panel_x = x() + (width() - panel_w) * 0.5f;
                const float panel_y = y() + (height() - panel_h) * 0.5f;
                m_panel_surface->set_bounds(panel_x, panel_y, panel_w, panel_h);
                if (m_panel_surface->visible()) {
                    m_panel_surface->layout();
                }
            }

            clear_layout_dirty();
        }

        [[nodiscard]] auto is_interactive() const noexcept -> bool override {
            return true;
        }

    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            if (!m_open) {
                return;
            }

            const auto backdrop = theme::NanStylePrimitives::current().dialog.backdrop.to<NanRgb>();
            auto* shape = tvg::Shape::gen();
            shape->appendRect(x(), y(), width(), height(), 0.0f, 0.0f);
            shape->fill(backdrop.red(), backdrop.green(), backdrop.blue(), backdrop.alpha());
            canvas.add(shape);
        }

        auto on_pointer_down(const runtime::PointerButtonEvent& event) -> bool override {
            if (event.button != nandina::types::PointerButton::Left) {
                return false;
            }
            if (!m_open) {
                if (trigger_contains(event.x, event.y)) {
                    m_pressed_trigger = true;
                    return true;
                }
                return false;
            }

            if (!panel_contains(event.x, event.y)) {
                m_pressed_backdrop = true;
                return true;
            }
            return false;
        }

        auto on_pointer_up(const runtime::PointerButtonEvent& event) -> bool override {
            if (event.button != nandina::types::PointerButton::Left) {
                return false;
            }

            if (!m_open) {
                const bool was_pressed = m_pressed_trigger;
                m_pressed_trigger = false;
                if (was_pressed && trigger_contains(event.x, event.y)) {
                    open(true);
                    return true;
                }
                return false;
            }

            const bool was_backdrop = m_pressed_backdrop;
            m_pressed_backdrop = false;
            if (was_backdrop && m_close_on_backdrop && !panel_contains(event.x, event.y)) {
                open(false);
                return true;
            }
            return false;
        }

        auto on_key_down(const runtime::KeyEvent& event) -> bool override {
            if (!m_open || !m_close_on_escape || event.key_code != 27) {
                return false;
            }
            open(false);
            return true;
        }

    private:
        Dialog() {
            const auto& style = theme::NanStylePrimitives::current().dialog;
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

        [[nodiscard]] static auto measured_or_preferred(const runtime::NanWidget& widget) noexcept -> geometry::NanSize {
            const auto measured = widget.measured_size();
            if (measured.width() > 0.0f || measured.height() > 0.0f) {
                return measured;
            }
            return widget.preferred_size();
        }

        auto sync_open_state() -> void {
            if (m_trigger) {
                m_trigger->set_visible(!m_open);
            }
            if (m_panel_surface) {
                m_panel_surface->set_visible(m_open);
                m_panel_surface->set_hit_test_visible(m_open);
            }
        }

        [[nodiscard]] auto trigger_contains(const float px, const float py) const noexcept -> bool {
            return m_trigger && m_trigger->bounds().contains(geometry::NanPoint{px, py});
        }

        [[nodiscard]] auto panel_contains(const float px, const float py) const noexcept -> bool {
            return m_panel_surface && m_panel_surface->bounds().contains(geometry::NanPoint{px, py});
        }

        runtime::NanWidget* m_trigger{nullptr};
        runtime::NanWidget* m_content{nullptr};
        Surface* m_panel_surface{nullptr};
        bool m_open{false};
        bool m_close_on_backdrop{true};
        bool m_close_on_escape{true};
        bool m_pressed_trigger{false};
        bool m_pressed_backdrop{false};
        reactive::EventSignal<bool> changed_signal_;
    };
}
