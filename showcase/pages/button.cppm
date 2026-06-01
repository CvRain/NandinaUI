//
// Created by cvrain on 2026/5/27.
//
module;

#include <string>
#include <string_view>

export module nandina.showcase.page.button;

import nandina.app.application;
import nandina.app.authoring;
import nandina.app.page;
import nandina.foundation.color;
import nandina.layout.container;
import nandina.text;
import nandina.theme;
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
        using namespace nandina::app;
        using nandina::theme::ColorVariant;
        using nandina::layout::LayoutAlignment;
        using nandina::theme::ButtonSize;
        using nandina::theme::ButtonVariant;
        using nandina::widgets::IconType;

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

        auto variants_title = label("Variants")
            .align(widgets::TextAlign::Start)
            .font(text::NanFont{}.size(18).weight(text::NanFontWeight::semiBold))
            .color(NanColor::from(NanRgb{"#cdd6f4"}));

        auto variants = row(children(
            button("Primary"),
            button("Secondary").variant(ButtonVariant::secondary),
            button("Outline").variant(ButtonVariant::outline),
            button("Ghost").variant(ButtonVariant::ghost),
            button("Danger").variant(ButtonVariant::destructive),
            button("Link").variant(ButtonVariant::link)
        )).gap(12);

        auto sizes_title = label("Sizes")
            .align(widgets::TextAlign::Start)
            .font(text::NanFont{}.size(18).weight(text::NanFontWeight::semiBold))
            .color(NanColor::from(NanRgb{"#cdd6f4"}));

        auto sizes = row(children(
            button("XS").size(ButtonSize::xs),
            button("SM").size(ButtonSize::sm),
            button("MD").size(ButtonSize::md),
            button("LG").size(ButtonSize::lg),
            button().size(ButtonSize::icon).icon(IconType::Check)
        )).gap(12);

        auto icons_title = label("With Icons")
            .align(widgets::TextAlign::Start)
            .font(text::NanFont{}.size(18).weight(text::NanFontWeight::semiBold))
            .color(NanColor::from(NanRgb{"#cdd6f4"}));

        auto icon_examples = row(children(
            button("Save").icon_left(IconType::Check),
            button("More").variant(ButtonVariant::outline).icon_right(IconType::ArrowDown),
            button("Sync").variant(ButtonVariant::secondary).icon_left(IconType::ArrowUp).icon_right(IconType::ArrowDown)
        )).gap(12);

        auto states_title = label("States")
            .align(widgets::TextAlign::Start)
            .font(text::NanFont{}.size(18).weight(text::NanFontWeight::semiBold))
            .color(NanColor::from(NanRgb{"#cdd6f4"}));

        auto states = row(children(
            button("Loading").loading(true).icon_left(IconType::Dots),
            button("Disabled").disabled(true).variant(ButtonVariant::outline),
            button("Busy").loading(true).variant(ButtonVariant::ghost)
        )).gap(12);

        auto colors_title = label("Color Variants")
            .align(widgets::TextAlign::Start)
            .font(text::NanFont{}.size(18).weight(text::NanFontWeight::semiBold))
            .color(NanColor::from(NanRgb{"#cdd6f4"}));

        auto colors = row(children(
            button("Primary").variant(ButtonVariant::outline).color_variant(ColorVariant::primary),
            button("Secondary").variant(ButtonVariant::outline).color_variant(ColorVariant::secondary),
            button("Neutral").variant(ButtonVariant::outline).color_variant(ColorVariant::neutral),
            button("Danger").variant(ButtonVariant::outline).color_variant(ColorVariant::destructive)
        )).gap(12);

        return mount(column(children(
            page_title,
            page_subtitle,
            variants_title,
            variants,
            sizes_title,
            sizes,
            icons_title,
            icon_examples,
            states_title,
            states,
            colors_title,
            colors
        ))
        .gap(12)
        .align_items(LayoutAlignment::start)
        .padding(24.0f));
    }
}
