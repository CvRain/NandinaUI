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
import nandina.widgets.button;
import nandina.widgets.surface;

// ── SidebarGroupLabel — 模块私有，仅供 SidebarGroup 使用 ─────────────────────
// 基于 Button(ghost)，构造时调 font_color() 设置 m_user_font_color，
// 依靠 Button 内置的 m_user_font_color 机制自动保持灰色文字不被 hover 重置。
//
// TODO: 待 NanIcon 组件实现后，为 SidebarGroup 添加 set_icon(IconType) 方法。
namespace nandina::widgets {

    class SidebarGroupLabel final : public Button {
    public:
        static auto create() -> std::unique_ptr<SidebarGroupLabel> {
            return std::unique_ptr<SidebarGroupLabel>{new SidebarGroupLabel()};
        }

    protected:
        SidebarGroupLabel() : Button() {
            variant(ButtonVariant::ghost);
            // font_color() 会将颜色写入 m_user_font_color，从而在 hover/leave 后自动恢复
            font_color(nandina::NanColor::from(nandina::NanRgb{120, 122, 140})); // 分组标题颜色稍暗
            font_size(13.0f);
            set_padding(geometry::NanInsets{14.0f, 2.0f, 12.0f, 2.0f});
            set_corner_radius(4.0f);
            // 分组标签左对齐
            content_row().justify_content(layout::LayoutAlignment::start);
        }
        // 无需覆写 update_visual_state()，Button 内置机制已处理颜色持久化
    };

} // namespace nandina::widgets

/**
 * nandina.widgets.sidebar_group
 *
 * SidebarGroup — 侧边栏分组组件。
 *
 * 职责：
 * - Surface 容器，带分组标签（Optional）
 * - 子节点（SidebarMenuButton 等）在标签下方排列
 *
 * 用法：
 *   auto group = SidebarGroup::create();
 *   group->label("Navigation");
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

        /// label(sv) — 设置分组标题文本；label() — 读取
        auto label(std::string_view text) -> SidebarGroup& {
            m_label_text = text;
            if (m_label) {
                m_label->text(text);
            }
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto label() const noexcept -> std::string_view { return m_label_text; }

        // ── 添加子节点 ──────────────────────────────────
        auto add_child(std::unique_ptr<runtime::NanWidget> child) -> void {
            Surface::add_child(std::move(child));
            mark_layout_dirty();
        }

        /// 返回头部按钮引用，开发者可直接配置样式 / 事件回调
        /// 示例： group.head().font_color(NanColor::from(NanRgb{200,200,220}))
        ///           group.head().on_click([&]{ toggle_expand(); })
        [[nodiscard]] auto head() -> Button& { return *m_label; }

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
        /// measure() 必须重写：Surface::measure() 用 max(children) 计算高度（重叠语义），
        /// 而 SidebarGroup 是堆叠布局，measured_size 应与 preferred_size() 一致。
        auto measure(const geometry::NanConstraints& constraints) -> void override {
            // 先让子节点自行 measure（保证其状态正确）
            for_each_child([&](runtime::NanWidget& child) {
                child.measure(constraints.loosen());
            });
            // 以堆叠高度作为自身 measured_size
            const auto pref = preferred_size();
            set_measured_layout_state(constraints, constraints.constrain(pref));
        }

        auto set_bounds(float x, float y, float w, float h) noexcept -> NanWidget& override {
            Surface::set_bounds(x, y, w, h);
            return *this;
        }

        auto layout() -> void override {
            float item_y = y();

            // 若有分组标签（有文本），先定位 label，再在其下方排列菜单项
            // 用文本是否为空判断，而非 visible()，以兼容 head().label().set_text() 路径
            constexpr float k_label_pad_top = 4.0f;
            constexpr float k_label_h       = 24.0f;
            constexpr float k_label_gap     = 4.0f;
            if (m_label && !m_label->text().empty()) {
                m_label->set_bounds(x() + 16.0f, item_y + k_label_pad_top,
                                    width() - 32.0f, k_label_h);
                m_label->layout();
                item_y += k_label_pad_top + k_label_h + k_label_gap;
            } else if (m_label) {
                m_label->set_bounds(0.0f, 0.0f, 0.0f, 0.0f); // 无文本时清除 bounds，防止事件拦截
            }

            for_each_child([&](runtime::NanWidget& child) {
                if (&child == m_label) return;
                child.set_bounds(x(), item_y, width(), 36.0f);
                child.layout();
                item_y += 38.0f; // 36 height + 2 gap
            });
            clear_layout_dirty();
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            float total_h = 0.0f;
            if (m_label && !m_label->text().empty()) {
                total_h += 4.0f + 24.0f + 4.0f; // pad_top + label_h + gap
            }
            size_t n = 0;
            for_each_child([&](const runtime::NanWidget& child) {
                if (&child != m_label) ++n;
            });
            total_h += static_cast<float>(n) * 38.0f;
            return {240.0f, total_h};
        }

    private:
        SidebarGroup() : Surface() {
            m_bg_color.set(nandina::NanColor::from(nandina::NanRgb{0, 0, 0, 0}));
            m_corner_radius.set(0.0f);
            m_padding.set(geometry::NanInsets{});

            auto label = SidebarGroupLabel::create();
            m_label = label.get();
            Surface::add_child(std::move(label));
        }

        std::string m_label_text;
        SidebarGroupLabel* m_label{nullptr};
    };

} // namespace nandina::widgets
