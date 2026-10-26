#include "preview_page.hpp"

#include "notes_page.hpp"

#include "lab_style.hpp"

#include "resource_keys.hpp"

#include <nandina/widget/controls.hpp>

#include <string>

namespace nandina::lab
{
    namespace
    {
        namespace resources = nandina::resource_keys::pkg_comDOTnandinaDOTlab;
    }

    auto PreviewPage::build(app::PageContext& context) -> widget::View {
        auto ui = context.ui();
        const auto navigation = context.navigation();

        const auto cover_key = "res://" + std::string {resources::RESOURCE_IMAGES_LAB_COVER_PNG};
        const auto logo_key = "res://" + std::string {resources::RESOURCE_IMAGES_LAB_LOGO_PNG};

        auto preview_card =
            ui.column()
                .width(widget::authoring::fill)
                .gap(10.0F)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(ui.make<widget::Image>(cover_key)
                              .width(widget::authoring::fill)
                              .height(340.0F)
                              .configure([](widget::Image& image) {
                                  image.set_scale_mode(widget::ImageScale::cover);
                              }))
                .children(caption(ui, cover_key + " — cover, fixed height"))
                .children(ui.make<widget::Image>(logo_key).width(96.0F).height(96.0F).configure(
                    [](widget::Image& image) { image.set_scale_mode(widget::ImageScale::contain); }
                ))
                .children(caption(ui, logo_key + " — contain, natural ratio"));

        auto content = page_column(ui)
                           .children(page_title(ui, "Preview"))
                           .children(body_text(
                               ui,
                               "The cover below is loaded from the application's resource "
                               "package. Its logical key comes from the generated header, so a "
                               "deleted resources.toml declaration becomes a compile error at "
                               "the call site.",
                               5
                           ))
                           .children(card(ui, preview_card))
                           .children(ui.row()
                                         .gap(8.0F)
                                         .cross_alignment(widget::LayoutAlignment::center)
                                         .children(ui.make<widget::Button>("Back to notes")
                                                       .treatment(theme::ButtonTreatment::tonal)
                                                       .on_click([navigation] {
                                                           (void)navigation.navigate<NotesPage>();
                                                       })));

        return ui.scroll_view()
            .width(widget::authoring::fill)
            .height(widget::authoring::fill)
            .child(ui.padding(foundation::NanInsets::all(PAGE_PADDING)).child(content))
            .build();
    }
} // namespace nandina::lab
