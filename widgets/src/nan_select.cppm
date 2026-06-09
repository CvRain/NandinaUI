module;

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

export module nandina.widgets.select;

import nandina.foundation.nan_rect;
import nandina.layout.container;
import nandina.reactive.event_signal;
import nandina.runtime.nan_event;
import nandina.runtime.nan_widget;
import nandina.theme.nan_style;
import nandina.widgets.button;
import nandina.widgets.surface;

export namespace nandina::widgets {

    class Select final : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<Select>;

        ~Select() override = default;

        static auto create() -> Ptr {
            return Ptr{new Select()};
        }

        auto add_option(std::string label, std::string value = {}) -> Select& {
            const std::size_t index = options_.size();
            auto option_button = Button::create();
            auto* option_ptr = option_button.get();
            option_ptr->text(label);
            option_ptr->on_click([this, index] {
                selected_index(index);
                open(false);
            });
            m_options_column->add(std::move(option_button));

            if (value.empty()) {
                value = label;
            }
            options_.push_back(Option{
                .label = std::move(label),
                .value = std::move(value),
                .button = option_ptr,
            });

            if (!m_has_selection) {
                m_has_selection = true;
                m_selected_index = 0;
            }
            sync_visual_state();
            mark_layout_dirty();
            return *this;
        }

        auto selected_index(const std::size_t index) -> Select& {
            if (options_.empty()) {
                return *this;
            }
            const auto next = std::min(index, options_.size() - 1);
            if (m_has_selection && m_selected_index == next) {
                return *this;
            }
            m_has_selection = true;
            m_selected_index = next;
            sync_visual_state();
            changed_signal_.emit(m_selected_index);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto selected_index() const noexcept -> std::size_t {
            return m_selected_index;
        }

        [[nodiscard]] auto has_selection() const noexcept -> bool {
            return m_has_selection;
        }

        [[nodiscard]] auto selected_label() const noexcept -> std::string_view {
            if (!m_has_selection || m_selected_index >= options_.size()) {
                return {};
            }
            return options_[m_selected_index].label;
        }

        [[nodiscard]] auto selected_value() const noexcept -> std::string_view {
            if (!m_has_selection || m_selected_index >= options_.size()) {
                return {};
            }
            return options_[m_selected_index].value;
        }

        [[nodiscard]] auto option_count() const noexcept -> std::size_t {
            return options_.size();
        }

        auto open(const bool value) -> Select& {
            if (m_open == value) {
                return *this;
            }
            m_open = value;
            sync_open_state();
            mark_layout_dirty();
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto open() const noexcept -> bool {
            return m_open;
        }

        auto on_changed(std::function<void(std::size_t)> callback) -> Select& {
            changed_signal_.connect(std::move(callback));
            return *this;
        }

        [[nodiscard]] auto trigger_bounds() const noexcept -> geometry::NanRect {
            return m_trigger ? m_trigger->bounds() : geometry::NanRect{};
        }

        [[nodiscard]] auto panel_bounds() const noexcept -> geometry::NanRect {
            return m_panel ? m_panel->bounds() : geometry::NanRect{};
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            const auto& style = theme::NanStylePrimitives::current().select;
            const auto trigger_size = m_trigger ? measured_or_preferred(*m_trigger) : geometry::NanSize{};
            const auto panel_size = m_panel ? measured_or_preferred(*m_panel) : geometry::NanSize{};
            return {
                std::max(style.min_width, std::max(trigger_size.width(), panel_size.width())),
                trigger_size.height() + style.gap + panel_size.height(),
            };
        }

        auto measure(const geometry::NanConstraints& constraints) -> void override {
            if (m_trigger) {
                m_trigger->measure(constraints.loosen());
            }
            if (m_panel) {
                m_panel->measure(constraints.loosen());
            }
            set_measured_layout_state(constraints, constraints.constrain(preferred_size()));
        }

        auto layout() -> void override {
            const auto& style = theme::NanStylePrimitives::current().select;
            const auto trigger_size = measured_or_preferred(*m_trigger);
            const auto panel_size = measured_or_preferred(*m_panel);
            const float resolved_w = std::max(style.min_width, std::max(width(), std::max(trigger_size.width(), panel_size.width())));

            m_trigger->set_bounds(x(), y(), resolved_w, trigger_size.height());
            m_trigger->layout();

            m_panel->set_bounds(x(), y() + trigger_size.height() + style.gap, resolved_w, panel_size.height());
            if (m_panel->visible()) {
                m_panel->layout();
            }

            clear_layout_dirty();
        }

        [[nodiscard]] auto is_interactive() const noexcept -> bool override {
            return true;
        }

    protected:
        auto on_pointer_down(const runtime::PointerButtonEvent& event) -> bool override {
            if (event.button != nandina::types::PointerButton::Left) {
                return false;
            }
            if (trigger_contains(event.x, event.y)) {
                m_pressed_trigger = true;
                return true;
            }
            if (m_open && !panel_contains(event.x, event.y)) {
                open(false);
                return true;
            }
            return false;
        }

        auto on_pointer_up(const runtime::PointerButtonEvent& event) -> bool override {
            if (event.button != nandina::types::PointerButton::Left) {
                return false;
            }
            const bool was_pressed = m_pressed_trigger;
            m_pressed_trigger = false;
            if (was_pressed && trigger_contains(event.x, event.y)) {
                open(!m_open);
                return true;
            }
            return false;
        }

        auto on_key_down(const runtime::KeyEvent& event) -> bool override {
            if (!m_open || event.key_code != 27) {
                return false;
            }
            open(false);
            return true;
        }

    private:
        struct Option {
            std::string label;
            std::string value;
            Button* button{nullptr};
        };

        Select() {
            const auto& style = theme::NanStylePrimitives::current().select;

            auto trigger = Button::create();
            m_trigger = trigger.get();
            m_trigger->text("Select...")
                .size(style.trigger_size)
                .variant(style.trigger_variant)
                .color_variant(style.trigger_color_variant);
            m_trigger->set_hit_test_visible(false);
            add_child(std::move(trigger));

            auto panel = Surface::create();
            panel->set_bg_color(style.panel_bg)
                .set_corner_radius(style.corner_radius)
                .set_border_color(style.border)
                .set_border_width(style.border_width)
                .set_padding(style.panel_padding);
            m_panel = panel.get();

            auto column = layout::Column::Create();
            m_options_column = column.get();
            m_options_column->gap(style.option_gap).align_items(layout::LayoutAlignment::stretch);
            m_panel->add_child(std::move(column));
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
            if (!m_panel) {
                return;
            }
            m_panel->set_visible(m_open);
            m_panel->set_hit_test_visible(m_open);
        }

        auto sync_visual_state() -> void {
            const auto& style = theme::NanStylePrimitives::current().select;
            if (m_trigger) {
                const std::string_view label = m_has_selection ? std::string_view{options_[m_selected_index].label} : std::string_view{"Select..."};
                m_trigger->text(label);
            }
            for (std::size_t i = 0; i < options_.size(); ++i) {
                auto* button = options_[i].button;
                if (!button) {
                    continue;
                }
                const bool selected = m_has_selection && i == m_selected_index;
                button->size(style.option_size)
                    .variant(selected ? style.selected_option_variant : style.option_variant)
                    .color_variant(selected ? style.selected_option_color_variant : style.option_color_variant);
            }
        }

        [[nodiscard]] auto trigger_contains(const float px, const float py) const noexcept -> bool {
            return m_trigger && m_trigger->bounds().contains(geometry::NanPoint{px, py});
        }

        [[nodiscard]] auto panel_contains(const float px, const float py) const noexcept -> bool {
            return m_panel && m_panel->bounds().contains(geometry::NanPoint{px, py});
        }

        Button* m_trigger{nullptr};
        Surface* m_panel{nullptr};
        layout::Column* m_options_column{nullptr};
        std::vector<Option> options_{};
        std::size_t m_selected_index{0};
        bool m_has_selection{false};
        bool m_open{false};
        bool m_pressed_trigger{false};
        reactive::EventSignal<std::size_t> changed_signal_;
    };
}
