//
// Created by cvrain on 2026/5/30.
//
module;

#include <format>
#include <functional>
#include <string>
#include <string_view>

export module nandina.showcase.page.forms;

import nandina.app.application;
import nandina.app.authoring;
import nandina.app.page;
import nandina.foundation.color;
import nandina.layout.container;
import nandina.text;
import nandina.theme;
import nandina.widgets;

export namespace nandina::showcase {
    class FormsPage final : public nandina::app::NanPage {
    public:
        [[nodiscard]] auto route_key() const noexcept -> ::std::string_view override {
            return "showcase::page::forms";
        }

        [[nodiscard]] auto title() const noexcept -> ::std::string_view override {
            return "Forms";
        }

        [[nodiscard]] auto icon_type() const noexcept -> nandina::widgets::IconType override {
            return nandina::widgets::IconType::Dot;
        }

        [[nodiscard]] auto build() -> app::NanComponent::Ptr override {
            using namespace nandina::app;
            using nandina::layout::LayoutAlignment;
            using nandina::theme::ButtonVariant;
            using nandina::theme::ButtonSize;

            auto page_title = label()
                .text("Forms")
                .align(widgets::TextAlign::Start)
                .font(text::NanFont{}
                    .size(24)
                    .weight(text::NanFontWeight::bold))
                .color(NanColor::from(NanRgb{"#caddf5"}));

            auto page_subtitle = label()
                .text("Text input controls, field containers, and form state composition")
                .align(widgets::TextAlign::Start)
                .font(text::NanFont{}
                    .size(16)
                    .weight(text::NanFontWeight::medium))
                .color(NanColor::from(NanRgb{"#caddf5"}));

            // ── Section 1: Basic TextField ─────────────────────
            auto section1_title = label()
                .text("TextField")
                .align(widgets::TextAlign::Start)
                .font(text::NanFont{}
                    .size(18)
                    .weight(text::NanFontWeight::semiBold))
                .color(NanColor::from(NanRgb{"#cdd6f4"}));

            auto basic_tf = text_field()
                .placeholder("Type something...")
                .width(280.0f);

            auto placeholder_tf = text_field()
                .placeholder("Search...")
                .value("prefilled text")
                .width(280.0f);

            auto disabled_tf = text_field()
                .placeholder("Disabled input")
                .value("can't edit")
                .disabled(true)
                .width(280.0f);

            auto readonly_tf = text_field()
                .placeholder("Read only")
                .value("read only text")
                .read_only(true)
                .width(280.0f);

            // ── Section 2: Field with label/helper/error ──────
            auto section2_title = label()
                .text("Field")
                .align(widgets::TextAlign::Start)
                .font(text::NanFont{}
                    .size(18)
                    .weight(text::NanFontWeight::semiBold))
                .color(NanColor::from(NanRgb{"#cdd6f4"}));

            auto email_field = field()
                .label("Email")
                .helper_text("We'll never share your email.")
                .control(text_field().placeholder("you@example.com"))
                .width(280.0f);

            auto required_field = field()
                .label("Username")
                .helper_text("Must be at least 3 characters.")
                .error_text("Username is required.")
                .control(text_field().placeholder("Enter username"))
                .required(true)
                .width(280.0f);

            // ── Section 3: Interactive invalid toggle ──────────
            auto section3_title = label()
                .text("Interactive States")
                .align(widgets::TextAlign::Start)
                .font(text::NanFont{}
                    .size(18)
                    .weight(text::NanFontWeight::semiBold))
                .color(NanColor::from(NanRgb{"#cdd6f4"}));

            auto interactive_field = field()
                .label("Password")
                .helper_text("Must be 8+ characters.")
                .error_text("Password is too short.")
                .control(text_field().placeholder("Enter password"))
                .bind(m_password_field_ref);

            auto toggle_invalid_btn = button()
                .size(ButtonSize::sm)
                .variant(ButtonVariant::outline);

            toggle_invalid_btn.on_click([this]() {
                m_show_password_error = !m_show_password_error;
                if (auto* f = m_password_field_ref.get()) {
                    f->set_invalid(m_show_password_error);
                }
            });

            // 根据当前状态更新按钮文字（点击时已通过 toggle 更新）
            // 使用 text effect 自动追踪 m_show_password_error
            auto toggle_btn_label = label()
                .text([this] {
                    return m_show_password_error ? "✓ Mark Valid" : "✗ Mark Invalid";
                })
                .font(text::NanFont{}
                    .size(13)
                    .color(NanColor::from(NanRgb{"#7c7f93"})))
                .align(widgets::TextAlign::Start);

            return mount(column(children(
                page_title,
                page_subtitle,
                spacer(1).height(24.0f),
                section1_title,
                spacer(1).height(8.0f),
                basic_tf,
                spacer(1).height(8.0f),
                placeholder_tf,
                spacer(1).height(8.0f),
                disabled_tf,
                spacer(1).height(8.0f),
                readonly_tf,
                spacer(1).height(24.0f),
                section2_title,
                spacer(1).height(8.0f),
                email_field,
                spacer(1).height(8.0f),
                required_field,
                spacer(1).height(24.0f),
                section3_title,
                spacer(1).height(8.0f),
                interactive_field.width(280.0f),
                spacer(1).height(8.0f),
                toggle_invalid_btn,
                toggle_btn_label
            ))
            .gap(0)
            .align_items(LayoutAlignment::start)
            .padding(24.0f));
        }

    private:
        nandina::app::Ref<nandina::widgets::Field> m_password_field_ref;
        bool m_show_password_error{false};
    };
}
