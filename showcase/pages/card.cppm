//
// CardPage — Card 组件展示页
// 参考 shadcn card demo: basic / with description / form / action / footer
//

module;

#include <functional>
#include <string>
#include <string_view>

export module nandina.showcase.page.card;

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

    class CardPage final : public nandina::app::NanPage {
    public:
        [[nodiscard]] auto route_key() const noexcept -> std::string_view override;
        [[nodiscard]] auto title() const noexcept -> std::string_view override;
        [[nodiscard]] auto icon_type() const noexcept -> nandina::widgets::IconType override;
        [[nodiscard]] auto build() -> app::NanComponent::Ptr override;
    };

} // namespace nandina::showcase


// ── 实现 ─────────────────────────────────────────────────────────────────────

namespace nandina::showcase {

    auto CardPage::route_key() const noexcept -> std::string_view {
        return "showcase::card";
    }

    auto CardPage::title() const noexcept -> std::string_view {
        return "Card";
    }

    auto CardPage::icon_type() const noexcept -> nandina::widgets::IconType {
        return nandina::widgets::IconType::Dot;
    }

    auto CardPage::build() -> app::NanComponent::Ptr {
        using namespace nandina::app;
        using nandina::theme::ButtonVariant;
        using nandina::theme::ButtonSize;
        using nandina::layout::LayoutAlignment;

        auto section_title = [](std::string_view t) {
            return app::label(t)
                .font(nandina::text::NanFont{}
                    .size(15)
                    .weight(nandina::text::NanFontWeight::semiBold)
                    .color(NanColor::from(NanRgb{"#cdd6f4"})));
        };

        auto body_text = [](std::string_view t) {
            return app::label(t)
                .font(nandina::text::NanFont{}
                    .size(13)
                    .single_line(false)
                    .overflow(nandina::text::TextOverflow::wrap)
                    .color(NanColor::from(NanRgb{"#a6adc8"})));
        };

        auto muted_text = [](std::string_view t) {
            return app::label(t)
                .font(nandina::text::NanFont{}
                    .size(12)
                    .color(NanColor::from(NanRgb{"#6c7086"})));
        };

        // ═══════════════════════════════════════════════════════════
        // 1. Basic — 最简单的 Card
        // ═══════════════════════════════════════════════════════════
        auto basic_card = card(children(
            column(children(
                section_title("Basic Card"),
                body_text("A simple card with a title and content."),
                body_text("Cards are used to group related information and actions. "
                          "They provide a clean, elevated surface for content display.")
            )).gap(8)
        ));

        // ═══════════════════════════════════════════════════════════
        // 2. With Elevation — 带阴影层次
        // ═══════════════════════════════════════════════════════════
        auto elevated_card = []() -> Node {
            auto c = nandina::widgets::Card::create();
            c->set_elevation(6.0f)
             .set_title("Elevated Card")
             .set_accent_color(NanColor::from(NanRgb{"#cba6f7"}))
             .set_show_accent(true);
            return adopt(std::move(c));
        }();

        // ═══════════════════════════════════════════════════════════
        // 3. Account / Profile Card
        // ═══════════════════════════════════════════════════════════
        auto account_card = card(children(
            column(children(
                section_title("Account"),
                body_text("Manage your account settings and preferences."),

                // 模拟 account rows
                column(children(
                    row(children(
                        label("Name").font(nandina::text::NanFont{}
                            .size(13).weight(nandina::text::NanFontWeight::medium)
                            .color(NanColor::from(NanRgb{"#cdd6f4"}))),
                        spacer(),
                        body_text("CvRain")
                    )).align_items(LayoutAlignment::center),
                    row(children(
                        label("Email").font(nandina::text::NanFont{}
                            .size(13).weight(nandina::text::NanFontWeight::medium)
                            .color(NanColor::from(NanRgb{"#cdd6f4"}))),
                        spacer(),
                        body_text("cvrain@example.com")
                    )).align_items(LayoutAlignment::center),
                    row(children(
                        label("Plan").font(nandina::text::NanFont{}
                            .size(13).weight(nandina::text::NanFontWeight::medium)
                            .color(NanColor::from(NanRgb{"#cdd6f4"}))),
                        spacer(),
                        tag("Pro").color_variant(nandina::theme::ColorVariant::secondary)
                    )).align_items(LayoutAlignment::center)
                )).gap(6)
            )).gap(8)
        ));

        // ═══════════════════════════════════════════════════════════
        // 4. Notification Card — 带 switch/checkbox 的卡片
        // ═══════════════════════════════════════════════════════════
        auto notification_card = card(children(
            column(children(
                section_title("Notifications"),
                body_text("Choose what notifications you want to receive."),

                column(children(
                    row(children(
                        column(children(
                            label("Push Notifications")
                                .font(nandina::text::NanFont{}
                                    .size(13).weight(nandina::text::NanFontWeight::medium)
                                    .color(NanColor::from(NanRgb{"#cdd6f4"}))),
                            muted_text("Receive push notifications on your device.")
                        )).gap(2),
                        spacer(),
                        checkbox("").checked(true)
                    )).align_items(LayoutAlignment::start),

                    row(children(
                        column(children(
                            label("Email Digest")
                                .font(nandina::text::NanFont{}
                                    .size(13).weight(nandina::text::NanFontWeight::medium)
                                    .color(NanColor::from(NanRgb{"#cdd6f4"}))),
                            muted_text("Weekly summary of activity.")
                        )).gap(2),
                        spacer(),
                        checkbox("")
                    )).align_items(LayoutAlignment::start),

                    row(children(
                        column(children(
                            label("Marketing Emails")
                                .font(nandina::text::NanFont{}
                                    .size(13).weight(nandina::text::NanFontWeight::medium)
                                    .color(NanColor::from(NanRgb{"#cdd6f4"}))),
                            muted_text("Promotions, tips, and product updates.")
                        )).gap(2),
                        spacer(),
                        checkbox("").disabled(true)
                    )).align_items(LayoutAlignment::start)
                )).gap(8)
            )).gap(8)
        ));

        // ═══════════════════════════════════════════════════════════
        // 5. Payment / Action Card — 带 footer 操作按钮
        // ═══════════════════════════════════════════════════════════
        auto payment_card = []() -> Node {
            auto c = nandina::widgets::Card::create();
            c->set_title("Upgrade to Pro");
            c->set_accent_color(NanColor::from(NanRgb{"#cba6f7"}));
            c->set_show_accent(true);

            // Footer with buttons
            auto footer = nandina::layout::Row::Create();
            footer->gap(8.0f);
            auto cancel = nandina::widgets::Button::create();
            cancel->text("Cancel");
            cancel->variant(nandina::widgets::ButtonVariant::ghost);
            cancel->size(nandina::widgets::ButtonSize::sm);
            auto spacer = nandina::layout::Spacer::Create(1);
            auto upgrade = nandina::widgets::Button::create();
            upgrade->text("Upgrade to Pro");
            upgrade->variant(nandina::widgets::ButtonVariant::default_variant);
            upgrade->size(nandina::widgets::ButtonSize::sm);
            footer->add(std::move(cancel));
            footer->add(std::move(spacer));
            footer->add(std::move(upgrade));
            c->set_footer(std::move(footer));

            return adopt(std::move(c));
        }();

        // ═══════════════════════════════════════════════════════════
        // 6. Simple Login Form Card
        // ═══════════════════════════════════════════════════════════
        auto login_card = card(children(
            column(children(
                section_title("Login"),
                body_text("Enter your credentials to access your account."),

                column(children(
                    label("Email").font(nandina::text::NanFont{}
                        .size(12).weight(nandina::text::NanFontWeight::medium)
                        .color(NanColor::from(NanRgb{"#a6adc8"}))),
                    text_field().placeholder("you@example.com"),

                    label("Password").font(nandina::text::NanFont{}
                        .size(12).weight(nandina::text::NanFontWeight::medium)
                        .color(NanColor::from(NanRgb{"#a6adc8"}))),
                    text_field().placeholder("Enter your password"),

                    row(children(
                        checkbox("Remember me"),
                        spacer(),
                        label("Forgot password?").font(nandina::text::NanFont{}
                            .size(12).color(NanColor::from(NanRgb{"#89b4fa"})))
                    )).align_items(LayoutAlignment::center),

                    button("Sign In")
                        .variant(ButtonVariant::default_variant)
                        .size(ButtonSize::md)
                )).gap(8)
            )).gap(8)
        ));

        // ── 最终页面布局 ──────────────────────────────────────────
        return mount(
            column(children(
                label("Card")
                    .font(nandina::text::NanFont{}
                        .size(24).weight(nandina::text::NanFontWeight::bold)
                        .color(NanColor::from(NanRgb{"#cdd6f4"}))),
                label("Displays a card with header, content, and footer.")
                    .font(nandina::text::NanFont{}
                        .size(13)
                        .color(NanColor::from(NanRgb{"#a6adc8"}))),

                // Row 1: Basic + Elevated
                row(children(
                    expanded(sized_box(basic_card)),
                    sized_box(spacer(0)).width(16),
                    expanded(sized_box(elevated_card))
                )).align_items(LayoutAlignment::stretch),

                sized_box(spacer(0)).height(16),

                // Row 2: Account + Notification
                row(children(
                    expanded(sized_box(account_card)),
                    sized_box(spacer(0)).width(16),
                    expanded(sized_box(notification_card))
                )).align_items(LayoutAlignment::stretch), 

                sized_box(spacer(0)).height(16),

                // Row 3: Payment + Login
                row(children(
                    expanded(sized_box(payment_card)),
                    sized_box(spacer(0)).width(16),
                    expanded(sized_box(login_card))
                )).align_items(LayoutAlignment::stretch)
            ))
            .align_items(LayoutAlignment::stretch)
            .gap(16)
            .padding(24)
        );
    }

} // namespace nandina::showcase
