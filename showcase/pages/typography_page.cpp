//
// showcase/pages/typography_page.cpp — P4.6 Step 2 typography scale, leading and roles.
//

#include "typography_page.hpp"

#include "../components/page_chrome.hpp"

#include <nandina/theme/typography_scale.hpp>
#include <nandina/widget/authoring.hpp>
#include <nandina/widget/layout.hpp>

#include <array>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace nandina::showcase
{
    namespace
    {
        inline constexpr float kRoleColumnWidth = 88.0F;
        inline constexpr float kMetaColumnWidth = 210.0F;
        inline constexpr float kSampleColumnWidth = 520.0F;

        [[nodiscard]] auto sample_row(
            const widget::BuildContext& ui,
            const PageChrome& chrome,
            const std::string_view name,
            const std::string_view meta,
            const std::string_view sample,
            const float sample_size
        ) -> widget::authoring::NodeBuilder<widget::Row> {
            return ui.row()
                .width(widget::authoring::fill)
                .gap(chrome.group)
                .cross_alignment(widget::LayoutAlignment::center)
                .children(
                    ui.make<widget::Label>(std::string(name))
                        .color_token(theme::ColorToken::muted_foreground)
                        .font_size(chrome.caption)
                        .width(kRoleColumnWidth),
                    ui.make<widget::Label>(std::string(meta))
                        .color_token(theme::ColorToken::muted_foreground)
                        .font_size(chrome.caption)
                        .width(kMetaColumnWidth),
                    ui.expanded().child(ui.make<widget::Label>(std::string(sample))
                                            .color_token(theme::ColorToken::foreground)
                                            .font_size(sample_size)
                                            .width(widget::authoring::fill))
                );
        }

        [[nodiscard]] auto intro_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);
            return ui.column()
                .width(widget::authoring::fill)
                .gap(chrome.block)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(page_heading(ui, chrome, "Typography"))
                .children(supporting_text(
                    ui,
                    chrome,
                    "P4.6 Step 2: the Tailwind-compatible TextSize scale, the Leading multipliers, "
                    "and the semantic roles that map them onto purposes. Body text now defaults to "
                    "1.5 leading through the body role; every role is theme-overridable.",
                    8
                ));
        }

        [[nodiscard]] auto scale_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);
            const auto& scale = context.theme().tokens.typography.scale;
            constexpr std::array<theme::TextSize, theme::text_size_count> sizes {
                theme::TextSize::xs,
                theme::TextSize::sm,
                theme::TextSize::base,
                theme::TextSize::lg,
                theme::TextSize::xl,
                theme::TextSize::xl_2,
                theme::TextSize::xl_3,
                theme::TextSize::xl_4,
                theme::TextSize::xl_5,
                theme::TextSize::xl_6,
                theme::TextSize::xl_7,
                theme::TextSize::xl_8,
                theme::TextSize::xl_9,
            };

            auto rows = ui.column()
                            .width(widget::authoring::fill)
                            .gap(chrome.group)
                            .cross_alignment(widget::LayoutAlignment::stretch);
            for (const auto size: sizes) {
                const auto px = theme::text_size_px(scale, size);
                const std::string meta = "TextSize::" + std::string(theme::text_size_name(size));
                rows.children(
                    ui.row()
                        .width(widget::authoring::fill)
                        .gap(chrome.group)
                        .cross_alignment(widget::LayoutAlignment::center)
                        .children(
                            ui.make<widget::Label>(std::string(theme::text_size_name(size)))
                                .color_token(theme::ColorToken::muted_foreground)
                                .font_size(chrome.caption)
                                .width(kRoleColumnWidth),
                            ui.make<widget::Label>(meta + " · " + std::to_string(px) + " px")
                                .color_token(theme::ColorToken::muted_foreground)
                                .font_size(chrome.caption)
                                .width(kMetaColumnWidth),
                            ui.expanded().child(ui.make<widget::Label>("Ag 汉字")
                                                    .color_token(theme::ColorToken::foreground)
                                                    .text_size(size))
                        )
                );
            }

            return section_block(
                ui,
                chrome,
                "Type scale",
                "Thirteen Tailwind-compatible steps; px values follow root base × ratio and stay "
                "logical (the OS content scale is applied by the renderer).",
                rows
            );
        }

        [[nodiscard]] auto leading_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);
            const auto& scale = context.theme().tokens.typography.scale;
            constexpr std::array<theme::Leading, theme::leading_count> leadings {
                theme::Leading::none,
                theme::Leading::tight,
                theme::Leading::snug,
                theme::Leading::normal,
                theme::Leading::relaxed,
                theme::Leading::loose,
            };
            constexpr std::string_view paragraph =
                "行高决定多行正文的呼吸感。The quick brown fox jumps over the lazy dog, and the "
                "second sentence keeps the sample long enough to wrap.";

            auto rows = ui.column()
                            .width(widget::authoring::fill)
                            .gap(chrome.group)
                            .cross_alignment(widget::LayoutAlignment::stretch);
            for (const auto leading: leadings) {
                const auto factor = theme::leading_factor(scale, leading);
                rows.children(
                    ui.row()
                        .width(widget::authoring::fill)
                        .gap(chrome.group)
                        .cross_alignment(widget::LayoutAlignment::start)
                        .children(
                            ui.make<widget::Label>(std::string(theme::leading_name(leading)))
                                .color_token(theme::ColorToken::muted_foreground)
                                .font_size(chrome.caption)
                                .width(kRoleColumnWidth),
                            ui.make<widget::Label>("Leading · " + std::to_string(factor) + "×")
                                .color_token(theme::ColorToken::muted_foreground)
                                .font_size(chrome.caption)
                                .width(kMetaColumnWidth),
                            ui.expanded().child(ui.make<widget::Label>(std::string(paragraph))
                                                    .color_token(theme::ColorToken::foreground)
                                                    .text_size(theme::TextSize::base)
                                                    .leading(leading)
                                                    .width(kSampleColumnWidth)
                                                    .configure([](widget::Label& label) {
                                                        label.set_overflow(
                                                            text::TextOverflow::wrap
                                                        );
                                                        label.set_max_lines(4);
                                                    }))
                        )
                );
            }

            return section_block(
                ui,
                chrome,
                "Leading",
                "Leading is a multiplier of the font size. The same paragraph is repeated so only "
                "the line height changes; body text defaults to normal (1.5).",
                rows
            );
        }

        [[nodiscard]] auto role_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);
            const auto& scale = context.theme().tokens.typography.scale;
            constexpr std::array<theme::TypographyRole, theme::typography_role_count> roles {
                theme::TypographyRole::display,
                theme::TypographyRole::heading,
                theme::TypographyRole::title,
                theme::TypographyRole::subtitle,
                theme::TypographyRole::body,
                theme::TypographyRole::label,
                theme::TypographyRole::caption,
            };

            auto rows = ui.column()
                            .width(widget::authoring::fill)
                            .gap(chrome.group)
                            .cross_alignment(widget::LayoutAlignment::stretch);
            for (const auto role: roles) {
                const auto style = theme::typography_role_style(scale, role);
                const std::string meta = std::string(theme::text_size_name(style.size)) + " · "
                    + std::to_string(static_cast<int>(theme::text_size_px(scale, style.size)))
                    + " px · " + std::string(theme::leading_name(style.leading)) + " · "
                    + std::to_string(style.font_weight);
                rows.children(
                    ui.row()
                        .width(widget::authoring::fill)
                        .gap(chrome.group)
                        .cross_alignment(widget::LayoutAlignment::center)
                        .children(
                            ui.make<widget::Label>(std::string(theme::typography_role_name(role)))
                                .color_token(theme::ColorToken::muted_foreground)
                                .font_size(chrome.caption)
                                .width(kRoleColumnWidth),
                            ui.make<widget::Label>(meta)
                                .color_token(theme::ColorToken::muted_foreground)
                                .font_size(chrome.caption)
                                .width(kMetaColumnWidth),
                            ui.expanded().child(ui.make<widget::Label>("Ag 汉字 — 语义角色")
                                                    .color_token(theme::ColorToken::foreground)
                                                    .typography_role(role))
                        )
                );
            }

            return section_block(
                ui,
                chrome,
                "Semantic roles",
                "display/heading/title/subtitle/body/label/caption resolve size + leading + weight "
                "through the theme scale; colors stay independent. A theme may replace "
                "scale.roles[role] without touching the primitive steps.",
                rows
            );
        }

        [[nodiscard]] auto body_default_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);
            auto paragraph =
                ui.make<widget::Label>(
                      "Label 的默认 role 现在是 body：base 字号 + normal 行高（1.5）。"
                      "This paragraph has no explicit size, leading, or role; the body default is "
                      "the only source. CJK paragraphs get the extra breathing room while explicit "
                      "UI labels keep their tighter layout."
                )
                    .color_token(theme::ColorToken::foreground)
                    .width(640.0F)
                    .configure([](widget::Label& label) {
                        label.set_overflow(text::TextOverflow::wrap);
                        label.set_max_lines(6);
                    });

            auto default_meta = ui.make<widget::Label>("Unstyled Label → body role (16 px / 1.5)")
                                    .color_token(theme::ColorToken::muted_foreground)
                                    .font_size(chrome.caption);

            return section_block(
                ui,
                chrome,
                "Body default",
                "Pages can rely on the semantic default for prose; explicit font sizes keep the "
                "backend line height unless the author chooses a leading token.",
                ui.column()
                    .width(widget::authoring::fill)
                    .gap(chrome.group)
                    .cross_alignment(widget::LayoutAlignment::start)
                    .children(paragraph, default_meta)
            );
        }
    } // namespace

    auto TypographyPage::build(app::PageContext& context) -> widget::View {
        const auto& ui = context.ui();
        const auto chrome = make_page_chrome(ui);

        auto content = ui.column()
                           .width(widget::authoring::fill)
                           .max_width(chrome.content_max)
                           .gap(chrome.block)
                           .cross_alignment(widget::LayoutAlignment::stretch);

        auto defer_section = [&context, &content, &ui](
                                 const float estimated_height,
                                 std::move_only_function<widget::View(app::PageContext&)> builder
                             ) -> void {
            auto placeholder =
                ui.column().width(widget::authoring::fill).height(estimated_height).build();
            const auto slot = context.defer_section(placeholder, std::move(builder));
            content.children(std::static_pointer_cast<scene::NanControl>(slot));
        };

        content.children(intro_section(context));
        defer_section(760.0F, [](app::PageContext& ctx) { return scale_section(ctx).build(); });
        defer_section(980.0F, [](app::PageContext& ctx) { return leading_section(ctx).build(); });
        defer_section(760.0F, [](app::PageContext& ctx) { return role_section(ctx).build(); });
        defer_section(360.0F, [](app::PageContext& ctx) {
            return body_default_section(ctx).build();
        });

        return ui.scroll_view()
            .width(widget::authoring::fill)
            .height(widget::authoring::fill)
            .child(ui.padding(foundation::NanInsets::all(chrome.page)).child(content))
            .build();
    }
} // namespace nandina::showcase
