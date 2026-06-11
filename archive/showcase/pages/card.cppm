//
// CardPage — Card contract verification page
// Focused on fixed-size pressure tests and responsive layout behavior.
//

module;

#include <string_view>

export module nandina.showcase.page.card;

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

class CardPage final: public nandina::app::NanPage {
public:
    [[nodiscard]] auto route_key() const noexcept -> std::string_view override;
    [[nodiscard]] auto title() const noexcept -> std::string_view override;
    [[nodiscard]] auto icon_type() const noexcept -> nandina::widgets::IconType override;
    [[nodiscard]] auto build() -> app::NanComponent::Ptr override;
};

} // namespace nandina::showcase

namespace nandina::showcase {

auto CardPage::route_key() const noexcept -> std::string_view {
    return "showcase::card";
}

auto CardPage::title() const noexcept -> std::string_view {
    return "Card";
}

auto CardPage::icon_type() const noexcept -> nandina::widgets::IconType {
    return nandina::widgets::IconType::Dot;
}

auto CardPage::build() -> app::NanComponent::Ptr {
    using namespace nandina::app;
    using nandina::layout::LayoutAlignment;
    using nandina::theme::ButtonSize;
    using nandina::theme::ButtonVariant;
    using nandina::widgets::CardSize;

    auto muted = [](std::string_view t) {
        return label(t).style({
            .font_size = 13,
            .font_weight = nandina::text::NanFontWeight::regular,
            .overflow = nandina::text::TextOverflow::wrap,
            .wrap_policy = nandina::text::TextWrapPolicy::break_word,
            .single_line = false,
            .text_color = NanColor::from(NanRgb{"#a6adc8"}),
        });
    };

    auto section_title = [](std::string_view t) {
        return label(t).style({
            .font_size = 16,
            .font_weight = nandina::text::NanFontWeight::semiBold,
            .text_color = NanColor::from(NanRgb{"#cdd6f4"}),
        });
    };

    auto section_lead = [](std::string_view t) {
        return label(t).style({
            .font_size = 12,
            .font_weight = nandina::text::NanFontWeight::regular,
            .overflow = nandina::text::TextOverflow::wrap,
            .wrap_policy = nandina::text::TextWrapPolicy::break_word,
            .single_line = false,
            .text_color = NanColor::from(NanRgb{"#9399b2"}),
        });
    };

    auto card_caption = [](std::string_view t) {
        return label(t).style({
            .font_size = 12,
            .font_weight = nandina::text::NanFontWeight::medium,
            .text_color = NanColor::from(NanRgb{"#89b4fa"}),
        });
    };

    auto contract_body = [](std::string_view text) {
        using namespace nandina::app;
        return column(children(
            label("Contract body").style({
                .font_size = 12,
                .font_weight = nandina::text::NanFontWeight::semiBold,
                .text_color = NanColor::from(NanRgb{"#cdd6f4"}),
            }),
            label(text).style({
                .font_size = 13,
                .font_weight = nandina::text::NanFontWeight::regular,
                .overflow = nandina::text::TextOverflow::wrap,
                .wrap_policy = nandina::text::TextWrapPolicy::break_word,
                .single_line = false,
                .text_color = NanColor::from(NanRgb{"#a6adc8"}),
            })
        )).gap(8);
    };

    auto fixed_demo = [&card_caption](std::string_view caption, auto node, const float width, const float height) {
        using namespace nandina::app;
        return column(children(
            card_caption(caption),
            sized_box(std::move(node)).width(width).height(height)
        )).gap(10);
    };

    auto responsive_demo = [&card_caption](std::string_view caption, auto node) {
        using namespace nandina::app;
        return column(children(
            card_caption(caption),
            sized_box(std::move(node)).width(420)
        )).gap(10);
    };

    auto fixed_header_card = card(children(
        contract_body("The body text stays inside the allocated content section even when the card height is intentionally smaller than the total natural text height. This card should stress both header and content containment.")
    ))
        .title("Fixed header pressure test")
        .description("This description is intentionally verbose and narrow-card hostile so we can verify that break-word header wrapping no longer presses through the divider or leaks into the body section.")
        .header_action(tag("120x220").color_variant(nandina::theme::ColorVariant::secondary));

    auto fixed_footer_card = card(children(
        contract_body("This card reserves a footer and intentionally uses a constrained total height. The contract to verify is that long body text should not overpaint into the footer region or mix with the action row.")
    ))
        .title("Fixed footer separation")
        .description("Footer divider and footer content should remain readable even when the card is under vertical pressure.")
        .footer(row(children(
                    button("Dismiss")
                        .variant(ButtonVariant::ghost)
                        .size(ButtonSize::sm),
                    spacer(),
                    button("Apply")
                        .variant(ButtonVariant::default_variant)
                        .size(ButtonSize::sm)
                )).gap(8).align_items(LayoutAlignment::center));

    auto fixed_small_card = card(children(
        contract_body("The small-size variant should still keep its header, body, and footer rhythm coherent under constrained dimensions.")
    ))
        .title("Small size contract")
        .description("Compact spacing should help density without causing section overlap.")
        .size(CardSize::sm)
        .footer(row(children(
                    button("Later")
                        .variant(ButtonVariant::ghost)
                        .size(ButtonSize::sm),
                    spacer(),
                    button("Open")
                        .variant(ButtonVariant::outline)
                        .size(ButtonSize::sm)
                )).gap(8).align_items(LayoutAlignment::center));

    auto fixed_spacing_card = card(children(
        contract_body("This card uses explicit section spacing so we can verify that custom rhythm does not destabilize divider placement under a fixed height.")
    ))
        .title("Custom spacing under pressure")
        .description("Spacing is a semantic knob, but it still needs to remain bounded when the card is short.")
        .header_action(button("Inspect")
            .variant(ButtonVariant::ghost)
            .size(ButtonSize::sm))
        .card_spacing(14.0f)
        .footer(row(children(
                    tag("Fixed").color_variant(nandina::theme::ColorVariant::secondary),
                    spacer(),
                    button("Confirm")
                        .variant(ButtonVariant::default_variant)
                        .size(ButtonSize::sm)
                )).gap(8).align_items(LayoutAlignment::center));

    auto responsive_header_card = card(children(
        contract_body("This card should move from a comfortable wide layout into a tighter wrapped layout as the page narrows, without the header divider being crossed by title or description ink.")
    ))
        .title("Responsive header and description")
        .description("Observe how the card keeps the header readable as the container width changes, especially around narrow widths where word wrapping used to look unstable.")
        .header_action(tag("Adaptive").color_variant(nandina::theme::ColorVariant::secondary));

    auto responsive_action_card = card(children(
        contract_body("This example verifies the inline-to-stacked CardAction contract. On wider widths the action should live at the top-right; on narrower widths it should stack below the header text while staying visually contained.")
    ))
        .title("Responsive header action")
        .description("The action slot should not crush the text block into unreadable columns.")
        .header_action(button("Manage Workspace")
            .variant(ButtonVariant::ghost)
            .size(ButtonSize::sm));

    auto responsive_footer_card = card(children(
        contract_body("This body is intentionally long enough to compete with the footer area. The contract is that content remains visually separated from footer actions as width changes.")
    ))
        .title("Responsive footer contract")
        .description("Footer actions should remain terminal actions, not become part of the body text flow.")
        .footer(row(children(
                    button("Cancel")
                        .variant(ButtonVariant::ghost)
                        .size(ButtonSize::sm),
                    spacer(),
                    button("Save Changes")
                        .variant(ButtonVariant::default_variant)
                        .size(ButtonSize::sm)
                )).gap(8).align_items(LayoutAlignment::center));

    auto responsive_visual_card = card(children(
        column(children(
            label("Signals").style({
                .font_size = 12,
                .font_weight = nandina::text::NanFontWeight::semiBold,
                .text_color = NanColor::from(NanRgb{"#cdd6f4"}),
            }),
            row(children(
                    label("Members").as_muted(),
                    spacer(),
                    label("12 active").as_muted()
                )).align_items(LayoutAlignment::center),
            row(children(
                    label("Publishing").as_muted(),
                    spacer(),
                    label("Enabled").as_muted()
                )).align_items(LayoutAlignment::center),
            muted("This richer example is still contract-oriented: it verifies that mixed text rows, semantic header content, and footer actions remain visually separated under width changes.")
        )).gap(8)
    ))
        .title("Responsive semantic composition")
        .description("A realistic card should still read clearly when compressed by the page width.")
        .header_action(button("Open")
            .variant(ButtonVariant::ghost)
            .size(ButtonSize::sm))
        .show_accent(true)
        .accent_color(NanColor::from(NanRgb{"#cba6f7"}))
        .footer(row(children(
                    button("Close")
                        .variant(ButtonVariant::ghost)
                        .size(ButtonSize::sm),
                    spacer(),
                    button("Save")
                        .variant(ButtonVariant::default_variant)
                        .size(ButtonSize::sm)
                )).gap(8).align_items(LayoutAlignment::center));

    return mount(
        column(children(
            label("Card").style({
                .font_size = 24,
                .font_weight = nandina::text::NanFontWeight::bold,
                .text_color = NanColor::from(NanRgb{"#cdd6f4"}),
            }),
            muted("This page is no longer a generic component gallery. It is a contract verification page for Card under constrained height and changing width."),

            section_title("Fixed-size section"),
            section_lead("These cards are intentionally given fixed bounds to expose section containment problems. Long header text, long body text, and footer actions are all placed under vertical pressure."),
            flow(children(
                    fixed_demo("120 x 220", std::move(fixed_header_card), 220.0f, 120.0f),
                    fixed_demo("140 x 240", std::move(fixed_footer_card), 240.0f, 140.0f),
                    fixed_demo("128 x 220", std::move(fixed_small_card), 220.0f, 128.0f),
                    fixed_demo("136 x 240", std::move(fixed_spacing_card), 240.0f, 136.0f)
                )).gap(16).line_gap(16),

            section_title("Responsive section"),
            section_lead("These cards use wider initial widths but are hosted in flow layouts so the page itself becomes the pressure source. Resize behavior should remain legible without header/footer overlap."),
            flow(children(
                    responsive_demo("Header wrap", std::move(responsive_header_card)),
                    responsive_demo("Header action stack", std::move(responsive_action_card))
                )).gap(16).line_gap(16),
            flow(children(
                    responsive_demo("Footer separation", std::move(responsive_footer_card)),
                    responsive_demo("Semantic composition", std::move(responsive_visual_card))
                )).gap(16).line_gap(16)
        ))
            .align_items(LayoutAlignment::stretch)
            .gap(16)
            .padding(24)
    );
}

} // namespace nandina::showcase
