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
import nandina.layout.flex_widgets;
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
            sync_label_slot_visibility();
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto label() const noexcept -> std::string_view {
            if (m_label) {
                return m_label->text();
            }
            return m_label_text;
        }

        // ── 添加子节点 ──────────────────────────────────
        auto add_child(std::unique_ptr<runtime::NanWidget> child) -> void {
            if (m_content_layout) {
                m_content_layout->add(std::move(child));
                return;
            }

            Surface::add_child(std::move(child));
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

        auto measure(const geometry::NanConstraints& constraints) -> void override {
            sync_label_slot_visibility();
            Surface::measure(constraints);
        }

        auto layout() -> void override {
            sync_label_slot_visibility();
            Surface::layout();
        }

        [[nodiscard]] auto preferred_size() const noexcept -> geometry::NanSize override {
            const auto pref = Surface::preferred_size();
            return {std::max(k_min_preferred_width, pref.width()), pref.height()};
        }

    private:
        SidebarGroup() : Surface() {
            m_bg_color.set(nandina::NanColor::from(nandina::NanRgb{0, 0, 0, 0}));
            m_corner_radius.set(0.0f);
            m_padding.set(geometry::NanInsets{});

            auto root_column = layout::Column::Create();
            root_column->align_items(layout::LayoutAlignment::stretch);

            auto label = SidebarGroupLabel::create();
            m_label = label.get();

            auto label_box = layout::SizedBox::Create();
            label_box->height(k_label_height).child(std::move(label));

            auto label_pad = layout::Padding::Create();
            label_pad->padding(16.0f, 4.0f, 16.0f, 4.0f).child(std::move(label_box));

            auto label_slot = layout::SizedBox::Create();
            label_slot->height(0.0f).child(std::move(label_pad));
            label_slot->set_visible(false);
            m_label_slot = label_slot.get();
            root_column->add(std::move(label_slot));

            auto content_layout = layout::Column::Create();
            content_layout->align_items(layout::LayoutAlignment::stretch).gap(k_item_gap);
            m_content_layout = content_layout.get();
            root_column->add(std::move(content_layout));

            Surface::add_child(std::move(root_column));
        }

        auto sync_label_slot_visibility() const -> void {
            if (!m_label_slot || !m_label) {
                return;
            }

            const bool visible = !m_label->text().empty();
            m_label_slot->height(visible ? k_label_slot_height : 0.0f);
            m_label_slot->set_visible(visible);
            m_label->set_visible(visible);
        }

        static constexpr float k_label_height = 24.0f;
        static constexpr float k_label_slot_height = 32.0f;
        static constexpr float k_item_gap = 2.0f;
        static constexpr float k_min_preferred_width = 240.0f;

        std::string m_label_text;
        SidebarGroupLabel* m_label{nullptr};
        layout::SizedBox* m_label_slot{nullptr};
        layout::Column* m_content_layout{nullptr};
    };

} // namespace nandina::widgets
