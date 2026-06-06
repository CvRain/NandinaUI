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
import nandina.theme;
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
        using nandina::theme::ButtonVariant;
        using nandina::theme::ButtonSize;

        auto switch_theme_button = app::button()
                .variant(ButtonVariant::outline)
                .size(ButtonSize::sm)
                .text("Toggle Light/Dark")
                .on_click([] {
                    nandina::theme::ThemeManager::instance().toggle_scheme();
                });

        return mount(column(children(
            column(children(
                app::label()
                .text("Welcome to NandinaUI")
                .style({
                    .font_size = 24,
                    .font_weight = text::NanFontWeight::bold,
                    .text_color = NanColor::from(NanRgb{"#cdd6f4"}),
                }),
                app::label()
                .text(
                    "The showcase shell now routes pages through Sidebar + PageHost, so this page only describes content instead of embedding its own navigation layout.")
                .style({
                    .font_size = 13,
                    .text_color = NanColor::from(NanRgb{"#a6adc8"}),
                })
            )).gap(10),
            app::panel(children(
                column(children(
                    app::label()
                    .text("Current validation focus")
                    .style({
                        .font_weight = text::NanFontWeight::semiBold,
                        .text_color = NanColor::from(NanRgb{"#cdd6f4"}),
                    }),
                    app::label()
                    .text(
                        "Authoring/layout primitives, page hosting, router-driven sidebar generation, and component composition are all exercised from the same executable path.")
                    .style({
                        .font_size = 13,
                        .text_color = NanColor::from(NanRgb{"#a6adc8"}),
                    })
                )).gap(10)
            )).title("Showcase Shell").padding(18.0f),
            app::card(children(
                column(children(
                    app::label()
                    .text("Next steps")
                    .style({
                        .font_weight = text::NanFontWeight::semiBold,
                        .text_color = NanColor::from(NanRgb{"#cdd6f4"}),
                    }),
                    app::label()
                    .text(
                        "Add more dedicated component pages and keep tests focused on structure and routing semantics rather than handwritten geometry.")
                    .style({
                        .font_size = 13,
                        .text_color = NanColor::from(NanRgb{"#a6adc8"}),
                    })
                )).gap(10)
            )).title("Roadmap").padding(18.0f),
            switch_theme_button

        )).gap(24).padding(32));
    }
}
