//
// FlowPage — 流式布局展示页
//

module;

#include <string_view>

export module nandina.showcase.page.flow_page;

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

class FlowPage final: public nandina::app::NanPage {
public:
    [[nodiscard]] auto route_key() const noexcept -> std::string_view override;
    [[nodiscard]] auto title() const noexcept -> std::string_view override;
    [[nodiscard]] auto icon_type() const noexcept -> nandina::widgets::IconType override;
    [[nodiscard]] auto build() -> app::NanComponent::Ptr override;
};

} // namespace nandina::showcase

namespace nandina::showcase {

auto FlowPage::route_key() const noexcept -> std::string_view {
    return "showcase::flow";
}

auto FlowPage::title() const noexcept -> std::string_view {
    return "Flow";
}

auto FlowPage::icon_type() const noexcept -> nandina::widgets::IconType {
    return nandina::widgets::IconType::Dot;
}

auto FlowPage::build() -> app::NanComponent::Ptr {
    using namespace nandina::app;
    using nandina::layout::LayoutAlignment;
    using nandina::theme::ButtonVariant;

    auto section_title = [](std::string_view t) {
        return app::label(t).style(
            {
                .font_size = 15,
                .font_weight = nandina::text::NanFontWeight::semiBold,
                .text_color = NanColor::from(NanRgb { "#cdd6f4" }),
            }
        );
    };

    // ═══════════════════════════════════════════════════════════
    // 1. Basic Flow — 标签列表
    // ═══════════════════════════════════════════════════════════
    auto basic_demo = card(children(column(children(
                                               section_title("Basic Flow"),
                                               label("Tags with auto-wrap.").as_muted(),
                                               flow(children(
                                                        tag("JavaScript"),
                                                        tag("TypeScript"),
                                                        tag("React"),
                                                        tag("Vue"),
                                                        tag("Angular"),
                                                        tag("Svelte"),
                                                        tag("Solid"),
                                                        tag("Rust"),
                                                        tag("Go"),
                                                        tag("Python"),
                                                        tag("C++"),
                                                        tag("Zig"),
                                                        tag("OCaml"),
                                                        tag("Haskell")
                                                    ))
                                                   .gap(8)
                                                   .line_gap(6)
                                           ))
                                        .gap(12)));

    // ═══════════════════════════════════════════════════════════
    // 2. Button Groups — 按钮组自动换行
    // ═══════════════════════════════════════════════════════════
    auto button_group_demo = card(children(
        column(children(
                   section_title("Button Groups"),
                   label("Buttons that wrap to the next line.").as_muted(),
                   flow(children(
                            button("Primary Action").variant(ButtonVariant::default_variant),
                            button("Secondary").variant(ButtonVariant::secondary),
                            button("Outline").variant(ButtonVariant::outline),
                            button("Ghost").variant(ButtonVariant::ghost),
                            button("Destructive").variant(ButtonVariant::destructive),
                            button("Link").variant(ButtonVariant::link),
                            button("Disabled").disabled(true),
                            button("Loading").loading(true)
                        ))
                       .gap(8)
                       .line_gap(8)
               ))
            .gap(12)
    ));

    // ═══════════════════════════════════════════════════════════
    // 3. Mixed Sizes — 混合尺寸
    // ═══════════════════════════════════════════════════════════
    auto mixed_demo =
        card(children(column(children(
                                 section_title("Mixed Content"),
                                 label("Items of varying sizes wrap naturally.").as_muted(),
                                 flow(children(
                                          sized_box(spacer(0)).width(80).height(48),
                                          sized_box(spacer(0)).width(120).height(48),
                                          sized_box(spacer(0)).width(64).height(48),
                                          sized_box(spacer(0)).width(96).height(48),
                                          sized_box(spacer(0)).width(48).height(48),
                                          sized_box(spacer(0)).width(140).height(48),
                                          sized_box(spacer(0)).width(72).height(48)
                                      ))
                                     .gap(10)
                                     .line_gap(10)
                             ))
                          .gap(12)));

    // ═══════════════════════════════════════════════════════════
    // 4. Justify Content — 主轴对齐
    // ═══════════════════════════════════════════════════════════
    auto justify_demo = card(children(
        column(children(
                   section_title("Justify Content"),
                   label("Try shrinking the window — items wrap and align.").as_muted(),
                   flow(children(tag("A"), tag("B"), tag("C"), tag("D"), tag("E"), tag("F")))
                       .gap(8)
                       .line_gap(6)
                       .justify_content(LayoutAlignment::center)
               ))
            .gap(12)
    ));

    return mount(
        column(children(
                   label("Flow").style(
                       {
                           .font_size = 24,
                           .font_weight = nandina::text::NanFontWeight::bold,
                           .text_color = NanColor::from(NanRgb { "#cdd6f4" }),
                       }
                   ),
                   label("A layout container that wraps items to the next line when they overflow.")
                       .style(
                           {
                               .font_size = 13,
                               .text_color = NanColor::from(NanRgb { "#a6adc8" }),
                           }
                       ),

                   basic_demo,
                   button_group_demo,
                   mixed_demo,
                   justify_demo
               ))
            .align_items(LayoutAlignment::stretch)
            .gap(20)
            .padding(24)
    );
}

} // namespace nandina::showcase
