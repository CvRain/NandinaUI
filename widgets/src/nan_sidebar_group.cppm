//
// Created by cvrain on 2026/4/29.
//

module;

#include <memory>
#include <string>
#include <string_view>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.sidebar_group;

import nandina.runtime.nan_widget;
import nandina.foundation.nan_insets;
import nandina.foundation.nan_rect;
import nandina.foundation.nan_size;
import nandina.foundation.color;
import nandina.reactive.prop;
import nandina.widgets.surface;
import nandina.widgets.label;
import nandina.widgets.icon;

/**
 * nandina.widgets.sidebar_group
 *
 * SidebarGroup — 侧边栏分组组件。
 *
 * 职责：
 * - Surface 容器，带分组标签（Optional）
 * - 可选左侧引导色条（和 Section 标题的装饰一致）
 * - 子节点（SidebarMenuButton 等）在标签下方排列
 *
 * 用法：
 *   auto group = SidebarGroup::create()
 *       .set_label("Navigation")
 *       .set_show_accent(true);
 *   group->add_child(std::move(btn));
 */
export namespace nandina::widgets {

    class SidebarGroup : public Surface {
    public:
        using Ptr = std::unique_ptr<SidebarGroup>;

        ~SidebarGroup() override = default;

        static auto create() -> Ptr {
            return Ptr{new SidebarGroup()};
        }

        auto set_label(std::string_view text) -> SidebarGroup& {
            m_label_text = text;
            if (m_label) {
                m_label->set_text(text);
            }
            mark_dirty();
            return *this;
        }

        auto set_show_accent(bool show) -> SidebarGroup& {
            m_show_accent = show;
            mark_dirty();
            return *this;
        }

        auto set_accent_color(const nandina::NanColor& color) -> SidebarGroup& {
            m_accent_color.set(color);
            if (m_accent_icon) {
                m_accent_icon->set_color(color);
            }
            mark_dirty();
            return *this;
        }

        auto set_label_color(const nandina::NanColor& color) -> SidebarGroup& {
            m_label_color.set(color);
            if (m_label) {
                m_label->set_color(color);
            }
            mark_dirty();
            return *this;
        }

        // ── Surface override ──────────────────────────────
        auto set_bg_color(const nandina::NanColor& color) -> SidebarGroup& {
            Surface::set_bg_color(color);
            return *this;
        }

        auto set_corner_radius(float radius) -> SidebarGroup& {
            Surface::set_corner_radius(radius);
            return *this;
        }

        // ── 布局 ──────────────────────────────────────────
        auto set_bounds(float x, float y, float w, float h) noexcept -> NanWidget& override {
            Surface::set_bounds(x, y, w, h);

            const float label_height = 16.0f;
            const float accent_offset = 14.0f;

            // 装饰色标（小方块）
            if (m_accent_icon && m_show_accent) {
                const float dot_size = 10.0f;
                const float dot_y = y + 10.0f;
                m_accent_icon->set_bounds(x + accent_offset, dot_y, dot_size, dot_size);
            }

            // 分组标签
            if (m_label) {
                const float lx = x + (m_show_accent ? 30.0f : 14.0f);
                const float ly = y + 6.0f;
                m_label->set_bounds(lx, ly, w - 40.0f, label_height);
            }

            // 子节点在标签下方排列（由 for_each_child 的 Surface 逻辑处理）
            return *this;
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            // 子节点累加 + 标签高度
            float child_h = 0.0f;
            for_each_child([&](const runtime::NanWidget& child) {
                const auto cp = child.preferred_size();
                child_h += cp.height();
            });
            const float header_h = m_label_text.empty() ? 10.0f : 28.0f;
            return {240.0f, header_h + child_h};
        }

        // ── 绘制 ──────────────────────────────────────────
    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            // 装饰色条（左侧竖线）
            const auto rect = bounds();
            if (m_show_accent) {
                const auto& acc = m_accent_color.get();
                const auto acc_rgb = acc.to<nandina::NanRgb>();
                auto* bar = tvg::Shape::gen();
                bar->appendRect(rect.x() + 14.0f, rect.y() + 6.0f, 4.0f, 14.0f, 2.0f, 2.0f);
                bar->fill(acc_rgb.red(), acc_rgb.green(), acc_rgb.blue(), acc_rgb.alpha());
                canvas.add(bar);
            }
            // 子节点由基类绘制
        }

    private:
        SidebarGroup() : Surface() {
            m_bg_color.set(nandina::NanColor::from(nandina::NanRgb{0, 0, 0, 0}));
            m_corner_radius.set(0.0f);
            m_padding.set(geometry::NanInsets{});
            m_show_accent = false;

            auto label = Label::create();
            label->set_text("")
                .set_font_size(9.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{160, 162, 180}));
            m_label = label.get();
            add_child(std::move(label));

            auto accent = Icon::create();
            accent->set_type(IconType::Square)
                .set_size(10.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{99, 102, 241}));
            m_accent_icon = accent.get();
            add_child(std::move(accent));
        }

        std::string m_label_text;
        bool m_show_accent{false};

        Label* m_label{nullptr};
        Icon*  m_accent_icon{nullptr};

        reactive::Prop<nandina::NanColor> m_accent_color{
            nandina::NanColor::from(nandina::NanRgb{99, 102, 241})};
        reactive::Prop<nandina::NanColor> m_label_color{
            nandina::NanColor::from(nandina::NanRgb{160, 162, 180})};
    };

} // namespace nandina::widgets
