//
// Created by cvrain on 2026/4/29.
//

module;

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.sidebar_menu_button;

import nandina.runtime.nan_widget;
import nandina.foundation.nan_insets;
import nandina.foundation.nan_rect;
import nandina.foundation.nan_size;
import nandina.foundation.color;
import nandina.layout.core;
import nandina.layout.container;
import nandina.layout.flex_widgets;
import nandina.reactive.prop;
import nandina.widgets.surface;
import nandina.widgets.label;
import nandina.widgets.icon;

export namespace nandina::widgets {

    class SidebarMenuButton : public Surface {
    public:
        using Ptr = std::unique_ptr<SidebarMenuButton>;

        ~SidebarMenuButton() override = default;

        static auto create() -> Ptr {
            return Ptr{new SidebarMenuButton()};
        }

        auto set_label(std::string_view text) -> SidebarMenuButton& {
            m_label_text = text;
            if (m_label) {
                m_label->set_text(text);
            }
            mark_layout_dirty();
            return *this;
        }

        auto set_icon_type(IconType type) -> SidebarMenuButton& {
            m_icon_type = type;
            if (m_icon) {
                m_icon->set_type(type);
            }
            mark_layout_dirty();
            return *this;
        }

        auto set_active(bool active) -> SidebarMenuButton& {
            m_active = active;
            update_colors();
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto is_active() const noexcept -> bool {
            return m_active;
        }

        auto set_accent_color(const nandina::NanColor& color) -> SidebarMenuButton& {
            m_accent_color.set(color);
            mark_dirty();
            return *this;
        }

        auto on_click(std::function<void()> cb) -> SidebarMenuButton& {
            m_on_click = std::move(cb);
            return *this;
        }

        // -- Surface override --
        auto set_bg_color(const nandina::NanColor& color) -> SidebarMenuButton& {
            Surface::set_bg_color(color);
            return *this;
        }

        auto set_corner_radius(float radius) -> SidebarMenuButton& {
            Surface::set_corner_radius(radius);
            return *this;
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            return {200.0f, 36.0f};
        }

    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            const auto rect    = bounds();
            const float radius = m_corner_radius.get();
            const auto& acc    = m_accent_color.get();
            const auto acc_rgb = acc.to<nandina::NanRgb>();

            if (m_active) {
                auto* bar = tvg::Shape::gen();
                bar->appendRect(rect.x(), rect.y(), 3.0f, rect.height(), 0.0f, 0.0f);
                bar->fill(acc_rgb.red(), acc_rgb.green(), acc_rgb.blue(), acc_rgb.alpha());
                canvas.add(bar);

                auto* hl = tvg::Shape::gen();
                hl->appendRect(rect.x(), rect.y(), rect.width(), rect.height(), radius, radius);
                hl->fill(40, 42, 60, 200);
                canvas.add(hl);
            }
        }

    private:
        SidebarMenuButton() : Surface() {
            m_bg_color.set(nandina::NanColor::from(nandina::NanRgb{0, 0, 0, 0}));
            m_corner_radius.set(0.0f);
            m_padding.set(geometry::NanInsets{});

            auto row = layout::Row::Create();
            row->padding(14.0f, 2.0f, 10.0f, 2.0f)
                .gap(12.0f)
                .align_items(nandina::layout::LayoutAlignment::stretch);

            auto label = Label::create();
            label->set_text("")
                .set_font_size(9.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{160, 162, 180}));
            m_label = label.get();

            auto label_slot = layout::Expanded::Create();
            label_slot->child(std::move(label));
            row->add(std::move(label_slot));

            add_child(std::move(row));
        }

        void update_colors() {
            if (m_label) {
                const auto& c = m_active
                                    ? nandina::NanColor::from(nandina::NanRgb{220, 220, 240})
                                    : nandina::NanColor::from(nandina::NanRgb{160, 162, 180});
                m_label->set_color(c);
            }
            if (m_icon) {
                const auto& c = m_active
                                    ? nandina::NanColor::from(nandina::NanRgb{220, 220, 240})
                                    : nandina::NanColor::from(nandina::NanRgb{110, 112, 130});
                m_icon->set_color(c);
            }
        }

        auto on_pointer_down(const runtime::PointerButtonEvent&) -> bool override {
            if (m_on_click)
                m_on_click();
            return true;
        }

        bool m_active{false};
        bool m_hovered{false};
        std::string m_label_text;
        std::optional<IconType> m_icon_type;

        Icon* m_icon{nullptr};
        Label* m_label{nullptr};

        reactive::Prop<nandina::NanColor> m_accent_color{
            nandina::NanColor::from(nandina::NanRgb{99, 102, 241})};

        std::function<void()> m_on_click;
    };

} // namespace nandina::widgets
