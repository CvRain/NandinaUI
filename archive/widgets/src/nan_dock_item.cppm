//
// Created by cvrain on 2026/4/29.
//

module;

#include <memory>
#include <functional>
#include <cmath>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.dock_item;

import nandina.runtime.nan_widget;
import nandina.foundation.nan_rect;
import nandina.foundation.nan_size;
import nandina.foundation.color;
import nandina.widgets.icon;

/**
 * nandina.widgets.dock_item
 *
 * DockItem — Dock 栏中的单个图标按钮。
 *
 * 职责：
 * - 彩色圆角矩形背景（32x32）
 * - 白色前景图标（Icon）
 * - 可选底部活动指示圆点（前3个高亮应用）
 * - 点击交互（on_click 回调）
 *
 * 结构：
 *   DockItem
 *   ├── Icon（白色前景，居中显示）
 *   └── 底部指示圆点（可选，由 set_show_active_dot 控制）
 *
 * 用法：
 *   auto item = DockItem::create();
 *   item->set_bg_color(NanColor::from(NanRgb{99, 102, 241}))
 *       .set_icon_type(IconType::Monitor)
 *       .set_show_active_dot(true);
 *   dock->add_child(std::move(item));
 */
export namespace nandina::widgets {

    class DockItem : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<DockItem>;

        ~DockItem() override = default;

        static auto create() -> Ptr {
            return Ptr{new DockItem()};
        }

        // ── 背景色 ──────────────────────────────────────────
        auto set_bg_color(const nandina::NanColor& color) -> DockItem& {
            m_bg_color = color;
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto bg_color() const noexcept -> const nandina::NanColor& {
            return m_bg_color;
        }

        // ── 图标类型 ────────────────────────────────────────
        auto set_icon_type(IconType type) -> DockItem& {
            m_icon_type = type;
            if (m_icon) {
                m_icon->set_type(type);
            }
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto icon_type() const noexcept -> IconType {
            return m_icon_type;
        }

        // ── 图标颜色（默认白色） ────────────────────────────
        auto set_icon_color(const nandina::NanColor& color) -> DockItem& {
            if (m_icon) {
                m_icon->set_color(color);
            }
            mark_dirty();
            return *this;
        }

        // ── 活动指示圆点 ────────────────────────────────────
        auto set_show_active_dot(bool show) -> DockItem& {
            m_show_active_dot = show;
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto show_active_dot() const noexcept -> bool {
            return m_show_active_dot;
        }

        // ── 活动指示圆点颜色（默认与背景色相同） ────────────
        auto set_active_dot_color(const nandina::NanColor& color) -> DockItem& {
            m_active_dot_color = color;
            mark_dirty();
            return *this;
        }

        // ── 点击回调 ────────────────────────────────────────
        auto on_click(std::function<void()> callback) -> DockItem& {
            m_on_click = std::move(callback);
            return *this;
        }

        // ── 布局与绘制 ──────────────────────────────────────
        auto set_bounds(float x, float y, float w, float h) noexcept -> NanWidget& override {
            NanWidget::set_bounds(x, y, w, h);

            return *this;
        }

        auto layout() -> void override {
            // 定位 Icon 子节点在中央
            if (m_icon) {
                const float icon_size = std::min(width(), height()) * 0.5f;
                const float icon_x    = x() + (width() - icon_size) * 0.5f;
                const float icon_y    = y() + (height() - icon_size) * 0.5f - (m_show_active_dot ? 3.0f : 0.0f);
                m_icon->set_bounds(icon_x, icon_y, icon_size, icon_size);
            }

            clear_layout_dirty();
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            return {32.0f, 32.0f};
        }

    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            const auto rect   = bounds();
            const auto bg_rgb = m_bg_color.to<nandina::NanRgb>();

            // ── 1. 绘制圆形背景 ──────────────────────────
            auto* shape = tvg::Shape::gen();
            shape->appendRect(rect.x(), rect.y(), rect.width(), rect.height(), 7.0f, 7.0f);
            shape->fill(bg_rgb.red(), bg_rgb.green(), bg_rgb.blue(), bg_rgb.alpha());
            canvas.add(shape);

            // ── 2. 活动指示圆点（在底边下方） ────────────
            if (m_show_active_dot) {
                const auto dot_rgb = m_active_dot_color.to<nandina::NanRgb>();
                auto* dot          = tvg::Shape::gen();
                dot->appendCircle(rect.center().x(), rect.y() + rect.height() + 4.0f, 2.0f, 2.0f);
                dot->fill(dot_rgb.red(), dot_rgb.green(), dot_rgb.blue(), dot_rgb.alpha());
                canvas.add(dot);
            }
        }

    private:
        DockItem() {
            // 自动创建 Icon 子节点
            auto icon = Icon::create();
            icon->set_type(IconType::Square)
                .set_size(16.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{255, 255, 255}));
            m_icon = static_cast<Icon*>(add_child(std::move(icon)));
        }

        nandina::NanColor m_bg_color{nandina::NanColor::from(nandina::NanRgb{99, 102, 241})};
        IconType m_icon_type{IconType::Square};
        bool m_show_active_dot{false};
        nandina::NanColor m_active_dot_color{m_bg_color};
        std::function<void()> m_on_click;

        Icon* m_icon{nullptr};
    };

} // namespace nandina::widgets
