#include "user_register_page.hpp"

#include "../home_page.hpp"
#include "auth_shell.hpp"
#include "user_login_page.hpp"

#include "app/notebook_store.hpp"
#include "core/services/auth_service.hpp"
#include "data/data_error.hpp"
#include "nandina/foundation/nan_logger.hpp"

#include <nandina/widget/controls.hpp>

#include <string>

namespace nan_example::notebook::ui
{
    auto UserRegisterPage::build(nandina::app::PageContext& context) -> nandina::widget::View {
        auto ui = context.ui();
        auto* store = &context.store<app::NotebookStore>();
        const auto navigation = context.navigation();

        auto& user_name = ui.signal_value(std::string {});
        auto& password = ui.signal_value(std::string {});
        auto& confirmation = ui.signal_value(std::string {});
        auto& feedback = ui.signal_value(std::string {});
        auto& feedback_error = ui.signal_value(false);

        const auto password_hint =
            "at least " + std::to_string(core::AuthService::minimum_password_length)
            + " characters";
        const auto user_name_input =
            ui.make<nandina::widget::TextField>(user_name, "input your name")
                .width(nandina::widget::authoring::fill)
                .height(36)
                .configure([](nandina::widget::TextField& field) {
                    field.set_name("register-username");
                })
                .build();
        const auto password_input =
            ui.make<nandina::widget::TextField>(password, password_hint)
                .width(nandina::widget::authoring::fill)
                .height(36)
                .configure([](nandina::widget::TextField& field) {
                    field.set_name("register-password");
                })
                .build();
        const auto confirmation_input =
            ui.make<nandina::widget::TextField>(confirmation, "repeat the password")
                .width(nandina::widget::authoring::fill)
                .height(36)
                .configure([](nandina::widget::TextField& field) {
                    field.set_name("register-password-confirmation");
                })
                .build();

        auto submit_registration = [&user_name,
                                    &password,
                                    &confirmation,
                                    &feedback,
                                    &feedback_error,
                                    user_name_input,
                                    password_input,
                                    confirmation_input,
                                    store,
                                    navigation] {
            const bool name_ok = !user_name.peek().empty();
            const bool password_ok =
                password.peek().size() >= core::AuthService::minimum_password_length;
            const bool confirmation_ok = password.peek() == confirmation.peek();
            user_name_input->set_invalid(!name_ok);
            password_input->set_invalid(!password_ok);
            confirmation_input->set_invalid(!confirmation_ok);

            if (!name_ok || !password_ok || !confirmation_ok) {
                feedback_error.set(true);
                if (!name_ok) {
                    feedback.set("Enter a user name.");
                }
                else if (!password_ok) {
                    feedback.set(
                        "Use at least "
                        + std::to_string(core::AuthService::minimum_password_length)
                        + " characters for the password."
                    );
                }
                else {
                    feedback.set("The passwords do not match.");
                }
                return;
            }

            const auto created =
                store->user_service().register_user(user_name.peek(), password.peek());
            if (!created) {
                nandina::log::error("registration failed: {}", data::describe(created.error()));
                feedback_error.set(true);
                feedback.set(
                    created.error().code == data::DataErrorCode::constraint_violation
                        ? "That user name is already taken."
                        : "Could not create the account. Please try again."
                );
                return;
            }

            store->sign_in(*created);
            feedback_error.set(false);
            feedback.set("Account created for " + created->username + ".");
            if (!navigation.navigate<HomePage>()) {
                feedback_error.set(true);
                feedback.set("Account created, but the notebook could not be opened.");
            }
        };

        const auto submit_button =
            ui.make<nandina::widget::Button>("Create account")
                .width(nandina::widget::authoring::fill)
                .on_click(submit_registration)
                .build();
        user_name_input->set_on_submit(
            [submit_registration](std::string_view) { submit_registration(); }
        );
        password_input->set_on_submit(
            [submit_registration](std::string_view) { submit_registration(); }
        );
        confirmation_input->set_on_submit(
            [submit_registration](std::string_view) { submit_registration(); }
        );

        const auto sign_in_tip =
            ui.make<nandina::widget::Label>("Already have an account?")
                .height(20)
                .font_size(14)
                .color_token(nandina::theme::ColorToken::accent_foreground)
                .build();
        const auto sign_in_button =
            ui.make<nandina::widget::Button>("Sign in")
                .treatment(nandina::theme::ButtonTreatment::link)
                .height(20)
                .font_size(14)
                .on_click([navigation] { (void)navigation.navigate<UserLoginPage>(); })
                .configure([](nandina::widget::Button& button) {
                    button.set_label_align(nandina::theme::TextAlign::center);
                    button.set_text_overflow(nandina::text::TextOverflow::clip);
                })
                .build();

        auto form = ui.flex(nandina::widget::LayoutAxis::vertical)
                        .width(nandina::widget::authoring::fill)
                        .height(nandina::widget::authoring::fill)
                        .children(ui.make<nandina::widget::Label>("Create your account")
                                      .width(nandina::widget::authoring::fill)
                                      .height(30)
                                      .font_size(28)
                                      .color_token(nandina::theme::ColorToken::card_foreground)
                                      .configure([](nandina::widget::Label& label) {
                                          label.set_align(nandina::theme::TextAlign::center);
                                          label.set_font_weight(16);
                                      }))
                        .children(ui.expanded(2))
                        .children(ui.make<nandina::widget::Label>("Start your local notebook")
                                      .width(nandina::widget::authoring::fill)
                                      .height(10)
                                      .font_size(16)
                                      .color_token(nandina::theme::ColorToken::muted_foreground)
                                      .configure([](nandina::widget::Label& label) {
                                          label.set_align(nandina::theme::TextAlign::center);
                                      }))
                        .children(ui.expanded(8))
                        .children(auth::form_label(ui, "User name"))
                        .children(ui.expanded(2))
                        .children(user_name_input)
                        .children(ui.expanded(6))
                        .children(auth::form_label(ui, "Password"))
                        .children(ui.expanded(2))
                        .children(password_input)
                        .children(ui.expanded(6))
                        .children(auth::form_label(ui, "Confirm password"))
                        .children(ui.expanded(2))
                        .children(confirmation_input)
                        .children(ui.expanded(2))
                        .children(auth::feedback_line(ui, feedback, feedback_error))
                        .children(ui.expanded(3))
                        .children(submit_button)
                        .children(ui.expanded(8))
                        .children(ui.make<nandina::widget::Divider>())
                        .children(ui.expanded(4))
                        .children(ui.flex()
                                      .width(nandina::widget::authoring::fill)
                                      .height(36)
                                      .gap(5)
                                      .children(ui.expanded())
                                      .children(sign_in_tip)
                                      .children(sign_in_button)
                                      .children(ui.expanded()));

        return auth::auth_shell(ui, std::move(form));
    }
} // namespace nan_example::notebook::ui
