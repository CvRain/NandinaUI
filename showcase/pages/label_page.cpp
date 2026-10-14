//
// Created by cvrain on 2026/10/8.
//
// Label 组件文档页。
//
// 排版约定见 `showcase/components/page_chrome.hpp`：chrome 走主题令牌，间距只用 4px 网格，
// 每个 section 统一是「标题 → 说明 → Card 预览面板」。示例文本的字号是字面量，因为这一页
// 演示的正是 `font_size` —— 样本必须能给出具体数值。

#include "label_page.hpp"

#include "../components/component_info_table.hpp"
#include "../components/page_chrome.hpp"

#include <nandina/widget/controls.hpp>

#include <array>
#include <string>
#include <string_view>

namespace nandina::showcase
{
    namespace
    {
        /// 预览面板里最大的示例字号。
        inline constexpr float kSpecimenSize = 18.0F;
        /// 角色 / 名称列的固定宽度：所有样本据此对齐。
        inline constexpr float kRoleColumnWidth = 104.0F;
        /// 尺寸标注列的固定宽度（右对齐）。
        inline constexpr float kMetaColumnWidth = 56.0F;
        /// Alignment 一节的名称列：比角色列窄，因为名称最长只有 "Center"。
        inline constexpr float kAlignmentColumnWidth = 88.0F;
        /// 正文段落刻意窄于内容列，让换行行为可见。
        inline constexpr float kWrappingWidthPercent = 68.0F;
        /// 三个 overflow 策略共用同一个可用宽度，差异只来自策略本身。
        inline constexpr float kOverflowWidthPercent = 55.0F;

        /// 示例行的三列结构：角色名 | 尺寸标注 | 样本。两列标注固定宽度，样本占满剩余，
        /// 因此所有样本的左边缘严格对齐 —— 这一页最需要的就是这条竖线。
        [[nodiscard]] auto specimen_row(
            const widget::BuildContext& ui,
            const PageChrome& chrome,
            const std::string_view role,
            const std::string_view meta,
            const std::string_view sample,
            const float sample_size
        ) -> widget::authoring::NodeBuilder<widget::Row> {
            return ui.row()
                .width(widget::authoring::fill)
                .gap(chrome.group)
                .cross_alignment(widget::LayoutAlignment::center)
                .children(
                    ui.make<widget::Label>(std::string(role))
                        .color_token(theme::ColorToken::muted_foreground)
                        .font_size(chrome.caption)
                        .width(kRoleColumnWidth),
                    ui.make<widget::Label>(std::string(meta))
                        .color_token(theme::ColorToken::muted_foreground)
                        .font_size(chrome.caption)
                        .width(kMetaColumnWidth)
                        .configure([](widget::Label& label) {
                            label.set_align(theme::TextAlign::end);
                        }),
                    // Row 的**主轴**上 `fill` 会请求整个可用宽度（见 ControlSizeSpec），
                    // 于是后面的兄弟会被推出容器；要「占剩余空间」必须用 Expanded。
                    ui.expanded().child(ui.make<widget::Label>(std::string(sample))
                                            .color_token(theme::ColorToken::foreground)
                                            .font_size(sample_size)
                                            .width(widget::authoring::fill))
                );
        }

        /// 定义列表的一行：键弱值强。元数据里键是次要信息，值才是要读的内容。
        [[nodiscard]] auto definition_row(
            const widget::BuildContext& ui,
            const PageChrome& chrome,
            const std::string_view key,
            const std::string_view value
        ) -> widget::authoring::NodeBuilder<widget::Row> {
            return ui.row()
                .width(widget::authoring::fill)
                .gap(chrome.group)
                .cross_alignment(widget::LayoutAlignment::start)
                .children(
                    ui.make<widget::Label>(std::string(key))
                        .color_token(theme::ColorToken::muted_foreground)
                        .font_size(chrome.supporting)
                        .width(kKeyColumnWidth),
                    ui.expanded().child(ui.make<widget::Label>(std::string(value))
                                            .color_token(theme::ColorToken::foreground)
                                            .font_size(chrome.body)
                                            .width(widget::authoring::fill)
                                            .configure([](widget::Label& label) {
                                                label.set_overflow(text::TextOverflow::wrap);
                                                label.set_max_lines(4);
                                            }))
                );
        }

        auto gen_brief_intro_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            auto info_table =
                ComponentInfoTable {context}
                    .set_file_location("nandina/widget/label.hpp")
                    .set_inherits("primitives::Text")
                    .set_tips({
                        "Uses the active theme unless an instance value overrides it.",
                        "Text wrapping and overflow are configured independently.",
                    })
                    .build()
                    .width(widget::authoring::fill);

            return ui.column()
                .width(widget::authoring::fill)
                .gap(chrome.group)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(page_heading(ui, chrome, "Label"))
                .children(supporting_text(
                    ui,
                    chrome,
                    "Label displays non-interactive text with theme-aware color and typography. "
                    "It also exposes static-text semantics; use TextField or TextArea when the "
                    "user needs to edit text.",
                    4
                ))
                .children(info_table);
        }

        auto gen_typography_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            struct Specimen {
                std::string_view role;
                float size;
                std::string_view meta;
                std::string_view sample;
            };

            // 这一页自己使用的字号阶梯。文档页展示的尺度与它实际使用的尺度一致，
            // 读者不需要去猜"到底哪个才算 body"。
            const std::array specimens {
                Specimen {"Display", chrome.page_title, "32 px", "Label"},
                Specimen {"Section", chrome.section_title, "20 px", "Typography"},
                Specimen {"Body", chrome.body, "16 px", "Default reading text"},
                Specimen {"Supporting", chrome.supporting, "14 px", "Supporting text and detail"},
                Specimen {"Caption", chrome.caption, "12 px", "Metadata and captions"},
            };

            auto rows = ui.column()
                            .width(widget::authoring::fill)
                            .gap(chrome.group)
                            .cross_alignment(widget::LayoutAlignment::stretch);
            for (const auto& [role, size, meta, sample]: specimens) {
                rows.children(specimen_row(ui, chrome, role, meta, sample, size));
            }

            return section_block(
                ui,
                chrome,
                "Typography",
                "Font size is an instance-level choice. The steps below are the ones this page "
                "itself uses: the chrome reads them from theme tokens, and the larger display "
                "sizes are page-level decisions.",
                rows
            );
        }

        auto gen_font_style_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            struct Specimen {
                std::string_view name;
                int weight;
                theme::FontSlant slant;
                std::string_view meta;
            };

            constexpr std::string_view kPangram = "Sphinx of black quartz, judge my vow";
            const std::array specimens {
                Specimen {"Regular", 400, theme::FontSlant::normal, "400"},
                Specimen {"Medium", 500, theme::FontSlant::normal, "500"},
                Specimen {"Bold", 700, theme::FontSlant::normal, "700"},
                Specimen {"Italic", 400, theme::FontSlant::italic, "italic"},
            };

            auto rows = ui.column()
                            .width(widget::authoring::fill)
                            .gap(chrome.group)
                            .cross_alignment(widget::LayoutAlignment::stretch);
            for (const auto& [name, weight, slant, meta]: specimens) {
                rows.children(ui.row()
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
                                          .width(kMetaColumnWidth)
                                          .configure([](widget::Label& label) {
                                              label.set_align(theme::TextAlign::end);
                                          }),
                                      ui.expanded().child(
                                          ui.make<widget::Label>(std::string(kPangram))
                                              .color_token(theme::ColorToken::foreground)
                                              .font_size(kSpecimenSize)
                                              .width(widget::authoring::fill)
                                              .configure([weight, slant](widget::Label& label) {
                                                  label.set_font_weight(weight);
                                                  label.set_font_slant(slant);
                                              })
                                      )
                                  ));
            }

            return section_block(
                ui,
                chrome,
                "Font style",
                "Weight and slant are part of the font request. The same sentence is repeated so "
                "the style difference is the only variable.",
                rows
            );
        }

        auto gen_color_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            struct Role {
                std::string_view name;
                theme::ColorToken token;
                std::string_view usage;
            };

            const std::array roles {
                Role {
                    "foreground",
                    theme::ColorToken::foreground,
                    "Default text on page and card surfaces"
                },
                Role {
                    "muted foreground",
                    theme::ColorToken::muted_foreground,
                    "Secondary text and supporting detail"
                },
                Role {"primary", theme::ColorToken::primary, "Primary actions and selected state"},
                Role {"success", theme::ColorToken::success, "Confirmation and healthy state"},
                Role {"warning", theme::ColorToken::warning, "Caution that still allows progress"},
                Role {
                    "destructive",
                    theme::ColorToken::destructive,
                    "Irreversible or destructive actions"
                },
            };

            auto cells = ui.grid()
                             .configure([&chrome](widget::Grid& grid) {
                                 grid.set_columns(2);
                                 grid.set_column_gap(chrome.block);
                                 grid.set_row_gap(chrome.group);
                             })
                             .width(widget::authoring::fill);

            for (const auto& [name, token, usage]: roles) {
                cells.children(ui.column()
                                   // 占满格子：Grid 的列宽取该列最宽单元格，不给宽度就会
                                   // 变成两列不等宽；percent 的基是 cell_width。
                                   .width(widget::authoring::percent(100.0F))
                                   .gap(chrome.tight)
                                   .cross_alignment(widget::LayoutAlignment::start)
                                   .children(
                                       // 18px 是"大字号"档：状态色作为正文文字不保证 4.5:1，
                                       // 按大字号 3:1 的门槛演示它们才是诚实的做法。
                                       ui.make<widget::Label>(std::string(name))
                                           .color_token(token)
                                           .font_size(kSpecimenSize),
                                       caption_text(ui, chrome, usage)
                                   ));
            }

            return section_block(
                ui,
                chrome,
                "Semantic colors",
                "Color tokens follow the active theme. Prefer semantic roles to fixed color values "
                "when the text should adapt to light and dark appearances.",
                cells
            );
        }

        auto gen_wrapping_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            auto panel =
                ui.column()
                    .width(widget::authoring::fill)
                    .gap(chrome.item)
                    .cross_alignment(widget::LayoutAlignment::stretch)
                    .children(caption_text(ui, chrome, "Assigned width: 68% of the content column"))
                    .children(ui.make<widget::Label>(
                                    "A Label can wrap naturally when its parent provides a "
                                    "finite width. This paragraph mixes English, 中文 and "
                                    "日本語 to show how shaped text flows through a narrower "
                                    "content area without a manually fixed height."
                    )
                                  .width(widget::authoring::percent(kWrappingWidthPercent))
                                  .font_size(chrome.body)
                                  .color_token(theme::ColorToken::foreground)
                                  .configure([](widget::Label& label) {
                                      label.set_overflow(text::TextOverflow::wrap);
                                      label.set_max_lines(20);
                                  }));

            return section_block(
                ui,
                chrome,
                "Wrapping",
                "Wrapping is constrained by the available width, and the label height follows the "
                "resulting line count. Resize the window to inspect the change.",
                panel
            );
        }

        auto gen_overflow_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            constexpr std::string_view kSample {
                "Long labels need a deliberate overflow policy when the available width is limited."
            };

            struct Policy {
                std::string_view name;
                text::TextOverflow overflow;
                int lines;
                std::string_view note;
            };

            // 三个策略共用同一段文字与同一个可用宽度，差异只来自 overflow 本身。
            const std::array policies {
                Policy {"Clip", text::TextOverflow::clip, 1, "cut at the box edge, no marker"},
                Policy {"Ellipsis", text::TextOverflow::ellipsis, 1, "cut with a trailing marker"},
                Policy {"Wrap", text::TextOverflow::wrap, 3, "continued on following lines"},
            };

            auto rows = ui.column()
                            .width(widget::authoring::fill)
                            .gap(chrome.block)
                            .cross_alignment(widget::LayoutAlignment::stretch);
            for (const auto& [name, overflow, lines, note]: policies) {
                rows.children(
                    ui.column()
                        .width(widget::authoring::fill)
                        .gap(chrome.item)
                        .cross_alignment(widget::LayoutAlignment::stretch)
                        .children(
                            caption_text(ui, chrome, std::string(name) + " — " + std::string(note))
                        )
                        .children(ui.make<widget::Label>(std::string(kSample))
                                      .width(widget::authoring::percent(kOverflowWidthPercent))
                                      .font_size(chrome.body)
                                      .color_token(theme::ColorToken::foreground)
                                      .configure([overflow, lines](widget::Label& label) {
                                          label.set_overflow(overflow);
                                          label.set_max_lines(lines);
                                      }))
                );
            }

            return section_block(
                ui,
                chrome,
                "Overflow",
                "All three rows use the same text and the same assigned width, so the only variable "
                "is the policy: clip cuts, ellipsis marks the cut, and wrap continues on the next "
                "line.",
                rows
            );
        }

        auto gen_alignment_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            struct Alignment {
                std::string_view name;
                theme::TextAlign align;
            };

            const std::array alignments {
                Alignment {"Start", theme::TextAlign::start},
                Alignment {"Center", theme::TextAlign::center},
                Alignment {"End", theme::TextAlign::end},
            };

            auto rows = ui.column()
                            .width(widget::authoring::fill)
                            .gap(chrome.item)
                            .cross_alignment(widget::LayoutAlignment::stretch);
            for (const auto& [name, align]: alignments) {
                rows.children(
                    ui.row()
                        .width(widget::authoring::fill)
                        .gap(chrome.group)
                        .cross_alignment(widget::LayoutAlignment::center)
                        .children(
                            ui.make<widget::Label>(std::string(name))
                                .color_token(theme::ColorToken::muted_foreground)
                                .font_size(chrome.caption)
                                .width(kAlignmentColumnWidth),
                            ui.expanded().child(
                                ui.make<widget::Label>(std::string(name) + " aligned text")
                                    .width(widget::authoring::fill)
                                    .height(32.0F)
                                    .font_size(chrome.body)
                                    .color_token(theme::ColorToken::foreground)
                                    .configure([align](widget::Label& label) {
                                        label.set_align(align);
                                    })
                            )
                        )
                );
            }

            return section_block(
                ui,
                chrome,
                "Alignment",
                "Alignment affects line placement inside the wider box each label is given. It does "
                "not change the text content or its semantics.",
                rows
            );
        }

        auto gen_reactive_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            auto& message =
                ui.signal_value(std::string {"The label is bound to a reactive string."});

            auto panel =
                ui.row()
                    .width(widget::authoring::fill)
                    .gap(chrome.group)
                    .cross_alignment(widget::LayoutAlignment::center)
                    .children(
                        ui.expanded().child(ui.make<widget::Label>(message)
                                                .width(widget::authoring::fill)
                                                .font_size(chrome.body)
                                                .color_token(theme::ColorToken::info)
                                                .configure([](widget::Label& label) {
                                                    label.set_overflow(text::TextOverflow::wrap);
                                                    label.set_max_lines(4);
                                                })),
                        ui.make<widget::Button>("Update label")
                            .treatment(theme::ButtonTreatment::outlined)
                            .on_click([&message] {
                                const std::string next = message.get().starts_with("The label")
                                    ? "Reactive text updates without rebuilding the page."
                                    : "The label is bound to a reactive string.";
                                message.set(next);
                            })
                    );

            return section_block(
                ui,
                chrome,
                "Reactive text",
                "Label accepts a string source through BuildContext and updates when that source "
                "changes. The surrounding layout is not rebuilt.",
                panel
            );
        }

        auto gen_semantics_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            auto panel = ui.column()
                             .width(widget::authoring::fill)
                             .gap(chrome.group)
                             .cross_alignment(widget::LayoutAlignment::stretch)
                             .children(definition_row(ui, chrome, "Semantics role", "static text"))
                             .children(supporting_text(
                                 ui,
                                 chrome,
                                 "Label is not an interactive form label and does not forward "
                                 "focus to another control; use TextField or TextArea for editable "
                                 "content.",
                                 4
                             ));

            return section_block(
                ui,
                chrome,
                "Semantics",
                "What assistive technology receives from this component.",
                panel
            );
        }
    } // namespace

    auto LabelPage::build(app::PageContext& context) -> widget::View {
        const auto& ui = context.ui();
        const auto chrome = make_page_chrome(ui);

        // 内容列限制可读宽度：宽屏上正文行长失控是这一页"不好看"的主要原因之一。
        auto content = ui.column()
                           .width(widget::authoring::fill)
                           .max_width(chrome.content_max)
                           .gap(chrome.block)
                           .cross_alignment(widget::LayoutAlignment::stretch);

        content.children(gen_brief_intro_section(context));
        content.children(gen_typography_section(context));
        content.children(gen_font_style_section(context));
        content.children(gen_color_section(context));
        content.children(gen_wrapping_section(context));
        content.children(gen_overflow_section(context));
        content.children(gen_alignment_section(context));
        content.children(gen_reactive_section(context));
        content.children(gen_semantics_section(context));

        return ui.scroll_view()
            .width(widget::authoring::fill)
            .height(widget::authoring::fill)
            .child(ui.padding(foundation::NanInsets::all(chrome.page)).child(content))
            .build();
    }
} // namespace nandina::showcase
