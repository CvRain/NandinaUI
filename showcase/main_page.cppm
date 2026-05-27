//
// Created by cvrain on 2026/5/26.
//
module;

#include <string>
#include <string_view>

export module nandina.showcase.main_page;

import nandina.app.authoring;
import nandina.foundation.color;
import nandina.layout.container;
import nandina.layout.flex_widgets;
import nandina.text;
import nandina.widgets;
import nandina.widgets.sidebar;
import nandina.widgets.sidebar_group;
import nandina.widgets.sidebar_menu_button;

export namespace nandina::showcase {
    class MainPage final : public nandina::app::NanPage {
    public:
        [[nodiscard]] auto route_key() const noexcept -> ::std::string_view override;

        [[nodiscard]] auto title() const noexcept -> ::std::string_view override;

        [[nodiscard]] auto build() -> nandina::app::NanComponent::Ptr override;
    };
}


namespace nandina::showcase {
    auto MainPage::route_key() const noexcept -> std::string_view {
        return "showcase::main";
    }

    auto MainPage::title() const noexcept -> std::string_view {
        return "Main Page";
    }

    auto MainPage::build() -> nandina::app::NanComponent::Ptr {
        using namespace nandina::app;
        using nandina::widgets::IconType;

        // ═══════════════════════════════════════════════════════════
        // 侧边栏
        // ═══════════════════════════════════════════════════════════
        auto sidebar = nandina::widgets::Sidebar::create();
        sidebar->set_header_title("NandinaUI");

        // ── Get Started 分组 ──────────────────────────────────
        auto get_started = nandina::widgets::SidebarGroup::create();
        get_started->set_label("Get Started")
            .set_show_accent(true);

        auto intro_btn = nandina::widgets::SidebarMenuButton::create();
        intro_btn->set_label("Introduction")
            .set_icon_type(IconType::Dot);
        get_started->add_child(std::move(intro_btn));

        auto install_btn = nandina::widgets::SidebarMenuButton::create();
        install_btn->set_label("Installation")
            .set_icon_type(IconType::Dot);
        get_started->add_child(std::move(install_btn));

        auto fundamentals_btn = nandina::widgets::SidebarMenuButton::create();
        fundamentals_btn->set_label("Fundamentals")
            .set_icon_type(IconType::Dot);
        get_started->add_child(std::move(fundamentals_btn));

        sidebar->add_group(std::move(get_started));

        // ── Components 分组 ──────────────────────────────────
        auto components = nandina::widgets::SidebarGroup::create();
        components->set_label("Components");

        auto label_btn = nandina::widgets::SidebarMenuButton::create();
        label_btn->set_label("Label")
            .set_icon_type(IconType::Square);
        components->add_child(std::move(label_btn));

        auto button_btn = nandina::widgets::SidebarMenuButton::create();
        button_btn->set_label("Button")
            .set_icon_type(IconType::Square);
        components->add_child(std::move(button_btn));

        sidebar->add_group(std::move(components));

        // ═══════════════════════════════════════════════════════════
        // 内容区
        // ═══════════════════════════════════════════════════════════
        auto content = column(children(
            app::label()
                .text("Welcome to NandinaUI")
                .font(text::NanFont{}
                    .size(24)
                    .weight(text::NanFontWeight::bold))
                .color(NanColor::from(NanRgb{"#cdd6f4"})),
            app::label()
                .text("Select a topic from the sidebar to get started.")
                .color(NanColor::from(NanRgb{"#a6adc8"}))
        )).gap(12).padding(32);

        // ═══════════════════════════════════════════════════════════
        // 组合布局：侧边栏 | 内容
        // ═══════════════════════════════════════════════════════════
        return mount(row(children(
            sized_box(adopt(std::move(sidebar)))
                .width(240),
            expanded(content)
        )).align_items(nandina::layout::LayoutAlignment::stretch));
    }
}
