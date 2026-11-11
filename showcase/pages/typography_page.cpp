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
                .children(page_heading(ui, chrome, "排版（Typography）"))
                .children(supporting_text(
                    ui,
                    chrome,
                    "P4.6 第 2 步：兼容 Tailwind 的 TextSize 字号尺度、Leading 行高倍数，"
                    "以及把两者映射到具体用途的语义角色。正文现在通过 body 角色默认使用 "
                    "1.5 行高；每个角色都可以由主题覆盖。",
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
                "字号尺度",
                "十三个兼容 Tailwind 的字号档位；px 值按根基准 × 比例推导，始终为逻辑像素，"
                "OS 内容缩放由渲染器施加。",
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
                "行高决定多行正文的呼吸感：同一段文字在六种行高下对照，只有行距变化，字号与字重"
                "保持一致。样本长度足以折成多行，便于观察段落整体的疏密与节奏。";

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
                "行高",
                "行高是字号的比例倍数。同一段落重复出现，只改变行高；正文默认 normal（1.5）。",
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
                "语义角色",
                "display/heading/title/subtitle/body/label/caption 通过主题尺度解析字号、行高与"
                "字重，颜色保持独立。主题可以替换 scale.roles[role]，而无须改动原始字号档位。",
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
                      "这段文字没有显式设置字号、行高或角色，正文默认值是唯一来源。CJK 段落因此"
                      "获得更充足的呼吸空间，而显式设置的界面标签仍保持更紧凑的布局。"
                )
                    .color_token(theme::ColorToken::foreground)
                    .width(640.0F)
                    .configure([](widget::Label& label) {
                        label.set_overflow(text::TextOverflow::wrap);
                        label.set_max_lines(6);
                    });

            auto default_meta =
                ui.make<widget::Label>("未设样式的 Label → body 角色（16 px / 1.5）")
                    .color_token(theme::ColorToken::muted_foreground)
                    .font_size(chrome.caption);

            return section_block(
                ui,
                chrome,
                "正文默认",
                "页面正文可以直接依赖语义默认值；显式字号在作者未选择行高 token 时沿用后端"
                "默认行高。",
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
