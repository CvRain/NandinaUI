module;

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <algorithm>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.switch_widget;

import nandina.widgets.surface;
import nandina.widgets.text;
import nandina.widgets.focus_ring;
import nandina.widgets.pressable;
import nandina.foundation.nan_insets;
import nandina.foundation.color;
import nandina.reactive.event_signal;
import nandina.runtime.nan_event;
import nandina.runtime.nan_widget;
import nandina.text.nan_font;
import nandina.theme.nan_style;

export namespace nandina::widgets {
    using SwitchColorVariant = nandina::theme::ColorVariant;
    using SwitchSize = nandina::theme::SwitchSize;

    class Switch : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<Switch>;
        using Callback = std::function<void()>;

        ~Switch() override = default;

        static auto create() -> Ptr {
            return Ptr{new Switch()};
        }

        auto label(std::string_view text) -> Switch& {
            m_label_widget->set_text(text);
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto label() const noexcept -> const std::string& {
            return m_label_widget->text();
        }

        auto checked(bool value) -> Switch& {
            if (m_checked == value) return *this;
            m_checked = value;
            m_checked_changed_signal.emit(value);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto checked() const noexcept -> bool {
            return m_checked;
        }

        auto size(SwitchSize value) -> Switch& {
            if (m_size == value) return *this;
            m_size = value;
            apply_size_style();
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto size() const noexcept -> SwitchSize {
            return m_size;
        }

        auto color_variant(SwitchColorVariant value) -> Switch& {
            if (m_color_variant == value) return *this;
            m_color_variant = value;
            sync_visual_state();
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto color_variant() const noexcept -> SwitchColorVariant {
            return m_color_variant;
        }

        auto disabled(bool value) -> Switch& {
            if (m_disabled == value) return *this;
            m_disabled = value;
            if (m_pressable) {
                m_pressable->set_disabled(value);
                sync_pressable_state();
            }
            if (m_disabled) {
                if (m_focus_ring) m_focus_ring->set_active(false);
            }
            sync_visual_state();
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto disabled() const noexcept -> bool {
            return m_disabled;
        }

        auto on_checked_changed(std::function<void(bool)> cb) -> Switch& {
            m_checked_changed_signal.connect(std::move(cb));
            return *this;
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            const auto &size_style = size_style_for(m_size);
            const float label_w = m_label_widget->text().empty()
                                      ? 0.0f
                                      : m_label_widget->preferred_size().width();
            return {size_style.track_width + size_style.gap + label_w, size_style.thumb_size};
        }

        auto layout() -> void override {
            const auto &size_style = size_style_for(m_size);
            const float thumb_sz = size_style.thumb_size;
            const float gap = size_style.gap;
            const float cy = (height() - thumb_sz) * 0.5f;

            if (m_label_widget) {
                const float label_x = size_style.track_width + gap;
                const float label_w = std::max(0.0f, width() - label_x);
                m_label_widget->set_bounds(x() + label_x, y() + cy, label_w, thumb_sz);
            }

            if (m_focus_ring) {
                m_focus_ring->set_bounds(x(), y(), width(), height());
            }

            clear_layout_dirty();
        }

        [[nodiscard]] auto is_interactive() const noexcept -> bool override {
            return true;
        }

    protected:
        auto on_pointer_move(const runtime::PointerMoveEvent &event) -> bool override {
            if (m_disabled || !m_pressable) return false;
            return m_pressable->dispatch_event(event);
        }

        auto on_pointer_leave(const runtime::PointerMoveEvent &) -> bool override {
            if (m_disabled || !m_pressable) return false;
            return m_pressable->dispatch_pointer_leave(runtime::PointerMoveEvent{});
        }

        auto on_pointer_down(const runtime::PointerButtonEvent &event) -> bool override {
            if (m_disabled || !m_pressable) return false;
            return m_pressable->dispatch_event(event, runtime::EventType::PointerDown);
        }

        auto on_pointer_up(const runtime::PointerButtonEvent &event) -> bool override {
            if (m_disabled || !m_pressable) return false;
            return m_pressable->dispatch_event(event, runtime::EventType::PointerUp);
        }

        auto on_focus_in() -> bool override {
            if (m_disabled || !m_pressable) return false;
            const bool handled = m_pressable->dispatch_event(runtime::FocusEvent{.got_focus = true});
            sync_pressable_state();
            return handled;
        }

        auto on_focus_out() -> bool override {
            if (!m_pressable) return false;
            const bool handled = m_pressable->dispatch_event(runtime::FocusEvent{.got_focus = false});
            sync_pressable_state();
            return handled;
        }

        void on_draw(tvg::SwCanvas &canvas) override {
            const auto &size_style = size_style_for(m_size);
            const auto rect = bounds();
            const float track_w = size_style.track_width;
            const float track_h = size_style.track_height;
            const float thumb_sz = size_style.thumb_size;
            const float radius = size_style.corner_radius;
            const float track_x = rect.x();
            const float track_y = rect.y() + (rect.height() - track_h) * 0.5f;
            const float thumb_r = thumb_sz * 0.5f;

            const auto colors = resolved_colors();

            // ── track ──
            {
                const auto track_color = m_checked ? colors.track_on : colors.track_off;
                const auto track_rgb = track_color.to<nandina::NanRgb>();
                auto *shape = tvg::Shape::gen();
                shape->appendRect(track_x, track_y, track_w, track_h, radius, radius);
                shape->fill(track_rgb.red(), track_rgb.green(), track_rgb.blue(), track_rgb.alpha());
                canvas.add(shape);
            }

            // ── thumb ──
            {
                const auto thumb_color = colors.thumb;
                const auto thumb_rgb = thumb_color.to<nandina::NanRgb>();
                const float thumb_cx = track_x + (m_checked ? track_w - thumb_r : thumb_r);
                const float thumb_cy = track_y + track_h * 0.5f;

                auto *shape = tvg::Shape::gen();
                shape->appendCircle(thumb_cx, thumb_cy, thumb_r, thumb_r);
                shape->fill(thumb_rgb.red(), thumb_rgb.green(), thumb_rgb.blue(), thumb_rgb.alpha());
                canvas.add(shape);
            }

            // ── children (label, focus ring) ──
            for (auto &child: children()) {
                if (child.get() != m_label_widget && child.get() != m_focus_ring) continue;
                child->draw(canvas);
            }
        }

    private:
        struct ResolvedColors {
            nandina::NanColor track_on;
            nandina::NanColor track_off;
            nandina::NanColor thumb;
        };

        Switch() {
            const auto &style = nandina::theme::NanStylePrimitives::current().switch_style;
            m_color_variant = style.color_variant;
            m_size = style.size;

            auto label = Text::create();
            label->set_align(TextAlign::Start);
            label->set_vertical_align(TextVerticalAlign::Center);
            label->set_font(text::NanFont{}
                .size(style.font_size)
                .single_line(true)
                .overflow(text::TextOverflow::ellipsis));
            m_label_widget = label.get();
            add_child(std::move(label));

            auto focus_ring = FocusRing::create();
            m_focus_ring = focus_ring.get();
            m_focus_ring->set_active(false);
            add_child(std::move(focus_ring));

            m_pressable = Pressable::create();
            m_pressable->on_hover([this] {
                sync_pressable_state();
                mark_dirty();
            });
            m_pressable->on_leave([this] {
                sync_pressable_state();
                mark_dirty();
            });
            m_pressable->on_press([this] {
                sync_pressable_state();
                mark_dirty();
            });
            m_pressable->on_release([this] {
                sync_pressable_state();
                mark_dirty();
            });
            m_pressable->on_click([this] {
                checked(!m_checked);
            });

            apply_size_style();
            sync_visual_state();
        }

        static auto size_style_for(SwitchSize s) -> const nandina::theme::NanSwitchStyle::SizeStyle& {
            const auto &style = nandina::theme::NanStylePrimitives::current().switch_style;
            switch (s) {
                case SwitchSize::sm: return style.sm;
                case SwitchSize::md: return style.md;
            }
            return style.md;
        }

        auto apply_size_style() -> void {
            const auto &size_style = size_style_for(m_size);
            m_label_widget->update_font([&](text::NanFont &font) {
                font.size(size_style.font_size);
            });
        }

        [[nodiscard]] auto resolved_colors() const -> ResolvedColors {
            const auto &style = nandina::theme::NanStylePrimitives::current().switch_style;
            const auto resolved_variant = m_color_variant == SwitchColorVariant::inherit
                                              ? SwitchColorVariant::primary
                                              : m_color_variant;

            const auto make_colors = [this, &style](const nandina::theme::NanSwitchStyle::ColorFamilyStyle &family) {
                if (m_disabled) {
                    return ResolvedColors{
                        .track_on = family.track_on_disabled,
                        .track_off = style.track_off_disabled,
                        .thumb = family.thumb_disabled,
                    };
                }
                return ResolvedColors{
                    .track_on = family.track_on,
                    .track_off = style.track_off,
                    .thumb = family.thumb,
                };
            };

            switch (resolved_variant) {
                case SwitchColorVariant::secondary: return make_colors(style.secondary_family);
                case SwitchColorVariant::neutral: return make_colors(style.neutral_family);
                case SwitchColorVariant::destructive: return make_colors(style.destructive_family);
                case SwitchColorVariant::primary:
                case SwitchColorVariant::inherit: break;
            }

            if (m_disabled) {
                return ResolvedColors{
                    .track_on = style.track_on_disabled,
                    .track_off = style.track_off_disabled,
                    .thumb = style.thumb_disabled,
                };
            }
            return ResolvedColors{
                .track_on = style.track_on,
                .track_off = style.track_off,
                .thumb = style.thumb,
            };
        }

        auto sync_visual_state() -> void {
        }

        auto sync_pressable_state() -> void {
            if (!m_pressable) {
                return;
            }
            const auto state = m_pressable->state();
            m_hovered = state.hovered;
            m_pressed = state.pressed;
            if (m_focus_ring) {
                m_focus_ring->set_active(state.focused && !m_disabled);
            }
        }

        Text *m_label_widget{nullptr};
        FocusRing *m_focus_ring{nullptr};
        Pressable::Ptr m_pressable;
        SwitchSize m_size{SwitchSize::md};
        SwitchColorVariant m_color_variant{SwitchColorVariant::inherit};
        bool m_checked{false};
        bool m_hovered{false};
        bool m_pressed{false};
        bool m_disabled{false};
        reactive::EventSignal<bool> m_checked_changed_signal;
    };
} // namespace nandina::widgets
