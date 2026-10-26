#include "account_pages.hpp"

#include "lab_style.hpp"
#include "notes_page.hpp"

#include "../lab_store.hpp"

#include <nandina/reactive/signal.hpp>
#include <nandina/widget/controls.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace nandina::lab
{
    namespace
    {
        [[nodiscard]] auto valid_email(const std::string_view value) -> bool {
            const auto position = value.find('@');
            return position != std::string_view::npos && position > 0
                && value.find('.', position) != std::string_view::npos;
        }

        template<typename Field>
        [[nodiscard]] auto
        labeled_field(const widget::BuildContext& ui, const std::string_view label, Field&& field)
            -> widget::authoring::NodeBuilder<widget::Column> {
            return ui.column()
                .width(widget::authoring::fill)
                .gap(6.0F)
                .children(caption(ui, label))
                .children(std::forward<Field>(field));
        }

        /// Feedback line shared by both account forms: success/info stays muted,
        /// validation failures switch to the error token through a reactive bind.
        [[nodiscard]] auto feedback_line(
            const widget::BuildContext& ui,
            reactive::Signal<std::string>& text,
            reactive::Signal<bool>& error
        ) -> std::shared_ptr<widget::Label> {
            auto label = ui.make<widget::Label>(std::string {})
                             .color_token(theme::ColorToken::muted_foreground)
                             .font_size(12.0F)
                             .width(widget::authoring::fill)
                             .configure([](widget::Label& target) {
                                 target.set_overflow(text::TextOverflow::wrap);
                                 target.set_max_lines(3);
                             })
                             .build();
            ui.bind(label, &widget::Label::set_text, text);
            ui.bind(
                label,
                [](widget::Label& target, const bool is_error) {
                    target.set_color_token(
                        is_error ? theme::ColorToken::error : theme::ColorToken::muted_foreground
                    );
                },
                error
            );
            return label;
        }

        [[nodiscard]] auto account_page(
            app::PageContext& context,
            const std::string_view title,
            const std::string_view description,
            widget::authoring::NodeBuilder<widget::Column> form,
            widget::authoring::NodeBuilder<widget::Row> actions
        ) -> widget::View {
            auto ui = context.ui();
            auto content = ui.column()
                               .width(widget::authoring::fill)
                               .max_width(FORM_MAX)
                               .gap(16.0F)
                               .cross_alignment(widget::LayoutAlignment::stretch)
                               .children(page_title(ui, title))
                               .children(body_text(ui, description, 4))
                               .children(card(ui, form))
                               .children(actions);

            return ui.scroll_view()
                .width(widget::authoring::fill)
                .height(widget::authoring::fill)
                .child(ui.padding(foundation::NanInsets::all(PAGE_PADDING))
                           .child(ui.center().width(widget::authoring::fill).child(content)))
                .build();
        }
    } // namespace

    auto LoginPage::build(app::PageContext& context) -> widget::View {
        auto ui = context.ui();
        auto* store = &context.store<LabStore>();
        const auto navigation = context.navigation();

        auto& email = ui.signal_value(std::string {});
        auto& password = ui.signal_value(std::string {});
        auto& feedback = ui.signal_value(std::string {});
        auto& feedback_error = ui.signal_value(false);
        auto& valid = ui.computed([&email, &password] {
            return valid_email(email.get()) && password.get().size() >= 6;
        });

        const auto email_field = ui.make<widget::TextField>(email, "you@example.com")
                                     .width(widget::authoring::fill)
                                     .build();
        const auto password_field = ui.make<widget::TextField>(password, "at least 6 characters")
                                        .width(widget::authoring::fill)
                                        .build();

        const auto submit = ui.make<widget::Button>("Sign in")
                                .treatment(theme::ButtonTreatment::filled)
                                .on_click([&email,
                                           &password,
                                           &feedback,
                                           &feedback_error,
                                           email_field,
                                           password_field,
                                           store,
                                           navigation] {
                                    const bool email_ok = valid_email(email.peek());
                                    const bool password_ok = password.peek().size() >= 6;
                                    email_field->set_invalid(!email_ok);
                                    password_field->set_invalid(!password_ok);
                                    if (!email_ok || !password_ok) {
                                        feedback_error.set(true);
                                        feedback.set(
                                            "Use a valid email and a password with at least 6 "
                                            "characters."
                                        );
                                        return;
                                    }
                                    feedback_error.set(false);
                                    store->signed_in.set(true);
                                    store->display_name.set(email.peek());
                                    store->notice.set("Signed in to the local example session.");
                                    if (!navigation.navigate<NotesPage>()) {
                                        feedback_error.set(true);
                                        feedback.set("Could not open the notes route.");
                                    }
                                })
                                .build();
        ui.bind(
            submit,
            [](widget::Button& button, const bool enabled) { button.set_disabled(!enabled); },
            valid
        );

        auto form = ui.column()
                        .width(widget::authoring::fill)
                        .gap(12.0F)
                        .children(labeled_field(ui, "Email", email_field))
                        .children(labeled_field(ui, "Password", password_field))
                        .children(feedback_line(ui, feedback, feedback_error));

        auto actions = ui.row()
                           .gap(8.0F)
                           .cross_alignment(widget::LayoutAlignment::center)
                           .children(submit)
                           .children(ui.make<widget::Button>("Use demo")
                                         .treatment(theme::ButtonTreatment::ghost)
                                         .on_click([&email,
                                                    &password,
                                                    &feedback,
                                                    &feedback_error,
                                                    email_field,
                                                    password_field] {
                                             email.set("demo@example.com");
                                             password.set("secret1");
                                             email_field->set_invalid(false);
                                             password_field->set_invalid(false);
                                             feedback_error.set(false);
                                             feedback.set("Demo credentials filled.");
                                         }))
                           .children(ui.make<widget::Button>("Create account")
                                         .treatment(theme::ButtonTreatment::ghost)
                                         .on_click([navigation] {
                                             (void)navigation.navigate<RegisterPage>();
                                         }));

        return account_page(
            context,
            "Sign in",
            "Account forms are local demo state. Nothing is sent over the network and no "
            "credential is persisted.",
            std::move(form),
            std::move(actions)
        );
    }

    auto RegisterPage::build(app::PageContext& context) -> widget::View {
        auto ui = context.ui();
        auto* store = &context.store<LabStore>();
        const auto navigation = context.navigation();

        auto& name = ui.signal_value(std::string {});
        auto& email = ui.signal_value(std::string {});
        auto& password = ui.signal_value(std::string {});
        auto& confirm = ui.signal_value(std::string {});
        auto& feedback = ui.signal_value(std::string {});
        auto& feedback_error = ui.signal_value(false);
        auto& valid = ui.computed([&name, &email, &password, &confirm] {
            return !name.get().empty() && valid_email(email.get()) && password.get().size() >= 6
                && password.get() == confirm.get();
        });

        const auto name_field =
            ui.make<widget::TextField>(name, "Display name").width(widget::authoring::fill).build();
        const auto email_field = ui.make<widget::TextField>(email, "you@example.com")
                                     .width(widget::authoring::fill)
                                     .build();
        const auto password_field = ui.make<widget::TextField>(password, "at least 6 characters")
                                        .width(widget::authoring::fill)
                                        .build();
        const auto confirm_field = ui.make<widget::TextField>(confirm, "repeat the password")
                                       .width(widget::authoring::fill)
                                       .build();

        const auto submit = ui.make<widget::Button>("Create account")
                                .treatment(theme::ButtonTreatment::filled)
                                .on_click([&name,
                                           &email,
                                           &password,
                                           &confirm,
                                           &feedback,
                                           &feedback_error,
                                           name_field,
                                           email_field,
                                           password_field,
                                           confirm_field,
                                           store,
                                           navigation] {
                                    const bool name_ok = !name.peek().empty();
                                    const bool email_ok = valid_email(email.peek());
                                    const bool password_ok = password.peek().size() >= 6;
                                    const bool confirm_ok = password.peek() == confirm.peek();
                                    name_field->set_invalid(!name_ok);
                                    email_field->set_invalid(!email_ok);
                                    password_field->set_invalid(!password_ok);
                                    confirm_field->set_invalid(!confirm_ok);
                                    if (!name_ok || !email_ok || !password_ok || !confirm_ok) {
                                        feedback_error.set(true);
                                        feedback.set(
                                            "Check the display name, email, password length and "
                                            "confirmation."
                                        );
                                        return;
                                    }
                                    feedback_error.set(false);
                                    store->signed_in.set(true);
                                    store->display_name.set(name.peek());
                                    store->notice.set("Local account created in memory only.");
                                    if (!navigation.navigate<NotesPage>()) {
                                        feedback_error.set(true);
                                        feedback.set("Could not open the notes route.");
                                    }
                                })
                                .build();
        ui.bind(
            submit,
            [](widget::Button& button, const bool enabled) { button.set_disabled(!enabled); },
            valid
        );

        auto form = ui.column()
                        .width(widget::authoring::fill)
                        .gap(12.0F)
                        .children(labeled_field(ui, "Display name", name_field))
                        .children(labeled_field(ui, "Email", email_field))
                        .children(labeled_field(ui, "Password", password_field))
                        .children(labeled_field(ui, "Confirm password", confirm_field))
                        .children(feedback_line(ui, feedback, feedback_error));

        auto actions =
            ui.row()
                .gap(8.0F)
                .cross_alignment(widget::LayoutAlignment::center)
                .children(submit)
                .children(ui.make<widget::Button>("Use demo")
                              .treatment(theme::ButtonTreatment::ghost)
                              .on_click([&name,
                                         &email,
                                         &password,
                                         &confirm,
                                         &feedback,
                                         &feedback_error,
                                         name_field,
                                         email_field,
                                         password_field,
                                         confirm_field] {
                                  name.set("Ada Lovelace");
                                  email.set("ada@example.com");
                                  password.set("secret1");
                                  confirm.set("secret1");
                                  name_field->set_invalid(false);
                                  email_field->set_invalid(false);
                                  password_field->set_invalid(false);
                                  confirm_field->set_invalid(false);
                                  feedback_error.set(false);
                                  feedback.set("Demo values filled.");
                              }))
                .children(ui.make<widget::Button>("Back to sign in")
                              .treatment(theme::ButtonTreatment::ghost)
                              .on_click([navigation] { (void)navigation.navigate<LoginPage>(); }));

        return account_page(
            context,
            "Create a local account",
            "The form validates locally and stores the display name in the application store. "
            "The password fields are visible: this alpha has no password masking API.",
            std::move(form),
            std::move(actions)
        );
    }
} // namespace nandina::lab
