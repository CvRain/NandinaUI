module;

#include <memory>
#include <string_view>

export module nandina.showcase.page.dialog;

import nandina.app.application;
import nandina.app.authoring;
import nandina.app.page;
import nandina.foundation.color;
import nandina.layout.container;
import nandina.text;
import nandina.theme;
import nandina.widgets;

export namespace nandina::showcase {
class DialogPage final : public nandina::app::NanPage {
public:
    [[nodiscard]] auto route_key() const noexcept -> ::std::string_view override;
    [[nodiscard]] auto title() const noexcept -> ::std::string_view override;
    [[nodiscard]] auto build() -> app::NanComponent::Ptr override;

private:
    const std::string_view page_path{"showcase::page::dialog"};
    const std::string_view page_title{"Dialog Showcase"};
};
}

namespace nandina::showcase {
auto DialogPage::route_key() const noexcept -> std::string_view {
    return page_path;
}

auto DialogPage::title() const noexcept -> std::string_view {
    return page_title;
}

auto DialogPage::build() -> app::NanComponent::Ptr {
    using namespace nandina::app;
    using nandina::layout::LayoutAlignment;

    auto dialog = nandina::widgets::Dialog::create();
    auto trigger = nandina::widgets::Button::create();
    trigger->text("Open Dialog");

    dialog->trigger(std::move(trigger))
        .content(nandina::app::mount(column(children(
            label("Confirm Project Settings")
                .style({
                    .font_size = 20,
                    .font_weight = text::NanFontWeight::bold,
                    .text_color = NanColor::from(NanRgb{"#1e1e2e"}),
                }),
            label("This dialog is container-local for now. It validates modal layering, backdrop dismissal, and ESC close before the future portal layer lands.")
                .style({
                    .font_size = 14,
                    .font_weight = text::NanFontWeight::regular,
                    .text_color = NanColor::from(NanRgb{"#4c4f69"}),
                }),
            row(children(
                button("Cancel").variant(theme::ButtonVariant::outline),
                button("Continue").variant(theme::ButtonVariant::default_variant)
            )).gap(10).justify_content(LayoutAlignment::end)
        )).gap(14).align_items(LayoutAlignment::stretch)));

    return mount(column(children(
        label("Dialog")
            .style({
                .font_size = 24,
                .font_weight = text::NanFontWeight::bold,
                .text_color = NanColor::from(NanRgb{"#caddf5"}),
            }),
        label("A container-local modal dialog with backdrop and keyboard dismissal.")
            .style({
                .font_size = 16,
                .font_weight = text::NanFontWeight::medium,
                .text_color = NanColor::from(NanRgb{"#cdd6f4"}),
            }),
        card(children(sized_box(adopt(std::move(dialog))).height(440))).padding(18)
    )).gap(16).align_items(LayoutAlignment::stretch).padding(24.0f));
}
}
