//
// Created by cvrain on 2026/4/29.
//

module;

#include <memory>
#include <string>
#include <string_view>
#include <thorvg-1/thorvg.h>

export module nandina.widgets.sidebar;

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
            add_child(std::move(group));
            mark_dirty();
            return *this;
        }

        /// 添加菜单项到导航区域（快捷方式：自动创建 SidebarGroup）
        auto add_menu_item(SidebarMenuButton::Ptr item) -> Sidebar& {
            if (!m_content_group) {
                auto group = SidebarGroup::create();
                group->set_label("Navigation")
                      .set_show_accent(true);
                m_content_group = group.get();
                add_child(std::move(group));
            }
            m_content_group->add_child(std::move(item));
            mark_dirty();
            return *this;
        }

        /// 添加项目到项目区
        auto add_project_item(SidebarMenuButton::Ptr item) -> Sidebar& {
            if (!m_project_group) {
                auto group = SidebarGroup::create();
                group->set_label("Recent Projects");
                m_project_group = group.get();
                add_child(std::move(group));
            }
            m_project_group->add_child(std::move(item));
            mark_dirty();
            return *this;
        }

        // ── 属性 ──────────────────────────────────────────
        auto set_header_title(std::string_view text) -> Sidebar& {
            m_header_title_text = text;
            if (m_header_label) {
                m_header_label->set_text(text);
            }
            mark_dirty();
            return *this;
        }

        auto set_user_name(std::string_view text) -> Sidebar& {
            m_user_name_text = text;
            if (m_user_name_label) {
                m_user_name_label->set_text(text);
            }
            mark_dirty();
            return *this;
        }

        auto set_user_role(std::string_view text) -> Sidebar& {
            m_user_role_text = text;
            if (m_user_role_label) {
                m_user_role_label->set_text(text);
            }
            mark_dirty();
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

            // Header：Logo + 标题（固定顶部）
            const float header_h = 50.0f;
            if (m_header_logo) {
                m_header_logo->set_bounds(x + 18.0f, y + 16.0f, 28.0f, 28.0f);
            }
            if (m_header_label) {
                m_header_label->set_bounds(x + 55.0f, y + 18.0f, w - 70.0f, 22.0f);
            }

            // Footer：用户信息（固定底部）
            const float footer_h = 70.0f;
            const float footer_y = y + h - footer_h;
            if (m_user_avatar) {
                m_user_avatar->set_bounds(x + 16.0f, footer_y + 12.0f, 28.0f, 28.0f);
            }
            if (m_user_name_label) {
                m_user_name_label->set_bounds(x + 52.0f, footer_y + 12.0f, w - 64.0f, 18.0f);
            }
            if (m_user_role_label) {
                m_user_role_label->set_bounds(x + 52.0f, footer_y + 28.0f, w - 64.0f, 14.0f);
            }

            // 分割线位置
            m_divider_header_y = y + header_h;
            m_divider_footer_y = footer_y;

            // 内容区子节点（SidebarGroup）自动分布在中间区域
            // 由 Surface 的 for_each_child 定位，但我们需要手动管理
            // 让每个 group 的 bounds 从 header 下方到 footer 上方
            const float content_y = y + header_h + 4.0f;
            float cur_y = content_y;

            for_each_child([&](runtime::NanWidget& child) {
                if (&child == m_header_logo || &child == m_header_label
                    || &child == m_user_avatar || &child == m_user_name_label
                    || &child == m_user_role_label) {
                    return;
                }
                const float child_h = child.preferred_size().height();
                child.set_bounds(x, cur_y, w, child_h);
                cur_y += child_h + 2.0f;
            });

            return *this;
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

    private:
        Sidebar() : Surface() {
            m_bg_color.set(nandina::NanColor::from(nandina::NanRgb{42, 44, 62}));
            m_corner_radius.set(0.0f);
            m_padding.set(geometry::NanInsets{});

            // Logo（双圆：外环 + 内点）
            auto logo = Icon::create();
            logo->set_type(IconType::Circle)
                .set_size(28.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{99, 102, 241}));
            m_header_logo = logo.get();
            add_child(std::move(logo));

            // Header 标题
            auto hdr = Label::create();
            hdr->set_text("Nandina Studio")
                .set_font_size(11.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{220, 220, 240}));
            m_header_label = hdr.get();
            add_child(std::move(hdr));

            // 用户头像
            auto avatar = Icon::create();
            avatar->set_type(IconType::Circle)
                .set_size(28.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{147, 150, 255}));
            m_user_avatar = avatar.get();
            add_child(std::move(avatar));

            // 用户名
            auto un = Label::create();
            un->set_text("CvRain")
                .set_font_size(9.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{220, 220, 240}));
            m_user_name_label = un.get();
            add_child(std::move(un));

            // 用户角色
            auto ur = Label::create();
            ur->set_text("Developer")
                .set_font_size(7.0f)
                .set_color(nandina::NanColor::from(nandina::NanRgb{110, 112, 130}));
            m_user_role_label = ur.get();
            add_child(std::move(ur));
        }

        std::string m_header_title_text{"Nandina Studio"};
        std::string m_user_name_text{"CvRain"};
        std::string m_user_role_text{"Developer"};

        // Header 子节点
        Icon*  m_header_logo{nullptr};
        Label* m_header_label{nullptr};

        // 分组指针（内部自动管理）
        SidebarGroup* m_content_group{nullptr};
        SidebarGroup* m_project_group{nullptr};

        // Footer 子节点
        Icon*  m_user_avatar{nullptr};
        Label* m_user_name_label{nullptr};
        Label* m_user_role_label{nullptr};

        // 分割线 Y 坐标
        float m_divider_header_y{0.0f};
        float m_divider_footer_y{0.0f};
    };

} // namespace nandina::widgets
