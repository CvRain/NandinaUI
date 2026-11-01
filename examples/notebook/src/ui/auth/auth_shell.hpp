#ifndef NANDINA_EXAMPLES_NOTEBOOK_UI_AUTH_AUTH_SHELL_HPP
#define NANDINA_EXAMPLES_NOTEBOOK_UI_AUTH_AUTH_SHELL_HPP

#include <nandina/widget/controls.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace nan_example::notebook::ui::auth
{
    [[nodiscard]] inline auto
    form_label(const nandina::widget::BuildContext& ui, const std::string_view text)
        -> std::shared_ptr<nandina::widget::Label> {
        return ui.make<nandina::widget::Label>(std::string(text))
            .width(nandina::widget::authoring::fill)
            .height(20)
            .font_size(16)
            .color_token(nandina::theme::ColorToken::accent_foreground)
            .build();
    }

    [[nodiscard]] inline auto feedback_line(
        const nandina::widget::BuildContext& ui,
        nandina::reactive::Signal<std::string>& message,
        nandina::reactive::Signal<bool>& error
    ) -> std::shared_ptr<nandina::widget::Label> {
        auto label = ui.make<nandina::widget::Label>("")
                         .width(nandina::widget::authoring::fill)
                         .height(18)
                         .font_size(13)
                         .color_token(nandina::theme::ColorToken::muted_foreground)
                         .configure([](nandina::widget::Label& target) {
                             target.set_overflow(nandina::text::TextOverflow::clip);
                             target.set_max_lines(1);
                         })
                         .build();
        ui.bind(label, &nandina::widget::Label::set_text, message);
        ui.bind(
            label,
            [](nandina::widget::Label& target, const bool is_error) {
                target.set_color_token(
                    is_error ? nandina::theme::ColorToken::error
                             : nandina::theme::ColorToken::muted_foreground
                );
            },
            error
        );
        return label;
    }

    /// Shared split-card layout for the login and registration pages: a centered form
    /// on the left, the resource-backed cover on the right.
    template<typename Form>
    [[nodiscard]] auto auth_shell(
        const nandina::widget::BuildContext& ui,
        nandina::widget::authoring::NodeBuilder<Form> form
    ) -> nandina::widget::View {
        auto form_area = ui.center()
                             .width(nandina::scene::percent(90))
                             .height(nandina::scene::percent(90))
                             .child(std::move(form));
        auto form_column = ui.column()
                               .gap(5)
                               .width(nandina::scene::percent(50))
                               .height(nandina::scene::percent(100))
                               .children(std::move(form_area));

        auto cover = ui.make<nandina::widget::Image>("res://images/nandina_cover_1.jpg")
                         .width(nandina::scene::percent(50))
                         .height(nandina::scene::percent(100))
                         .configure([](nandina::widget::Image& image) {
                             image.set_scale_mode(nandina::widget::ImageScale::cover);
                             image.set_alignment(nandina::widget::ImageAlignment::end);
                         });

        auto columns = ui.row().children(std::move(form_column)).children(std::move(cover));
        auto card =
            ui.make<nandina::widget::Card>().width(894).height(528.25).child(std::move(columns));
        return ui.center().child(std::move(card)).build();
    }
} // namespace nan_example::notebook::ui::auth

#endif // NANDINA_EXAMPLES_NOTEBOOK_UI_AUTH_AUTH_SHELL_HPP
