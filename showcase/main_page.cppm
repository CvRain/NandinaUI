//
// Created by cvrain on 2026/5/26.
//
module;

#include <string>
#include <string_view>

export module nandina.showcase.main_page;

import nandina.app.application;
import nandina.app.authoring;
import nandina.app.page;
import nandina.foundation.color;
import nandina.layout.container;
import nandina.layout.flex_widgets;
import nandina.text;
import nandina.theme;
import nandina.widgets;

export namespace nandina::showcase {
class MainPage final: public nandina::app::NanPage {
public:
    [[nodiscard]] auto route_key() const noexcept -> ::std::string_view override;

    [[nodiscard]] auto title() const noexcept -> ::std::string_view override;

    [[nodiscard]] auto build() -> nandina::app::NanComponent::Ptr override;

private:
    nandina::app::Var<bool> is_dark_mode { false };
    // nandina::app::ComputedVar<std::string> toggle_button_title{[this] {
    //     return is_dark_mode() ? "Dark" : "Light";
    // }};
};
} // namespace nandina::showcase

namespace nandina::showcase {
auto MainPage::route_key() const noexcept -> std::string_view {
    return "showcase::main";
}

auto MainPage::title() const noexcept -> std::string_view {
    return "Main Page";
}

auto MainPage::build() -> nandina::app::NanComponent::Ptr {
    using namespace nandina;
    using namespace nandina::app;

    const auto title_font_style =
        widgets::TextStyle { .text_color = NanColor::from(nandina::NanRgb { "#c6d0f5" }),
                             .font_size = 20,
                             .font_weight = text::NanFontWeight::bold };

    auto basic_card = card(children(
        column(
            children(
                label().text("Hello NandinaUI").style(title_font_style),
                label()
                    .text(
                        "Nandina是一个基于C++26的现代化UI框架，旨在提供高性能、易用且功能丰富的UI开发体验。"
                    )
                    .style(
                        widgets::TextStyle { .text_color =
                                                 NanColor::from(nandina::NanRgb { "#abb2bf" }) }
                    )
            )
        )
            .gap(12)
            .padding(5)
    ));

    auto section_title = [](std::string_view t) {
        return app::label(t).style(
            {
                .font_size = 15,
                .font_weight = nandina::text::NanFontWeight::semiBold,
                .text_color = NanColor::from(NanRgb { "#cdd6f4" }),
            }
        );
    };

    auto button_group_demo = card(children(
        column(
            children(
                section_title("Button Groups"),
                label("Buttons that wrap to the next line.").as_muted(),
                flow(children(
                         button("Primary Action").variant(widgets::ButtonVariant::default_variant),
                         button("Secondary").variant(widgets::ButtonVariant::secondary),
                         button("Outline").variant(widgets::ButtonVariant::outline),
                         button("Ghost").variant(widgets::ButtonVariant::ghost),
                         button("Destructive").variant(widgets::ButtonVariant::destructive),
                         button("Link").variant(widgets::ButtonVariant::link),
                         button("Disabled").disabled(true),
                         button("Loading").loading(true)
                     ))
                    .gap(8)
                    .line_gap(8)
            )
        )
            .gap(12)
    ));

    return app::mount((column(children(basic_card, button_group_demo)).padding(10)));
}
} // namespace nandina::showcase
