module;

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <algorithm>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.checkbox;

import nandina.widgets.surface;
import nandina.widgets.text;
import nandina.widgets.focus_ring;
import nandina.foundation.nan_insets;
import nandina.foundation.color;
import nandina.reactive.event_signal;
import nandina.runtime.nan_event;
import nandina.runtime.nan_widget;
import nandina.text.nan_font;
import nandina.theme.nan_style;

export namespace nandina::widgets {
    using ColorVariant = nandina::theme::ColorVariant;
    using CheckboxSize = nandina::theme::CheckboxSize;

    class Checkbox : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<Checkbox>;
        using Callback = std::function<void()>;

        ~Checkbox() override = default;

        static auto create() -> Ptr {
            return Ptr{new Checkbox()};
        }

        auto label(std::string_view text) -> Checkbox& {
            m_label_widget->set_text(text);
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto label() const noexcept -> const std::string& {
            return m_label_widget->text();
        }

        auto checked(bool value) -> Checkbox& {
            if (m_checked == value) return *this;
            m_checked = value;
            m_checked_changed_signal.emit(value);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto checked() const noexcept -> bool {
            return m_checked;
        }

        auto size(CheckboxSize value) -> Checkbox& {
            if (m_size == value) return *this;
            m_size = value;
            apply_size_style();
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto size() const noexcept -> CheckboxSize {
            return m_size;
        }

        auto color_variant(ColorVariant value) -> Checkbox& {
            if (m_color_variant == value) return *this;
            m_color_variant = value;
            sync_visual_state();
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto color_variant() const noexcept -> ColorVariant {
            return m_color_variant;
        }

        auto disabled(bool value) -> Checkbox& {
            if (m_disabled == value) return *this;
            m_disabled = value;
            if (m_disabled) {
                m_hovered = false;
                m_pressed = false;
                if (m_focus_ring) m_focus_ring->set_active(false);
            }
            sync_visual_state();
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto disabled() const noexcept -> bool {
            return m_disabled;
        }

        auto on_checked_changed(std::function<void(bool)> cb) -> Checkbox& {
            m_checked_changed_signal.connect(std::move(cb));
            return *this;
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            const auto &size_style = size_style_for(m_size);
            const float label_w = m_label_widget->text().empty()
                                      ? 0.0f
                                      : m_label_widget->preferred_size().width();
            return {size_style.box_size + size_style.gap + label_w, size_style.box_size};
        }

        auto layout() -> void override {
            const auto &size_style = size_style_for(m_size);
            const float box_sz = size_style.box_size;
            const float gap = size_style.gap;
            const float box_y = (height() - box_sz) * 0.5f;

            // 将 label 定位到复选框右侧
            if (m_label_widget) {
                const float label_x = box_sz + gap;
                const float label_w = std::max(0.0f, width() - label_x);
                m_label_widget->set_bounds(x() + label_x, y() + box_y, label_w, box_sz);
            }

            // Focus ring 覆盖整个 checkbox 区域
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
            if (m_disabled) return false;
            const auto bnds = bounds();
            if (event.x >= bnds.x() && event.x < bnds.x() + bnds.width() &&
                event.y >= bnds.y() && event.y < bnds.y() + bnds.height()) {
                if (!m_hovered) {
                    m_hovered = true;
                    mark_dirty();
                }
                return true;
            }
            if (m_hovered) {
                m_hovered = false;
                m_pressed = false;
                mark_dirty();
            }
            return false;
        }

        auto on_pointer_leave(const runtime::PointerMoveEvent &) -> bool override {
            if (m_disabled) return false;
            if (m_hovered || m_pressed) {
                m_hovered = false;
                m_pressed = false;
                mark_dirty();
            }
            return true;
        }

        auto on_pointer_down(const runtime::PointerButtonEvent &event) -> bool override {
            if (m_disabled) return false;
            if (event.button != nandina::types::PointerButton::Left) return false;
            m_pressed = true;
            mark_dirty();
            return true;
        }

        auto on_pointer_up(const runtime::PointerButtonEvent &) -> bool override {
            if (m_disabled) return false;
            if (!m_pressed) return false;
            m_pressed = false;
            checked(!m_checked);
            mark_dirty();
            return true;
        }

        auto on_focus_in() -> bool override {
            if (m_disabled) return false;
            if (m_focus_ring) m_focus_ring->set_active(true);
            mark_dirty();
            return true;
        }

        auto on_focus_out() -> bool override {
            if (m_focus_ring) m_focus_ring->set_active(false);
            mark_dirty();
            return true;
        }

        void on_draw(tvg::SwCanvas &canvas) override {
            const auto &size_style = size_style_for(m_size);
            const auto rect = bounds();
            const float box_sz = size_style.box_size;
            const float box_y = rect.y() + (rect.height() - box_sz) * 0.5f;
            const float radius = size_style.corner_radius;

            const auto colors = resolved_colors();

            // ── 背景填充 ──
            {
                const auto bg = (m_checked || m_pressed) ? colors.box_bg : colors.box_bg_unchecked;
                const auto bg_rgb = bg.to<nandina::NanRgb>();
                auto *shape = tvg::Shape::gen();
                shape->appendRect(rect.x(), box_y, box_sz, box_sz, radius, radius);
                shape->fill(bg_rgb.red(), bg_rgb.green(), bg_rgb.blue(), bg_rgb.alpha());
                canvas.add(shape);
            }

            // ── 边框 ──
            {
                const auto border = colors.box_border;
                const auto border_rgb = border.to<nandina::NanRgb>();
                auto *shape = tvg::Shape::gen();
                shape->appendRect(rect.x(), box_y, box_sz, box_sz, radius, radius);
                shape->strokeWidth(1.5f);
                shape->strokeFill(border_rgb.red(), border_rgb.green(), border_rgb.blue(), border_rgb.alpha());
                canvas.add(shape);
            }

            // ── 选中勾号 ──
            if (m_checked) {
                const auto check = colors.check;
                const auto check_rgb = check.to<nandina::NanRgb>();
                const float cx = rect.x() + box_sz * 0.5f;
                const float cy = box_y + box_sz * 0.5f;
                const float hw = box_sz * 0.27f;
                const float hh = box_sz * 0.15f;

                auto *check_shape = tvg::Shape::gen();
                // simple check mark: two line segments forming a ✓
                check_shape->moveTo(cx - hw, cy);
                check_shape->lineTo(cx - hw * 0.3f, cy + hh);
                check_shape->lineTo(cx + hw, cy - hh);
                check_shape->strokeWidth(2.0f);
                check_shape->strokeCap(tvg::StrokeCap::Round);
                check_shape->strokeJoin(tvg::StrokeJoin::Round);
                check_shape->strokeFill(check_rgb.red(), check_rgb.green(), check_rgb.blue(), check_rgb.alpha());
                canvas.add(check_shape);
            }

            // ── 子节点绘制（label, focus ring）──
            for (auto &child: children()) {
                if (child.get() != m_label_widget && child.get() != m_focus_ring) continue;
                child->draw(canvas);
            }
        }

    private:
        struct ResolvedColors {
            nandina::NanColor box_bg;
            nandina::NanColor box_border;
            nandina::NanColor check;
            nandina::NanColor box_bg_unchecked;
            nandina::NanColor box_border_unchecked;
        };

        Checkbox() {
            const auto &style = nandina::theme::NanStylePrimitives::current().checkbox;
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

            apply_size_style();
            sync_visual_state();
        }

        static auto size_style_for(CheckboxSize s) -> const nandina::theme::NanCheckboxStyle::SizeStyle& {
            const auto &style = nandina::theme::NanStylePrimitives::current().checkbox;
            switch (s) {
                case CheckboxSize::sm: return style.sm;
                case CheckboxSize::md: return style.md;
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
            const auto &style = nandina::theme::NanStylePrimitives::current().checkbox;
            const auto resolved_variant = m_color_variant == ColorVariant::inherit
                                              ? ColorVariant::primary
                                              : m_color_variant;

            const auto make_colors = [this, &style](const nandina::theme::NanCheckboxStyle::ColorFamilyStyle &family) {
                if (m_disabled) {
                    return ResolvedColors{
                        .box_bg = family.box_bg_disabled,
                        .box_border = family.box_border_disabled,
                        .check = family.check_disabled,
                        .box_bg_unchecked = family.box_bg_disabled,
                        .box_border_unchecked = family.box_border_disabled,
                    };
                }
                return ResolvedColors{
                    .box_bg = family.box_bg,
                    .box_border = family.box_border,
                    .check = family.check,
                    .box_bg_unchecked = style.box_bg_unchecked,
                    .box_border_unchecked = style.box_border_unchecked,
                };
            };

            switch (resolved_variant) {
                case ColorVariant::secondary: return make_colors(style.secondary_family);
                case ColorVariant::neutral: return make_colors(style.neutral_family);
                case ColorVariant::destructive: return make_colors(style.destructive_family);
                case ColorVariant::primary:
                case ColorVariant::inherit: break;
            }

            if (m_disabled) {
                return ResolvedColors{
                    .box_bg = style.box_bg_disabled,
                    .box_border = style.box_border_disabled,
                    .check = style.check_disabled,
                    .box_bg_unchecked = style.box_bg_disabled,
                    .box_border_unchecked = style.box_border_disabled,
                };
            }
            return ResolvedColors{
                .box_bg = style.box_bg,
                .box_border = style.box_border,
                .check = style.check,
                .box_bg_unchecked = style.box_bg_unchecked,
                .box_border_unchecked = style.box_border_unchecked,
            };
        }

        auto sync_visual_state() -> void {
            // colors are resolved on-demand in on_draw
        }

        Text *m_label_widget{nullptr};
        FocusRing *m_focus_ring{nullptr};
        CheckboxSize m_size{CheckboxSize::md};
        ColorVariant m_color_variant{ColorVariant::inherit};
        bool m_checked{false};
        bool m_hovered{false};
        bool m_pressed{false};
        bool m_disabled{false};
        reactive::EventSignal<bool> m_checked_changed_signal;
    };
} // namespace nandina::widgets
