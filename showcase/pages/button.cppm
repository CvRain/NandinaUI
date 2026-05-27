//
// Created by cvrain on 2026/5/27.
//
module;

#include <string>
#include <string_view>

export module nandina.showcase.page.button;

import nandina.app.application;
import nandina.app.page;
import nandina.foundation.color;
import nandina.text;
import nandina.widgets;

export namespace nandina::showcase {
    class ButtonPage final : public nandina::app::NanPage {
    public:
        [[nodiscard]] auto route_key() const noexcept -> ::std::string_view override;

        [[nodiscard]] auto title() const noexcept -> ::std::string_view override;

        [[nodiscard]] auto build() -> app::NanComponent::Ptr override;

    private:
        const std::string_view page_path{"showcase::page::button"};
        const std::string_view page_title{"Button Showcase"};
    };
}

namespace nandina::showcase {
    auto ButtonPage::route_key() const noexcept -> std::string_view {
        return this->page_path;
    }

    auto ButtonPage::title() const noexcept -> std::string_view {
        return this->page_title;
    }

    auto ButtonPage::build() -> app::NanComponent::Ptr {
        auto page_title = app::label()
                .text("Button")
                .align(widgets::TextAlign::Start)
                .font(text::NanFont{}
                    .size(24)
                    .weight(text::NanFontWeight::bold))
                .color(NanColor::from(NanRgb{"#caddf5"}));

        auto page_subtitle = app::label()
                .text("Provide a variety of button or a component that looks like a button ")
                .align(widgets::TextAlign::Start)
                .font(text::NanFont{}
                    .size(16)
                    .weight(text::NanFontWeight::medium))
                .color(NanColor::from(NanRgb{"#caddf5"}));

        return app::mount(app::column(app::children(
            page_title,
            page_subtitle
        )).gap(12));
    }
}
