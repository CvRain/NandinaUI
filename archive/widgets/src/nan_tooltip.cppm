module;

#include <memory>
#include <string>
#include <string_view>

export module nandina.widgets.tooltip;

import nandina.foundation.color;
import nandina.runtime.nan_event;
import nandina.theme.nan_style;
import nandina.widgets.label;
import nandina.widgets.popover;

export namespace nandina::widgets {

    class Tooltip final : public Popover {
    public:
        using Ptr = std::unique_ptr<Tooltip>;

        ~Tooltip() override = default;

        static auto create() -> Ptr {
            return Ptr{new Tooltip()};
        }

        auto text(std::string_view value) -> Tooltip& {
            m_label->set_text(value);
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto text() const noexcept -> const std::string& {
            return m_label->text();
        }

    protected:
        auto on_pointer_move(const runtime::PointerMoveEvent& event) -> bool override {
            const bool handled = Popover::on_pointer_move(event);
            if (handled) {
                open(true);
            } else {
                open(false);
            }
            return handled;
        }

        auto on_pointer_leave(const runtime::PointerMoveEvent& event) -> bool override {
            open(false);
            return Popover::on_pointer_leave(event);
        }

        auto on_pointer_down(const runtime::PointerButtonEvent&) -> bool override {
            return false;
        }

        auto on_pointer_up(const runtime::PointerButtonEvent&) -> bool override {
            return false;
        }

    private:
        Tooltip() {
            const auto& style = theme::NanStylePrimitives::current().tooltip;
            placement(style.placement);
            gap(style.gap);
            panel_surface()
                .set_bg_color(style.bg)
                .set_corner_radius(style.corner_radius)
                .set_border_width(0.0f)
                .set_padding(style.padding);

            auto label = Label::create();
            label->set_color(style.text);
            label->set_font_size(style.font_size);
            label->set_single_line(true);
            m_label = label.get();
            content(std::move(label));
        }

        Label* m_label{nullptr};
    };
}
