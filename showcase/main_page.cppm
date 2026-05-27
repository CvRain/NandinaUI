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
import nandina.widgets;
import nandina.theme;

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

        auto label = app::label()
                .text("Hello, NandinaUI!")
                .align(widgets::TextAlign::Center)
                .color(NanColor::from(NanRgb{"#cdd6f4"}));

        auto left_sidebar = widgets::Sidebar::create();
        left_sidebar->set_header_title("Menu");

        return mount(center(adopt(std::move(left_sidebar))));
    }
}
