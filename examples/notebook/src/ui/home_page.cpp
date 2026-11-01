#include "home_page.hpp"

#include "app/notebook_store.hpp"
#include "ui/auth/user_login_page.hpp"

#include <nandina/widget/controls.hpp>

#include <string>

namespace nan_example::notebook::ui
{
    auto HomePage::build(nandina::app::PageContext& context) -> nandina::widget::View {
        auto ui = context.ui();
        auto* store = &context.store<app::NotebookStore>();
        const auto navigation = context.navigation();
        const auto user = store->current_user().peek();

        auto title = ui.make<nandina::widget::Label>(
                             user ? "Welcome back, " + user->username : "No active account"
        )
                         .width(nandina::widget::authoring::fill)
                         .font_size(28)
                         .color_token(nandina::theme::ColorToken::card_foreground)
                         .build();
        auto detail = ui.make<nandina::widget::Label>(
                              user ? "Your notebook session is active."
                                   : "Sign in to open your notebook."
        )
                          .width(nandina::widget::authoring::fill)
                          .font_size(16)
                          .color_token(nandina::theme::ColorToken::muted_foreground)
                          .configure([](nandina::widget::Label& label) {
                              label.set_overflow(nandina::text::TextOverflow::wrap);
                              label.set_max_lines(2);
                          })
                          .build();

        auto action = user
            ? ui.make<nandina::widget::Button>("Sign out")
                  .treatment(nandina::theme::ButtonTreatment::tonal)
                  .on_click([store, navigation] {
                      store->sign_out();
                      (void)navigation.navigate<UserLoginPage>();
                  })
                  .build()
            : ui.make<nandina::widget::Button>("Go to sign in")
                  .treatment(nandina::theme::ButtonTreatment::filled)
                  .on_click([navigation] { (void)navigation.navigate<UserLoginPage>(); })
                  .build();

        auto content = ui.column()
                           .width(nandina::widget::authoring::fill)
                           .gap(14)
                           .children(title)
                           .children(detail)
                           .children(action);

        return ui.center()
            .child(ui.make<nandina::widget::Card>()
                       .width(560)
                       .height(300)
                       .child(ui.padding(nandina::foundation::NanInsets::all(32))
                                  .width(nandina::widget::authoring::fill)
                                  .child(content)))
            .build();
    }
} // namespace nan_example::notebook::ui
