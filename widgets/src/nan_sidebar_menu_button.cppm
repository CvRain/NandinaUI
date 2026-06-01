//
// Created by cvrain on 2026/4/29.
//
// SidebarMenuButton — 基于 Button(ghost variant) 构建的侧边栏导航项。
// 预配置：ghost 样式、sidebar 尺寸和内边距、sidebar 文字颜色。
// 扩展：active() — 绘制左侧 3px 指示条 + 高亮背景。
//
// 设计原则：
//   - Button 能做的不重新封装：text()、icon_left()、font_*() 等直接使用 Button 的 API
//   - 自身专属：active() 状态、accent_color()、active_changed 信号
//

module;

#include <functional>
#include <memory>
#include <optional>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.sidebar_menu_button;

import nandina.foundation.nan_insets;
import nandina.foundation.nan_size;
import nandina.foundation.color;
import nandina.layout.container;
import nandina.reactive.event_signal;
import nandina.theme.nan_style;
import nandina.widgets.button;
import nandina.widgets.icon;

export namespace nandina::widgets {
    class SidebarMenuButton : public Button {
    public:
        using Ptr = std::unique_ptr<SidebarMenuButton>;

        ~SidebarMenuButton() override = default;

        static auto create() -> Ptr {
            return Ptr{new SidebarMenuButton()};
        }

        // ── Active 状态（sidebar 导航高亮）─────────────────────────────

        /// 无参：读取 active 状态
        [[nodiscard]] auto active() const noexcept -> bool { return m_active; }

        /// 有参：设置 active 状态，自动切换文字色并触发信号
        auto active(bool v) -> SidebarMenuButton& {
            if (m_active == v) return *this;
            m_active = v;
            update_visual_state();
            mark_dirty();
            m_active_changed_signal.emit(v);
            return *this;
        }

        // ── Accent 指示条颜色──────────────────────────────────────────

        [[nodiscard]] auto accent_color() const noexcept -> const nandina::NanColor& {
            return m_accent_color;
        }

        auto accent_color(nandina::NanColor color) -> SidebarMenuButton& {
            m_has_explicit_accent_color = true;
            m_accent_color = color;
            update_visual_state();
            mark_dirty();
            return *this;
        }

        // ── active_changed 回调 / 信号─────────────────────────────────

        auto on_active_changed(std::function<void(bool)> cb) -> SidebarMenuButton& {
            m_active_changed_signal.connect(std::move(cb));
            return *this;
        }

        [[nodiscard]] auto active_changed() -> reactive::EventSignal<bool>& {
            return m_active_changed_signal;
        }

        // ── 首选尺寸──────────────────────────────────────────────────

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            return {200.0f, 36.0f};
        }

    protected:
        auto update_visual_state() -> void override {
            Button::update_visual_state();

            if (!m_has_explicit_accent_color) {
                m_accent_color = resolved_accent_color();
            }

            if (!m_active) {
                return;
            }

            if (apply_resolved_foreground_color(m_accent_color)) {
                mark_dirty();
            }
        }

        void on_draw(tvg::SwCanvas& canvas) override {
            Button::on_draw(canvas);

            if (m_active) {
                const auto rect    = bounds();
                const auto acc_rgb = m_accent_color.to<nandina::NanRgb>();

                // 左侧 3px 指示条
                auto* bar = tvg::Shape::gen();
                bar->appendRect(rect.x(), rect.y(), 3.0f, rect.height(), 0.0f, 0.0f);
                bar->fill(acc_rgb.red(), acc_rgb.green(), acc_rgb.blue(), acc_rgb.alpha());
                canvas.add(bar);

                // active 高亮背景
                auto* hl = tvg::Shape::gen();
                hl->appendRect(rect.x(), rect.y(), rect.width(), rect.height(), 6.0f, 6.0f);
                hl->fill(acc_rgb.red(), acc_rgb.green(), acc_rgb.blue(), 48);
                canvas.add(hl);
            }
        }

    private:
        SidebarMenuButton() : Button() {
            variant(ButtonVariant::ghost);
            font_size(13.0f);
            set_padding(geometry::NanInsets{14.0f, 4.0f, 12.0f, 4.0f});
            set_corner_radius(6.0f);
            // 内容左对齐（图标 + 文字靠左，而非居中）
            content_row().justify_content(layout::LayoutAlignment::start);
            update_visual_state();
        }

        [[nodiscard]] auto resolved_accent_color() const -> nandina::NanColor {
            const auto preset = resolved_preset_style();
            const auto border = preset.border.to<nandina::NanRgb>();
            if (border.alpha() > 0) {
                return preset.border;
            }

            const auto bg = preset.bg.to<nandina::NanRgb>();
            if (bg.alpha() > 0) {
                return preset.bg;
            }

            return preset.text;
        }

        bool m_active{false};
        bool m_has_explicit_accent_color{false};
        nandina::NanColor m_accent_color{nandina::NanColor::from(nandina::NanRgb{99, 102, 241})};
        reactive::EventSignal<bool> m_active_changed_signal;
    };
} // namespace nandina::widgets
