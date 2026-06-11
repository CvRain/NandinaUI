module;

#include <memory>
#include <string_view>

export module nandina.showcase.page.overlay;

import nandina.app.application;
import nandina.app.authoring;
import nandina.app.page;
import nandina.foundation.color;
import nandina.layout.container;
import nandina.text;
import nandina.theme;
import nandina.widgets;

export namespace nandina::showcase {
class OverlayPage final : public nandina::app::NanPage {
public:
    [[nodiscard]] auto route_key() const noexcept -> ::std::string_view override;
    [[nodiscard]] auto title() const noexcept -> ::std::string_view override;
    [[nodiscard]] auto build() -> app::NanComponent::Ptr override;

private:
    const std::string_view page_path{"showcase::page::overlay"};
    const std::string_view page_title{"Overlay Showcase"};
};
}

namespace nandina::showcase {
auto OverlayPage::route_key() const noexcept -> std::string_view {
    return page_path;
}

auto OverlayPage::title() const noexcept -> std::string_view {
    return page_title;
}

auto OverlayPage::build() -> app::NanComponent::Ptr {
    using namespace nandina::app;
    using nandina::layout::LayoutAlignment;

    auto popover = nandina::widgets::Popover::create();
    auto popover_trigger = nandina::widgets::Button::create();
    popover_trigger->text("Toggle Popover");
    popover->trigger(std::move(popover_trigger))
        .content(nandina::app::mount(column(children(
            label("Popover content stays inside the container bounds.")
                .style({
                    .font_size = 14,
                    .font_weight = text::NanFontWeight::medium,
                    .text_color = NanColor::from(NanRgb{"#4c4f69"}),
                }),
            button("Action").variant(theme::ButtonVariant::outline),
            checkbox("Keep open while editing")
        )).gap(10).align_items(LayoutAlignment::stretch)));

    auto tooltip = nandina::widgets::Tooltip::create();
    auto tooltip_trigger = nandina::widgets::Button::create();
    tooltip_trigger->text("Hover For Tooltip").variant(theme::ButtonVariant::outline);
    tooltip->trigger(std::move(tooltip_trigger));
    tooltip->text("Tooltips now use the same container overlay positioning path.");

    return mount(column(children(
        label("Overlay")
            .style({
                .font_size = 24,
                .font_weight = text::NanFontWeight::bold,
                .text_color = NanColor::from(NanRgb{"#caddf5"}),
            }),
        label("Container-local popover and tooltip infrastructure for anchored overlays.")
            .style({
                .font_size = 16,
                .font_weight = text::NanFontWeight::medium,
                .text_color = NanColor::from(NanRgb{"#cdd6f4"}),
            }),
        card(children(sized_box(adopt(std::move(popover))).height(220))).padding(18),
        card(children(sized_box(adopt(std::move(tooltip))).height(120))).padding(18)
    )).gap(16).align_items(LayoutAlignment::stretch).padding(24.0f));
}
}
