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
            mark_dirty();
            return *this;
        }

        auto set_icon_type(IconType type) -> SidebarMenuButton& {
            m_icon_type = type;
            if (m_icon) {
                m_icon->set_type(type);
            }
            mark_dirty();
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

        // -- Layout --
        auto set_bounds(float x, float y, float w, float h) noexcept -> NanWidget& override {
            Surface::set_bounds(x, y, w, h);

            const float icon_size = h * 0.48f;
            const float icon_y = y + (h - icon_size) * 0.5f;

            if (m_icon) {
                m_icon->set_bounds(x + 14.0f, icon_y, icon_size, icon_size);
            }

            if (m_label) {
                const float lx = x + 42.0f;
                const float lw = w - 52.0f;
                m_label->set_bounds(lx, y + 2.0f, lw, h - 4.0f);
            }

            return *this;
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            return {200.0f, 36.0f};
        }

    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            const auto rect = bounds();
            const float radius = m_corner_radius.get();
            const auto& acc = m_accent_color.get();
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

            auto icon = Icon::create();
            icon->set_type(IconType::Square)
                .set_size(16.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{160, 162, 180}));
            m_icon = icon.get();
            add_child(std::move(icon));

            auto label = Label::create();
            label->set_text("")
                .set_font_size(9.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{160, 162, 180}));
            m_label = label.get();
            add_child(std::move(label));
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
            if (m_on_click) m_on_click();
            return true;
        }

        bool m_active{false};
        bool m_hovered{false};
        std::string m_label_text;
        std::optional<IconType> m_icon_type;

        Icon*  m_icon{nullptr};
        Label* m_label{nullptr};

        reactive::Prop<nandina::NanColor> m_accent_color{
            nandina::NanColor::from(nandina::NanRgb{99, 102, 241})};

        std::function<void()>       m_on_click;
    };

} // namespace nandina::widgets
