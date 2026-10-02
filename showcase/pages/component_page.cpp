#include "component_page.hpp"

#include <nandina/widget/controls.hpp>

#include <string>

namespace nandina::showcase
{
    auto ComponentPage::build(app::PageContext& context) -> widget::View {
        const auto& ui = context.ui();
        const auto& component = component_info(params().component);
        const auto& category = category_info(component.category);

        auto metadata = ui.row()
                            .gap(8.0F)
                            .cross_alignment(widget::LayoutAlignment::center)
                            .children(
                                ui.make<widget::Badge>(std::string(category.name)),
                                ui.make<widget::Badge>(component.experimental ? "实验性" : "可用")
                            );

        auto content = ui.column()
                           .width(widget::authoring::fill)
                           .gap(16.0F)
                           .cross_alignment(widget::LayoutAlignment::start)
                           .children(
                               ui.make<widget::Label>(std::string(component.name))
                                   .font_size(34.0F)
                                   .color_token(theme::ColorToken::foreground),
                               metadata,
                               ui.make<widget::Divider>().width(widget::authoring::fill),
                               ui.make<widget::Label>(std::string(component.description))
                                   .width(widget::authoring::fill)
                                   .font_size(16.0F)
                                   .color_token(theme::ColorToken::muted_foreground)
                                   .configure([](widget::Label& label) {
                                       label.set_overflow(widget::primitives::TextOverflow::wrap);
                                       label.set_max_lines(4);
                                   })
                           );

        return ui.scroll_view()
            .width(widget::authoring::fill)
            .height(widget::authoring::fill)
            .child(ui.padding(foundation::NanInsets::all(32.0F)).child(content))
            .build();
    }
} // namespace nandina::showcase
