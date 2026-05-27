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
import nandina.layout.container;
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
                m_label->set_visible(!text.empty());
            }
            mark_layout_dirty();
            return *this;
        }

        auto set_show_accent(bool show) -> SidebarGroup& {
            m_show_accent = show;
            if (m_accent_icon) m_accent_icon->set_visible(show);
            mark_layout_dirty();
            return *this;
        }

        // ── 添加子节点（菜单项进入内部 Column） ───────
        auto add_child(std::unique_ptr<runtime::NanWidget> child) -> void {
            if (m_item_column) m_item_column->add(std::move(child));
            mark_layout_dirty();
        }

        auto set_accent_color(const nandina::NanColor& color) -> SidebarGroup& {
            m_accent_color.set(color);
            if (m_accent_icon) {
                m_accent_icon->set_color(color);
            }
            mark_layout_dirty();
            return *this;
        }

        auto set_label_color(const nandina::NanColor& color) -> SidebarGroup& {
            m_label_color.set(color);
            if (m_label) {
                m_label->set_color(color);
            }
            mark_layout_dirty();
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
            return *this;
        }

        auto layout() -> void override {
            Surface::layout();
            clear_layout_dirty();
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            return Surface::preferred_size();
        }

        // ── 绘制 ──────────────────────────────────────────
    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            // 装饰色条（左侧竖线）
            const auto rect = bounds();
            if (m_show_accent) {
                const auto& acc    = m_accent_color.get();
                const auto acc_rgb = acc.to<nandina::NanRgb>();
                auto* bar          = tvg::Shape::gen();
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

            auto col = layout::Column::Create();
            col->align_items(layout::LayoutAlignment::stretch);
            m_item_column = col.get();
            Surface::add_child(std::move(col));

            auto accent = Icon::create();
            accent->set_type(IconType::Square)
                .set_size(10.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{99, 102, 241}));
            accent->set_visible(false);
            m_accent_icon = accent.get();
            Surface::add_child(std::move(accent));

            auto label = Label::create();
            label->set_text("")
                .set_font_size(9.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{160, 162, 180}));
            label->set_visible(false);
            m_label = label.get();
            Surface::add_child(std::move(label));
        }

        layout::Column* m_item_column{nullptr};

        /// 统计菜单项数量（排除内部管理的 label 和 accent icon）
        [[nodiscard]] auto count_menu_items() const noexcept -> size_t {
            size_t cnt = 0;
            for_each_child([&](const runtime::NanWidget& child) {
                if (&child != m_label && &child != m_accent_icon) {
                    ++cnt;
                }
            });
            return cnt;
        }

        std::string m_label_text;
        bool m_show_accent{false};

        Label* m_label{nullptr};
        Icon* m_accent_icon{nullptr};

        reactive::Prop<nandina::NanColor> m_accent_color{
            nandina::NanColor::from(nandina::NanRgb{99, 102, 241})};
        reactive::Prop<nandina::NanColor> m_label_color{
            nandina::NanColor::from(nandina::NanRgb{160, 162, 180})};
    };

} // namespace nandina::widgets
