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


        // ═══════════════════════════════════════════════════════════
        // 侧边栏（仅内容分组，无 header/footer）
        // ═══════════════════════════════════════════════════════════
        auto sidebar = nandina::widgets::Sidebar::create();

        // Get Started 分组
        auto test_group = nandina::widgets::SidebarGroup::create();
        test_group->label("Test Group");
        test_group->head().font(text::NanFont{}
            .color(NanColor::from(NanRgb{"#f38ba8"}))
            .size(22)
        ).label().set_align(widgets::TextAlign::Start);

        test_group->add_child(sidebar_menu_button("Test Item 1"));
        test_group->add_child(sidebar_menu_button("Test Item 2"));
        sidebar->add_group(std::move(test_group));

        auto get_started = nandina::widgets::SidebarGroup::create();
        get_started->label("Get Started");
        get_started->add_child(sidebar_menu_button("Introduction").active(true));
        get_started->add_child(sidebar_menu_button("Installation"));
        get_started->add_child(sidebar_menu_button("Fundamentals"));
        sidebar->add_group(std::move(get_started));

        // Components 分组
        auto components = nandina::widgets::SidebarGroup::create();
        components->label("Components");
        components->add_child(sidebar_menu_button("Label"));
        components->add_child(sidebar_menu_button("Button"));
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

        return mount(row(children(
            sized_box(adopt(std::move(sidebar))).width(240),
            expanded(content)
        )).align_items(nandina::layout::LayoutAlignment::stretch));
    }
}
