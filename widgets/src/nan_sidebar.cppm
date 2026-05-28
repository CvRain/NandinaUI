//
// Created by cvrain on 2026/4/29.
//

module;

#include <memory>
#include <string>
#include <string_view>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.sidebar;

import nandina.layout.container;
import nandina.layout.flex_widgets;
import nandina.runtime.nan_widget;
import nandina.foundation.nan_insets;
import nandina.foundation.nan_rect;
import nandina.foundation.nan_size;
import nandina.foundation.color;
import nandina.reactive.prop;
import nandina.widgets.surface;
import nandina.widgets.label;
import nandina.widgets.icon;
import nandina.widgets.sidebar_group;
import nandina.widgets.sidebar_menu_button;

/**
 * @file nan_sidebar.cppm
 * @brief Sidebar — 应用侧边栏组件
 *
 * 内部使用 Row / Column / Padding / SizedBox / Expanded 布局原语组合，
 * 不依赖手工坐标计算。通过 Surface 继承获得背景/圆角/内边距能力。
 *
 * ## 结构
 *   ┌─────────────────┐
 *   │  Header (Logo + 标题)        │ ← m_header_slot (SizedBox, h=50)
 *   │  ─── divider ───             │
 *   │  Content (SidebarGroup 列表) │ ← m_content_slot (Expanded)
 *   │  ─── divider ───             │
 *   │  Footer (头像 + 用户名 + 角色)│ ← m_footer_slot (SizedBox, h=56)
 *   └─────────────────┘
 *
 * ## 创建方式
 *
 * Sidebar 是**有状态容器**，需要动态构建子组件（add_group / add_menu_item），
 * 采用 `create() → 配置 → adopt()` 模式：
 *
 * @code
 *   auto sidebar = Sidebar::create();
 *   sidebar->set_header_title("My App")
 *           .set_user_name("Alice");
 *   sidebar->add_menu_item(std::move(btn));
 *   // 嵌入 authoring 树：
 *   sized_box(adopt(std::move(sidebar))).width(260)
 * @endcode
 *
 * 或通过便捷入口 create_shell() / setup_shell() 自动构建。
 *
 * @see create_shell() in nan_authoring.cppm
 */
export namespace nandina::widgets {

    class Sidebar : public Surface {
    public:
        using Ptr = std::unique_ptr<Sidebar>;

        ~Sidebar() override = default;

        static auto create() -> Ptr {
            return Ptr{new Sidebar()};
        }

        /// 添加一个分组到导航区域
        auto add_group(SidebarGroup::Ptr group) -> Sidebar& {
            if (m_content_layout) {
                m_content_layout->add(std::move(group));
            }
            mark_layout_dirty();
            return *this;
        }

        /// 添加菜单项到导航区域（快捷方式：自动创建 SidebarGroup）
        auto add_menu_item(SidebarMenuButton::Ptr item) -> Sidebar& {
            if (!m_content_group) {
                auto group = SidebarGroup::create();
                group->label("Navigation");
                m_content_group = group.get();
                add_group(std::move(group));
            }
            m_content_group->add_child(std::move(item));
            mark_layout_dirty();
            return *this;
        }

        /// 添加项目到项目区
        auto add_project_item(SidebarMenuButton::Ptr item) -> Sidebar& {
            if (!m_project_group) {
                auto group = SidebarGroup::create();
                group->label("Recent Projects");
                m_project_group = group.get();
                add_group(std::move(group));
            }
            m_project_group->add_child(std::move(item));
            mark_layout_dirty();
            return *this;
        }

        // ── 属性 ──────────────────────────────────────────
        auto set_header_title(std::string_view text) -> Sidebar& {
            m_header_title_text = text;
            const bool visible = !text.empty();
            if (m_header_slot) {
                m_header_slot->height(visible ? 50.0f : 0.0f);
                m_header_slot->set_visible(visible);
            }
            if (m_header_label) {
                m_header_label->set_text(text);
                m_header_label->set_visible(visible);
            }
            if (m_header_logo) m_header_logo->set_visible(visible);
            mark_layout_dirty();
            return *this;
        }

        auto set_user_name(std::string_view text) -> Sidebar& {
            m_user_name_text = text;
            _sync_footer_visibility();
            if (m_user_name_label) m_user_name_label->set_text(text);
            mark_layout_dirty();
            return *this;
        }

        auto set_user_role(std::string_view text) -> Sidebar& {
            m_user_role_text = text;
            _sync_footer_visibility();
            if (m_user_role_label) m_user_role_label->set_text(text);
            mark_layout_dirty();
            return *this;
        }

    private:
        auto _sync_footer_visibility() -> void {
            if (!m_footer_slot) return;
            const bool visible = !(m_user_name_text.empty() && m_user_role_text.empty());
            m_footer_slot->height(visible ? 70.0f : 0.0f);
            m_footer_slot->set_visible(visible);
            if (m_user_avatar)     m_user_avatar->set_visible(visible);
            if (m_user_name_label) m_user_name_label->set_visible(!m_user_name_text.empty());
            if (m_user_role_label) m_user_role_label->set_visible(!m_user_role_text.empty());
        }

        // ── Surface override ──────────────────────────────
        auto set_bg_color(const nandina::NanColor& color) -> Sidebar& {
            Surface::set_bg_color(color);
            return *this;
        }

        auto set_corner_radius(float radius) -> Sidebar& {
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

            m_divider_header_y = m_header_slot ? m_header_slot->bounds().bottom() : 0.0f;
            m_divider_footer_y = m_footer_slot ? m_footer_slot->bounds().y() : 0.0f;
        }

    protected:
        void on_draw(tvg::SwCanvas& canvas) override {
            const auto rect = bounds();

            // ── 背景（由 Surface draw_background 处理） ──
            draw_background(canvas);

            // ── 分割线 ────────────────────────────────────
            auto draw_divider = [&](float y) {
                auto* line = tvg::Shape::gen();
                line->moveTo(rect.x() + 12.0f, y);
                line->lineTo(rect.x() + rect.width() - 12.0f, y);
                line->strokeWidth(1.0f);
                line->strokeFill(55, 57, 75, 150);
                canvas.add(line);
            };

            const bool header_visible = m_header_slot && m_header_slot->runtime::NanWidget::height() > 0.0f;
            const bool footer_visible = m_footer_slot && m_footer_slot->runtime::NanWidget::height() > 0.0f;
            if (m_divider_header_y > 0.0f && header_visible) {
                draw_divider(m_divider_header_y);
            }
            if (m_divider_footer_y > 0.0f && footer_visible) {
                draw_divider(m_divider_footer_y);
            }
        }

    protected:
        Sidebar() : Surface() {
            m_bg_color.set(nandina::NanColor::from(nandina::NanRgb{42, 44, 62}));
            m_corner_radius.set(0.0f);
            m_padding.set(geometry::NanInsets{});

            auto root_column = layout::Column::Create();
            root_column->align_items(layout::LayoutAlignment::stretch);

            // ── Header slot（默认完全隐藏：不可见 + 零高度）───
            {
                auto logo = Icon::create();
                logo->set_type(IconType::Circle).set_size(28.0f)
                    .set_color(NanColor::from(NanRgb{99, 102, 241}));
                logo->set_visible(false);
                m_header_logo = logo.get();

                auto hdr = Label::create();
                hdr->set_text("");
                hdr->set_visible(false);
                hdr->set_font_size(11.0f)
                    .set_color(NanColor::from(NanRgb{220, 220, 240}));
                m_header_label = hdr.get();

                auto lbl_pad = layout::Padding::Create();
                lbl_pad->padding(0, 2, 0, 4).child(std::move(hdr));

                auto hdr_row = layout::Row::Create();
                hdr_row->align_items(layout::LayoutAlignment::center).gap(9);

                auto logo_box = layout::SizedBox::Create();
                logo_box->width(28).height(28).child(std::move(logo));
                hdr_row->add(std::move(logo_box));

                auto lbl_exp = layout::Expanded::Create();
                lbl_exp->child(std::move(lbl_pad));
                hdr_row->add(std::move(lbl_exp));

                auto hdr_pad = layout::Padding::Create();
                hdr_pad->padding(18, 16, 15, 6).child(std::move(hdr_row));

                auto hdr_slot = layout::SizedBox::Create();
                hdr_slot->height(0).child(std::move(hdr_pad));
                hdr_slot->set_visible(false);
                m_header_slot = hdr_slot.get();
                root_column->add(std::move(hdr_slot));
            }

            // ── Content（Expanded） ──────────────────────
            auto content_layout = layout::Column::Create();
            content_layout->align_items(layout::LayoutAlignment::stretch).gap(2);
            m_content_layout = content_layout.get();

            auto content_pad = layout::Padding::Create();
            content_pad->padding(0, 4, 0, 0).child(std::move(content_layout));

            auto content_exp = layout::Expanded::Create();
            content_exp->child(std::move(content_pad));
            m_content_slot = content_exp.get();
            root_column->add(std::move(content_exp));

            // ── Footer slot（默认完全隐藏：不可见 + 零高度）───
            {
                auto av = Icon::create();
                av->set_type(IconType::Circle).set_size(28);
                av->set_visible(false);
                av->set_color(NanColor::from(NanRgb{147, 150, 255}));
                m_user_avatar = av.get();

                auto un = Label::create();
                un->set_text("");
                un->set_visible(false);
                un->set_font_size(9)
                    .set_color(NanColor::from(NanRgb{220, 220, 240}));
                m_user_name_label = un.get();

                auto ur = Label::create();
                ur->set_text("");
                ur->set_visible(false);
                ur->set_font_size(7)
                    .set_color(NanColor::from(NanRgb{110, 112, 130}));
                m_user_role_label = ur.get();

                auto av_box = layout::SizedBox::Create(); av_box->width(28).height(28).child(std::move(av));
                auto nm_box = layout::SizedBox::Create(); nm_box->height(16).child(std::move(un));
                auto rl_box = layout::SizedBox::Create(); rl_box->height(14).child(std::move(ur));

                auto info_col = layout::Column::Create();
                info_col->align_items(layout::LayoutAlignment::stretch);
                info_col->add(std::move(nm_box)); info_col->add(std::move(rl_box));

                auto info_exp = layout::Expanded::Create(); info_exp->child(std::move(info_col));

                auto ft_row = layout::Row::Create();
                ft_row->align_items(layout::LayoutAlignment::start).gap(8);
                ft_row->add(std::move(av_box)); ft_row->add(std::move(info_exp));

                auto ft_pad = layout::Padding::Create();
                ft_pad->padding(16, 12, 12, 28).child(std::move(ft_row));

                auto ft_slot = layout::SizedBox::Create();
                ft_slot->height(0).child(std::move(ft_pad));
                ft_slot->set_visible(false);
                m_footer_slot = ft_slot.get();
                root_column->add(std::move(ft_slot));
            }

            add_child(std::move(root_column));
        }

        std::string m_header_title_text;
        std::string m_user_name_text;
        std::string m_user_role_text;

        // Header 子节点
        Icon* m_header_logo{nullptr};
        Label* m_header_label{nullptr};

        layout::SizedBox* m_header_slot{nullptr};
        layout::Expanded* m_content_slot{nullptr};
        layout::Column* m_content_layout{nullptr};
        layout::SizedBox* m_footer_slot{nullptr};

        // 分组指针（内部自动管理）
        SidebarGroup* m_content_group{nullptr};
        SidebarGroup* m_project_group{nullptr};

        // Footer 子节点
        Icon* m_user_avatar{nullptr};
        Label* m_user_name_label{nullptr};
        Label* m_user_role_label{nullptr};

        // 分割线 Y 坐标
        float m_divider_header_y{0.0f};
        float m_divider_footer_y{0.0f};
    };

} // namespace nandina::widgets
