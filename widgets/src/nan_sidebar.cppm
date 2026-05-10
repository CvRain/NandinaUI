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
 * nandina.widgets.sidebar
 *
 * Sidebar — 侧边栏主组件。
 *
 * 职责：
 * - 全高度容器，固定宽度
 * - 组合：Header（Logo + 标题） → Content（多个 SidebarGroup） → Footer（用户信息）
 * - 用户信息区域：头像（Icon Circle） + 用户名 + 角色
 *
 * 用法：
 *   auto sidebar = Sidebar::create();
 *
 *   // Header 自动创建（Logo + "Nandina Studio"）
 *   // 导航组
 *   auto nav = SidebarGroup::create()
 *       .set_label("Navigation")
 *       .set_show_accent(true);
 *   nav->add_child(std::move(dash_btn));
 *   sidebar->add_group(std::move(nav));
 *
 *   // 项目组
 *   auto proj = SidebarGroup::create()
 *       .set_label("Recent Projects")
 *       .set_accent_color(NanColor::from(NanRgb{95, 200, 130}));
 *   sidebar->add_group(std::move(proj));
 *
 *   // Footer 自动创建
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
                group->set_label("Navigation")
                    .set_show_accent(true);
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
                group->set_label("Recent Projects");
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
            if (m_header_label) {
                m_header_label->set_text(text);
            }
            mark_layout_dirty();
            return *this;
        }

        auto set_user_name(std::string_view text) -> Sidebar& {
            m_user_name_text = text;
            if (m_user_name_label) {
                m_user_name_label->set_text(text);
            }
            mark_layout_dirty();
            return *this;
        }

        auto set_user_role(std::string_view text) -> Sidebar& {
            m_user_role_text = text;
            if (m_user_role_label) {
                m_user_role_label->set_text(text);
            }
            mark_layout_dirty();
            return *this;
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

            if (m_divider_header_y > 0.0f) {
                draw_divider(m_divider_header_y);
            }
            if (m_divider_footer_y > 0.0f) {
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

            // Logo（双圆：外环 + 内点）
            auto logo = Icon::create();
            logo->set_type(IconType::Circle)
                .set_size(28.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{99, 102, 241}));
            m_header_logo = logo.get();

            // Header 标题
            auto hdr = Label::create();
            hdr->set_text("Nandina Studio")
                .set_font_size(11.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{220, 220, 240}));
            m_header_label = hdr.get();

            auto header_label_padding = layout::Padding::Create();
            header_label_padding->padding(0.0f, 2.0f, 0.0f, 4.0f)
                .child(std::move(hdr));

            auto header_row = layout::Row::Create();
            header_row->align_items(layout::LayoutAlignment::center)
                .gap(9.0f);

            auto logo_box = layout::SizedBox::Create();
            logo_box->width(28.0f)
                .height(28.0f)
                .child(std::move(logo));
            header_row->add(std::move(logo_box));

            auto header_label_expanded = layout::Expanded::Create();
            header_label_expanded->child(std::move(header_label_padding));
            header_row->add(std::move(header_label_expanded));

            auto header_padding = layout::Padding::Create();
            header_padding->padding(18.0f, 16.0f, 15.0f, 6.0f)
                .child(std::move(header_row));

            auto header_slot = layout::SizedBox::Create();
            header_slot->height(50.0f)
                .child(std::move(header_padding));
            m_header_slot = header_slot.get();
            root_column->add(std::move(header_slot));

            auto content_layout = layout::Column::Create();
            content_layout->align_items(layout::LayoutAlignment::stretch)
                .gap(2.0f);
            m_content_layout = content_layout.get();

            auto content_padding = layout::Padding::Create();
            content_padding->padding(0.0f, 4.0f, 0.0f, 0.0f)
                .child(std::move(content_layout));

            auto content_expanded = layout::Expanded::Create();
            content_expanded->child(std::move(content_padding));
            m_content_slot = content_expanded.get();
            root_column->add(std::move(content_expanded));

            // 用户头像
            auto avatar = Icon::create();
            avatar->set_type(IconType::Circle)
                .set_size(28.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{147, 150, 255}));
            m_user_avatar = avatar.get();

            // 用户名
            auto un = Label::create();
            un->set_text("CvRain")
                .set_font_size(9.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{220, 220, 240}));
            m_user_name_label = un.get();

            // 用户角色
            auto ur = Label::create();
            ur->set_text("Developer")
                .set_font_size(7.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{110, 112, 130}));
            m_user_role_label = ur.get();

            auto avatar_box = layout::SizedBox::Create();
            avatar_box->width(28.0f)
                .height(28.0f)
                .child(std::move(avatar));

            auto name_box = layout::SizedBox::Create();
            name_box->height(16.0f)
                .child(std::move(un));

            auto role_box = layout::SizedBox::Create();
            role_box->height(14.0f)
                .child(std::move(ur));

            auto info_column = layout::Column::Create();
            info_column->align_items(layout::LayoutAlignment::stretch);
            info_column->add(std::move(name_box));
            info_column->add(std::move(role_box));

            auto info_expanded = layout::Expanded::Create();
            info_expanded->child(std::move(info_column));

            auto footer_row = layout::Row::Create();
            footer_row->align_items(layout::LayoutAlignment::start)
                .gap(8.0f);
            footer_row->add(std::move(avatar_box));
            footer_row->add(std::move(info_expanded));

            auto footer_padding = layout::Padding::Create();
            footer_padding->padding(16.0f, 12.0f, 12.0f, 28.0f)
                .child(std::move(footer_row));

            auto footer_slot = layout::SizedBox::Create();
            footer_slot->height(70.0f)
                .child(std::move(footer_padding));
            m_footer_slot = footer_slot.get();
            root_column->add(std::move(footer_slot));

            add_child(std::move(root_column));
        }

        std::string m_header_title_text{"Nandina Studio"};
        std::string m_user_name_text{"CvRain"};
        std::string m_user_role_text{"Developer"};

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
