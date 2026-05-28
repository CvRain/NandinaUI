//
// Created by cvrain on 2026/5/26.
//
module;

#include <string>
#include <string_view>

export module nandina.showcase.main_page;

import nandina.app.authoring;
import nandina.foundation.color;
import nandina.foundation.nan_insets;
import nandina.layout.container;
import nandina.layout.flex_widgets;
import nandina.text;
import nandina.widgets;

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

        return mount(column(children(
            app::label()
                .text("Welcome to NandinaUI")
                .align(widgets::TextAlign::Start)
                .font(text::NanFont{}
                    .size(24)
                    .weight(text::NanFontWeight::bold))
                .color(NanColor::from(NanRgb{"#cdd6f4"})),
            app::label()
                .text("The showcase shell now routes pages through Sidebar + PageHost, so this page only describes content instead of embedding its own navigation layout.")
                .align(widgets::TextAlign::Start)
                .color(NanColor::from(NanRgb{"#a6adc8"})),
            app::panel(children(
                app::label()
                    .text("Current validation focus")
                    .align(widgets::TextAlign::Start)
                    .font(text::NanFont{}.weight(text::NanFontWeight::semiBold))
                    .color(NanColor::from(NanRgb{"#cdd6f4"})),
                app::label()
                    .text("Authoring/layout primitives, page hosting, router-driven sidebar generation, and component composition are all exercised from the same executable path.")
                    .align(widgets::TextAlign::Start)
                    .color(NanColor::from(NanRgb{"#a6adc8"}))
            )).title("Showcase Shell").padding(16.0f),
            app::card(children(
                app::label()
                    .text("Next steps")
                    .align(widgets::TextAlign::Start)
                    .font(text::NanFont{}.weight(text::NanFontWeight::semiBold))
                    .color(NanColor::from(NanRgb{"#cdd6f4"})),
                app::label()
                    .text("Add more dedicated component pages and keep tests focused on structure and routing semantics rather than handwritten geometry.")
                    .align(widgets::TextAlign::Start)
                    .color(NanColor::from(NanRgb{"#a6adc8"}))
            )).title("Roadmap").padding(16.0f)
        )).gap(16).padding(32));
    }
}
