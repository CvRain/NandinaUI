#include "user_login_page.hpp"

#include "../home_page.hpp"
#include "auth_shell.hpp"
#include "user_register_page.hpp"

#include "app/notebook_store.hpp"
#include "data/data_error.hpp"
#include "nandina/foundation/nan_logger.hpp"

#include <nandina/widget/controls.hpp>

#include <string>
#include <utility>

namespace nan_example::notebook::ui
{
    auto UserLoginPage::build(nandina::app::PageContext& context) -> nandina::widget::View {
        auto ui = context.ui();
        auto* store = &context.store<app::NotebookStore>();
        const auto navigation = context.navigation();

        auto& user_name = ui.signal_value(std::string {});
        auto& user_password = ui.signal_value(std::string {});
        auto& feedback = ui.signal_value(std::string {});
        auto& feedback_error = ui.signal_value(false);

        const auto user_name_input =
            ui.make<nandina::widget::TextField>(user_name, "input your name")
                .width(nandina::widget::authoring::fill)
                .height(36)
                .configure([](nandina::widget::TextField& field) {
                    field.set_name("login-username");
                })
                .build();

        const auto user_password_input =
            ui.make<nandina::widget::TextField>(user_password, "password")
                .width(nandina::widget::authoring::fill)
                .configure([](nandina::widget::TextField& field) {
                    field.set_name("login-password");
                })
                .build();

        auto submit_login = [&user_name,
                             &user_password,
                             &feedback,
                             &feedback_error,
                             user_name_input,
                             user_password_input,
                             store,
                             navigation] {
            user_name_input->set_invalid(user_name.peek().empty());
            user_password_input->set_invalid(user_password.peek().empty());
            if (user_name.peek().empty() || user_password.peek().empty()) {
                feedback_error.set(true);
                feedback.set("Enter your user name and password.");
                return;
            }

            const auto result =
                store->user_service().user_login(user_name.peek(), user_password.peek());
            if (!result) {
                nandina::log::error("login failed: {}", data::describe(result.error()));
                feedback_error.set(true);
                feedback.set("Sign in failed. Please try again.");
                return;
            }
            if (!result->has_value()) {
                feedback_error.set(true);
                feedback.set("Incorrect user name or password.");
                return;
            }

            store->sign_in(**result);
            feedback_error.set(false);
            feedback.set("Signed in as " + (*result)->username + ".");
            if (!navigation.navigate<HomePage>()) {
                feedback_error.set(true);
                feedback.set("Could not open the notebook.");
            }
        };

        const auto user_login_button = ui.make<nandina::widget::Button>("Login")
                                           .width(nandina::widget::authoring::fill)
                                           .on_click(submit_login)
                                           .build();
        user_name_input->set_on_submit([submit_login](std::string_view) { submit_login(); });
        user_password_input->set_on_submit([submit_login](std::string_view) { submit_login(); });

        const auto register_tip = ui.make<nandina::widget::Label>("Don't have an account?")
                                      .height(20)
                                      .font_size(14)
                                      .color_token(nandina::theme::ColorToken::accent_foreground)
                                      .build();
        const auto register_jump_button =
            ui.make<nandina::widget::Button>("Sign up")
                .treatment(nandina::theme::ButtonTreatment::link)
                .label_align(nandina::theme::TextAlign::start)
                .height(20)
                .font_size(14)
                .on_click([navigation] { (void)navigation.navigate<UserRegisterPage>(); })
                .configure([](nandina::widget::Button& button) {
                    button.set_label_align(nandina::theme::TextAlign::center);
                    button.set_text_overflow(nandina::text::TextOverflow::clip);
                })
                .build();

        auto form = ui.flex(nandina::widget::LayoutAxis::vertical)
                        .width(nandina::widget::authoring::fill)
                        .height(nandina::widget::authoring::fill)
                        .children(ui.make<nandina::widget::Label>("Welcome back")
                                      .width(nandina::widget::authoring::fill)
                                      .height(30)
                                      .font_size(28)
                                      .color_token(nandina::theme::ColorToken::card_foreground)
                                      .configure([](nandina::widget::Label& label) {
                                          label.set_align(nandina::theme::TextAlign::center);
                                          label.set_font_weight(16);
                                      }))
                        .children(ui.expanded(2))
                        .children(ui.make<nandina::widget::Label>("Login to your notebook account")
                                      .width(nandina::widget::authoring::fill)
                                      .height(10)
                                      .font_size(16)
                                      .color_token(nandina::theme::ColorToken::muted_foreground)
                                      .configure([](nandina::widget::Label& label) {
                                          label.set_align(nandina::theme::TextAlign::center);
                                      }))
                        .children(ui.expanded(15))
                        .children(auth::form_label(ui, "User name"))
                        .children(ui.expanded(2))
                        .children(user_name_input)
                        .children(ui.expanded(10))
                        .children(auth::form_label(ui, "Password"))
                        .children(ui.expanded(2))
                        .children(user_password_input)
                        .children(ui.expanded(2))
                        .children(auth::feedback_line(ui, feedback, feedback_error))
                        .children(ui.expanded(3))
                        .children(user_login_button)
                        .children(ui.expanded(12))
                        .children(ui.make<nandina::widget::Divider>())
                        .children(ui.expanded(5))
                        .children(ui.flex()
                                      .width(nandina::widget::authoring::fill)
                                      .height(36)
                                      .gap(5)
                                      .children(ui.expanded())
                                      .children(register_tip)
                                      .children(register_jump_button)
                                      .children(ui.expanded()));

        return auth::auth_shell(ui, std::move(form));
    }
} // namespace nan_example::notebook::ui

/**
 * 开发时遇到问题：
 * 计划设计一个卡片式登录界面，按计划设计是一个圆角矩形的组件被均分为两份，左侧是登录表单，右侧是图片。
 * 遇到了一些不太顺利的问题：
 * 1. 计划使用Card将两个子组件包裹，但是因为Card内部存在margin，导致无法将图片完全拉满。此问题的本质不是Card
 * 而是因为缺少一个类似Rectangle的组件，可以设置任意圆角。直接为widget::Image提供圆角功能并不好。
 * 2. Label似乎缺少垂直布局的问题，默认情况下Label和Button中的文字是自动垂直居中的，但实际上可能有不同的需求
 * 3. widget::Image在设置cover的方式后，有必要增加一个从什么什么位置可以截取，作为百分比。
 * 比如增加start_with(20,0)图片被裁剪后从宽度20%的位置开始，高度是0%
 * 4. 需要增加一些基本基本颜色，比如read, blue, steel blue等方便使用，同时也需要将主题颜色对应做一套色卡
 * 现在为某个组件文字或者一些内容配色，使用accent, accent_foreground, muted_foreground不是很明显，
 * 当然这些也很有用，不过一些自定义组件或者个性配置的时候就不够用了。
 */