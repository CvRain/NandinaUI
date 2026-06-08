//
// SwitchPage — Switch 组件展示页
//

module;

#include <string_view>

export module nandina.showcase.page.switch_page;

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

class SwitchPage final: public nandina::app::NanPage {
public:
    [[nodiscard]] auto route_key() const noexcept -> std::string_view override;
    [[nodiscard]] auto title() const noexcept -> std::string_view override;
    [[nodiscard]] auto icon_type() const noexcept -> nandina::widgets::IconType override;
    [[nodiscard]] auto build() -> app::NanComponent::Ptr override;
};

} // namespace nandina::showcase

namespace nandina::showcase {

auto SwitchPage::route_key() const noexcept -> std::string_view {
    return "showcase::switch";
}

auto SwitchPage::title() const noexcept -> std::string_view {
    return "Switch";
}

auto SwitchPage::icon_type() const noexcept -> nandina::widgets::IconType {
    return nandina::widgets::IconType::Dot;
}

auto SwitchPage::build() -> app::NanComponent::Ptr {
    using namespace nandina::app;
    using nandina::layout::LayoutAlignment;
    using nandina::theme::ColorVariant;
    using nandina::theme::SwitchSize;

    auto section_title = [](const std::string_view& title) {
        return app::label(title).style(
            widgets::TextStyle {
                .font_size = 15,
                .font_weight = nandina::text::NanFontWeight::semiBold,
                .letter_spacing = 0.5,
                .text_color = NanColor::from(NanRgb { "#cdd6f4" }),
            }
        );
    };

    auto section_desc = [](const std::string_view& desc) { return app::label(desc).as_muted(); };

    // ═══════════════════════════════════════════════════════════
    // 1. Basic — 最简单的 Switch
    // ═══════════════════════════════════════════════════════════
    auto basic_demo = card(children(column(children(
                                               section_title("Basic"),
                                               section_desc("A switch control with label."),
                                               switch_button("Airplane Mode")
                                           ))
                                        .gap(12)));

    // ═══════════════════════════════════════════════════════════
    // 2. Checked State — 选中状态
    // ═══════════════════════════════════════════════════════════
    auto checked_demo = card(children(column(children(
                                                 section_title("Checked State"),
                                                 section_desc("Controlled on/off states."),
                                                 switch_button("Wi-Fi").checked(true),
                                                 switch_button("Bluetooth").checked(false)
                                             ))
                                          .gap(12)));

    // ═══════════════════════════════════════════════════════════
    // 3. Color Variants — 颜色变体
    // ═══════════════════════════════════════════════════════════
    auto variant_demo = card(children(
        column(
            children(
                section_title("Color Variants"),
                section_desc("Semantic color variants."),
                switch_button("Primary").checked(true).color_variant(ColorVariant::primary),
                switch_button("Secondary").checked(true).color_variant(ColorVariant::secondary),
                switch_button("Neutral").checked(true).color_variant(ColorVariant::neutral),
                switch_button("Destructive").checked(true).color_variant(ColorVariant::destructive)
            )
        )
            .gap(12)
    ));

    // ═══════════════════════════════════════════════════════════
    // 4. Sizes — 尺寸
    // ═══════════════════════════════════════════════════════════
    auto size_demo = card(children(
        column(children(
                   section_title("Sizes"),
                   section_desc("Small and medium sizes."),
                   switch_button("Small size").size(SwitchSize::sm).checked(true),
                   switch_button("Medium size (default)").size(SwitchSize::md).checked(true)
               ))
            .gap(12)
    ));

    // ═══════════════════════════════════════════════════════════
    // 5. Disabled — 禁用态
    // ═══════════════════════════════════════════════════════════
    auto disabled_demo =
        card(children(column(children(
                                 section_title("Disabled"),
                                 section_desc("Disabled prevents interaction."),
                                 switch_button("Disabled off").disabled(true),
                                 switch_button("Disabled on").disabled(true).checked(true),
                                 switch_button("Disabled on + destructive")
                                     .disabled(true)
                                     .checked(true)
                                     .color_variant(ColorVariant::destructive)
                             ))
                          .gap(12)));

    return mount(column(children(
                            label("Switch").style(
                                {
                                    .font_size = 24,
                                    .font_weight = nandina::text::NanFontWeight::bold,
                                    .text_color = NanColor::from(NanRgb { "#cdd6f4" }),
                                }
                            ),
                            label("A toggle switch control.")
                                .style(
                                    {
                                        .font_size = 13,
                                        .text_color = NanColor::from(NanRgb { "#a6adc8" }),
                                    }
                                ),

                            basic_demo,
                            checked_demo,
                            variant_demo,
                            size_demo,
                            disabled_demo
                        ))
                     .align_items(LayoutAlignment::stretch)
                     .gap(20)
                     .padding(24));
}

} // namespace nandina::showcase
