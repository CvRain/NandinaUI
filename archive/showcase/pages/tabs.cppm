module;

#include <memory>
#include <string_view>

export module nandina.showcase.page.tabs;

import nandina.app.application;
import nandina.app.authoring;
import nandina.app.page;
import nandina.foundation.color;
import nandina.layout.container;
import nandina.text;
import nandina.theme;
import nandina.widgets;

export namespace nandina::showcase {
class TabsPage final : public nandina::app::NanPage {
public:
    [[nodiscard]] auto route_key() const noexcept -> ::std::string_view override;
    [[nodiscard]] auto title() const noexcept -> ::std::string_view override;
    [[nodiscard]] auto build() -> app::NanComponent::Ptr override;

private:
    const std::string_view page_path{"showcase::page::tabs"};
    const std::string_view page_title{"Tabs Showcase"};
};
}

namespace nandina::showcase {
auto TabsPage::route_key() const noexcept -> std::string_view {
    return page_path;
}

auto TabsPage::title() const noexcept -> std::string_view {
    return page_title;
}

auto TabsPage::build() -> app::NanComponent::Ptr {
    using namespace nandina::app;
    using nandina::layout::LayoutAlignment;

    auto tabs = nandina::widgets::Tabs::create();

    auto overview = nandina::widgets::Card::create();
    overview->set_title("Overview").set_description("Summary of project health").set_show_accent(true);
    overview->add_child(nandina::app::mount(column(children(
        label("Tabs provide a compact way to switch between related views.")
            .style({
                .font_size = 15,
                .font_weight = text::NanFontWeight::medium,
                .text_color = NanColor::from(NanRgb{"#4c4f69"}),
            }),
        row(children(
            tag("Stable").color_variant(theme::ColorVariant::secondary),
            tag("Composable").color_variant(theme::ColorVariant::primary),
            tag("Primitive-driven").color_variant(theme::ColorVariant::neutral)
        )).gap(10)
    )).gap(12)));

    auto metrics = nandina::widgets::Card::create();
    metrics->set_title("Metrics").set_description("Example structured content");
    metrics->add_child(nandina::app::mount(row(children(
        card(children(label("42 components").style({.font_size = 20, .font_weight = text::NanFontWeight::bold}))).padding(14),
        card(children(label("86 widget tests").style({.font_size = 20, .font_weight = text::NanFontWeight::bold}))).padding(14),
        card(children(label("70 control tests").style({.font_size = 20, .font_weight = text::NanFontWeight::bold}))).padding(14)
    )).gap(12)));

    auto settings = nandina::widgets::Card::create();
    settings->set_title("Settings").set_description("Interactive controls inside tab content");
    settings->add_child(nandina::app::mount(column(children(
        checkbox("Enable optimistic layout caching").checked(true),
        switch_button("Use semantic theme tokens").checked(true),
        button("Apply changes").variant(theme::ButtonVariant::default_variant)
    )).gap(12).align_items(LayoutAlignment::stretch)));

    tabs->add_tab("Overview", std::move(overview))
        .add_tab("Metrics", std::move(metrics))
        .add_tab("Settings", std::move(settings));

    return mount(column(children(
        label("Tabs")
            .style({
                .font_size = 24,
                .font_weight = text::NanFontWeight::bold,
                .text_color = NanColor::from(NanRgb{"#caddf5"}),
            }),
        label("Switch related content panes while keeping the surrounding page context stable.")
            .style({
                .font_size = 16,
                .font_weight = text::NanFontWeight::medium,
                .text_color = NanColor::from(NanRgb{"#cdd6f4"}),
            }),
        adopt(std::move(tabs))
    )).gap(16).align_items(LayoutAlignment::stretch).padding(24.0f));
}
}
