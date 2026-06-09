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

export namespace nandina::showcase
{
    class MainPage final: public nandina::app::NanPage {
    public:
        [[nodiscard]] auto route_key() const noexcept -> ::std::string_view override;

        [[nodiscard]] auto title() const noexcept -> ::std::string_view override;

        [[nodiscard]] auto build() -> nandina::app::NanComponent::Ptr override;
    };

} // namespace nandina::showcase

namespace nandina::showcase
{
    auto MainPage::route_key() const noexcept -> std::string_view {
        return "showcase::main";
    }

    auto MainPage::title() const noexcept -> std::string_view {
        return "Main Page";
    }

    auto MainPage::build() -> nandina::app::NanComponent::Ptr {
        using namespace nandina;
        using namespace nandina::app;

        auto basic_card = card(children(
            column(children(
                       label()
                           .text("Hello NandinaUI")
                           .style(
                               widgets::TextStyle {
                                   .font_size = 26,
                                   .font_weight = text::NanFontWeight::bold,
                                   .text_color = NanColor::from(nandina::NanRgb {"#e64553"})
                               }
                           ),
                       spacer(0).height(8),
                       label()
                           .text(
                               "Nandina是一个基于C++"
                               "26的现代化UI框架，旨在提供高性能、易用且功能丰"
                               "富的UI开发体验。"
                               "Nandina提供了一套统一的设计语言和结构化的框架"
                               "，用于控制产品的外观和用户体验。"
                               "它作为一个带有主观意见的设计系统，旨在大大减少"
                               "管理设计元素和模式所需的时间，使您能够更快地在"
                               "大规模上构建和管理前端界面。"
                           )
                           .style(
                               widgets::TextStyle {
                                   .text_color = NanColor::from(nandina::NanRgb {"#4c4f69"})
                               }
                           )
                   ))
                .gap(12)
                .padding(5)
        ));

        auto how_to_use_card = card(children(
            column(children(
                       label()
                           .text("How to Use This Showcase")
                           .style(
                               widgets::TextStyle {
                                   .font_size = 20,
                                   .font_weight = text::NanFontWeight::bold,
                                   .text_color = NanColor::from(nandina::NanRgb {"#e64553"})
                               }
                           ),
                       spacer(0).height(8),
                       label()
                           .text(
                               "Use the sidebar on the left to navigate through "
                               "different component demos. "
                               "Each page demonstrates a specific UI component or "
                               "pattern, showcasing its features and usage examples."
                           )
                           .style(
                               widgets::TextStyle {
                                   .text_color = NanColor::from(nandina::NanRgb {"#4c4f69"})
                               }
                           )
                   ))
                .gap(12)
                .padding(5)
        ));

        return app::mount((column(children(basic_card, how_to_use_card)).gap(10).padding(10)));
    }
} // namespace nandina::showcase
