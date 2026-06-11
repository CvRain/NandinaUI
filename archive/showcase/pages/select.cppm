module;

#include <memory>
#include <string_view>

export module nandina.showcase.page.select;

import nandina.app.application;
import nandina.app.authoring;
import nandina.app.page;
import nandina.foundation.color;
import nandina.layout.container;
import nandina.text;
import nandina.widgets;

export namespace nandina::showcase {
class SelectPage final : public nandina::app::NanPage {
public:
    [[nodiscard]] auto route_key() const noexcept -> ::std::string_view override;
    [[nodiscard]] auto title() const noexcept -> ::std::string_view override;
    [[nodiscard]] auto build() -> app::NanComponent::Ptr override;

private:
    const std::string_view page_path{"showcase::page::select"};
    const std::string_view page_title{"Select Showcase"};
};
}

namespace nandina::showcase {
auto SelectPage::route_key() const noexcept -> std::string_view {
    return page_path;
}

auto SelectPage::title() const noexcept -> std::string_view {
    return page_title;
}

auto SelectPage::build() -> app::NanComponent::Ptr {
    using namespace nandina::app;
    using nandina::layout::LayoutAlignment;

    auto density = nandina::widgets::Select::create();
    density->add_option("Compact", "compact")
        .add_option("Comfortable", "comfortable")
        .add_option("Spacious", "spacious");

    auto theme = nandina::widgets::Select::create();
    theme->add_option("System", "system")
        .add_option("Light", "light")
        .add_option("Dark", "dark")
        .selected_index(1);

    return mount(column(children(
        label("Select")
            .style({
                .font_size = 24,
                .font_weight = text::NanFontWeight::bold,
                .text_color = NanColor::from(NanRgb{"#caddf5"}),
            }),
        label("Dropdown selection built on the same local overlay sizing contract as Popover and Dialog.")
            .style({
                .font_size = 16,
                .font_weight = text::NanFontWeight::medium,
                .text_color = NanColor::from(NanRgb{"#cdd6f4"}),
            }),
        card(children(column(children(
            label("Density"),
            sized_box(adopt(std::move(density))).height(180),
            label("Theme"),
            sized_box(adopt(std::move(theme))).height(180)
        )).gap(12).align_items(LayoutAlignment::stretch))).padding(18)
    )).gap(16).align_items(LayoutAlignment::stretch).padding(24.0f));
}
}
