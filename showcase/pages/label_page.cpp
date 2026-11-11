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

            auto info_table = ComponentInfoTable {context}
                                  .set_file_location("nandina/widget/label.hpp")
                                  .set_inherits("primitives::Text")
                                  .set_tips({
                                      "默认使用当前主题，实例上的取值可以覆盖。",
                                      "换行与溢出各自独立配置。",
                                  })
                                  .build()
                                  .width(widget::authoring::fill);

            return ui.column()
                .width(widget::authoring::fill)
                .gap(chrome.group)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(page_heading(ui, chrome, "标签（Label）"))
                .children(supporting_text(
                    ui,
                    chrome,
                    "Label 用于显示不可交互的文本，颜色与排版跟随主题。它同时导出静态文本语义；"
                    "需要让用户编辑文本时，请改用 TextField 或 TextArea。",
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
                Specimen {"展示", chrome.page_title, "32 px", "Label"},
                Specimen {"章节", chrome.section_title, "20 px", "字号阶梯"},
                Specimen {"正文", chrome.body, "16 px", "默认阅读文本"},
                Specimen {"辅助", chrome.supporting, "14 px", "辅助信息与细节"},
                Specimen {"图注", chrome.caption, "12 px", "元数据与图注"},
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
                "字号阶梯",
                "字号由实例决定。下面几档正是本页自己使用的字号：页面框架从主题令牌读取，"
                "更大的展示字号则属于页面级决策。",
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

            constexpr std::string_view kPangram = "天地玄黄，宇宙洪荒。日月盈昃，辰宿列张。";
            const std::array specimens {
                Specimen {"常规", 400, theme::FontSlant::normal, "400"},
                Specimen {"中等", 500, theme::FontSlant::normal, "500"},
                Specimen {"粗体", 700, theme::FontSlant::normal, "700"},
                Specimen {"斜体", 400, theme::FontSlant::italic, "italic"},
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
                "字体样式",
                "字重与倾斜是字体请求的一部分。每行重复同一句话，"
                "让样式差异成为唯一的变量。",
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
                Role {"foreground", theme::ColorToken::foreground, "页面与卡片表面上的默认文字"},
                Role {
                    "muted foreground",
                    theme::ColorToken::muted_foreground,
                    "次要文字与辅助细节"
                },
                Role {"primary", theme::ColorToken::primary, "主要操作与选中状态"},
                Role {"success", theme::ColorToken::success, "确认反馈与正常状态"},
                Role {"warning", theme::ColorToken::warning, "提醒谨慎，但仍可继续操作"},
                Role {"destructive", theme::ColorToken::destructive, "不可撤销或具破坏性的操作"},
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
                "语义颜色",
                "颜色令牌跟随当前主题。当文字需要同时适配明暗外观时，"
                "请优先使用语义角色，而不是固定色值。",
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
                    .children(caption_text(ui, chrome, "指定宽度：内容列的 68%"))
                    .children(ui.make<widget::Label>(
                                    "父级给出有限宽度时，Label 便能自然换行。这段文字混排了 "
                                    "English、中文与日本語，用来展示整形后的文本如何流入更窄的 "
                                    "内容区域，而无需手动固定高度。"
                    )
                                  .width(widget::authoring::percent(kWrappingWidthPercent))
                                  .font_size(chrome.body)
                                  .leading(theme::Leading::normal)
                                  .color_token(theme::ColorToken::foreground)
                                  .configure([](widget::Label& label) {
                                      label.set_overflow(text::TextOverflow::wrap);
                                      label.set_max_lines(20);
                                  }));

            return section_block(
                ui,
                chrome,
                "换行",
                "换行受可用宽度约束，Label 的高度由实际行数决定。调整窗口大小即可观察变化。",
                panel
            );
        }

        auto gen_overflow_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            constexpr std::string_view kSample {
                "当可用宽度不足时，较长的文本需要明确选择溢出策略。"
            };

            struct Policy {
                std::string_view name;
                text::TextOverflow overflow;
                int lines;
                std::string_view note;
            };

            // 三个策略共用同一段文字与同一个可用宽度，差异只来自 overflow 本身。
            const std::array policies {
                Policy {"裁剪", text::TextOverflow::clip, 1, "在边界处直接截断，不留标记"},
                Policy {"省略号", text::TextOverflow::ellipsis, 1, "截断后以尾部标记提示"},
                Policy {"换行", text::TextOverflow::wrap, 3, "在后续行继续显示"},
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
                                      .leading(theme::Leading::normal)
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
                "溢出",
                "三行使用同一段文字和同一个指定宽度，唯一的变量是策略本身："
                "裁剪直接截断，省略号标出截断位置，换行则延续到下一行。",
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
                Alignment {"起始", theme::TextAlign::start},
                Alignment {"居中", theme::TextAlign::center},
                Alignment {"末尾", theme::TextAlign::end},
            };

            auto rows = ui.column()
                            .width(widget::authoring::fill)
                            .gap(chrome.item)
                            .cross_alignment(widget::LayoutAlignment::stretch);
            for (const auto& [name, align]: alignments) {
                rows.children(ui.row()
                                  .width(widget::authoring::fill)
                                  .gap(chrome.group)
                                  .cross_alignment(widget::LayoutAlignment::center)
                                  .children(
                                      ui.make<widget::Label>(std::string(name))
                                          .color_token(theme::ColorToken::muted_foreground)
                                          .font_size(chrome.caption)
                                          .width(kAlignmentColumnWidth),
                                      ui.expanded().child(
                                          ui.make<widget::Label>(std::string(name) + "对齐的文本")
                                              .width(widget::authoring::fill)
                                              .height(32.0F)
                                              .font_size(chrome.body)
                                              .color_token(theme::ColorToken::foreground)
                                              .configure([align](widget::Label& label) {
                                                  label.set_align(align);
                                              })
                                      )
                                  ));
            }

            return section_block(
                ui,
                chrome,
                "对齐",
                "对齐只决定每行文字在 Label 给定宽度内的位置，不改变文本内容，也不改变语义。",
                rows
            );
        }

        auto gen_reactive_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);

            auto& message = ui.signal_value(std::string {"这个 Label 绑定了一个响应式字符串。"});

            auto panel =
                ui.row()
                    .width(widget::authoring::fill)
                    .gap(chrome.group)
                    .cross_alignment(widget::LayoutAlignment::center)
                    .children(
                        ui.expanded().child(ui.make<widget::Label>(message)
                                                .width(widget::authoring::fill)
                                                .font_size(chrome.body)
                                                .leading(theme::Leading::normal)
                                                .color_token(theme::ColorToken::info)
                                                .configure([](widget::Label& label) {
                                                    label.set_overflow(text::TextOverflow::wrap);
                                                    label.set_max_lines(4);
                                                })),
                        ui.make<widget::Button>("更新 Label")
                            .treatment(theme::ButtonTreatment::outlined)
                            .on_click([&message] {
                                const std::string next = message.get().starts_with("这个 Label")
                                    ? "响应式文本更新时无需重建页面。"
                                    : "这个 Label 绑定了一个响应式字符串。";
                                message.set(next);
                            })
                    );

            return section_block(
                ui,
                chrome,
                "响应式文本",
                "Label 通过 BuildContext 接收字符串数据源，数据源变化时自动更新，"
                "周围布局不会重建。",
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
                             .children(definition_row(ui, chrome, "语义角色", "静态文本"))
                             .children(supporting_text(
                                 ui,
                                 chrome,
                                 "Label 不是可交互的表单标签，也不会把焦点转交给其他控件；"
                                 "需要可编辑内容时，请使用 TextField 或 TextArea。",
                                 4
                             ));

            return section_block(ui, chrome, "语义", "辅助技术能从该组件获取的信息。", panel);
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

        // 文本密集型页面：首帧只构建页面壳、简介与 Typography，其余节按帧渐进挂载，
        // 避免导航后整页同步构建+首次整形把 UI 冻住数百毫秒（见
        // docs/references/page_load_responsiveness.md）。占位高度只影响首帧滚动范围，
        // 真实节挂载后由布局自然收回。
        auto defer_section = [&context, &content, &ui](
                                 const float estimated_height,
                                 std::move_only_function<widget::View(app::PageContext&)> builder
                             ) -> void {
            auto placeholder =
                ui.column().width(widget::authoring::fill).height(estimated_height).build();
            const auto slot = context.defer_section(placeholder, std::move(builder));
            // Placeholder and every section builder in this page are controls; the deferred API
            // returns the generic View so the router can replace any node.
            content.children(std::static_pointer_cast<scene::NanControl>(slot));
        };

        content.children(gen_brief_intro_section(context));
        defer_section(320.0F, [](app::PageContext& ctx) {
            return gen_typography_section(ctx).build();
        });
        defer_section(340.0F, [](app::PageContext& ctx) {
            return gen_font_style_section(ctx).build();
        });
        defer_section(400.0F, [](app::PageContext& ctx) { return gen_color_section(ctx).build(); });
        defer_section(260.0F, [](app::PageContext& ctx) {
            return gen_wrapping_section(ctx).build();
        });
        defer_section(260.0F, [](app::PageContext& ctx) {
            return gen_overflow_section(ctx).build();
        });
        defer_section(340.0F, [](app::PageContext& ctx) {
            return gen_alignment_section(ctx).build();
        });
        defer_section(200.0F, [](app::PageContext& ctx) {
            return gen_reactive_section(ctx).build();
        });
        defer_section(220.0F, [](app::PageContext& ctx) {
            return gen_semantics_section(ctx).build();
        });

        return ui.scroll_view()
            .width(widget::authoring::fill)
            .height(widget::authoring::fill)
            .child(ui.padding(foundation::NanInsets::all(chrome.page)).child(content))
            .build();
    }
} // namespace nandina::showcase
