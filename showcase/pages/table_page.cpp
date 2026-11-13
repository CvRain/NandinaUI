//
// showcase/pages/table_page.cpp — 静态 Table 的列轨道 / 自然行高 / 有限宽度展示。
//

#include "table_page.hpp"

#include "../components/page_chrome.hpp"

#include <nandina/widget/authoring.hpp>
#include <nandina/widget/layout.hpp>
#include <nandina/widget/table.hpp>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nandina::showcase
{
    namespace
    {
        inline constexpr float kNarrowTableWidth = 560.0F;

        [[nodiscard]] auto cell_label(
            const widget::BuildContext& ui,
            const PageChrome& chrome,
            const std::string_view text,
            const bool header = false
        ) -> std::shared_ptr<scene::NanControl> {
            return ui.make<widget::Label>(std::string(text))
                .color_token(
                    header ? theme::ColorToken::foreground : theme::ColorToken::muted_foreground
                )
                .font_size(chrome.supporting)
                .width(widget::authoring::fill)
                .build();
        }

        [[nodiscard]] auto wrapping_cell(
            const widget::BuildContext& ui,
            const PageChrome& chrome,
            const std::string_view text
        ) -> std::shared_ptr<scene::NanControl> {
            return ui.make<widget::Label>(std::string(text))
                .color_token(theme::ColorToken::muted_foreground)
                .font_size(chrome.supporting)
                .leading(theme::Leading::normal)
                .width(widget::authoring::fill)
                .configure([](widget::Label& label) {
                    label.set_overflow(text::TextOverflow::wrap);
                    label.set_max_lines(3);
                })
                .build();
        }

        /// 三列：固定 150 / 固定 110 / 权重共享剩余空间。
        [[nodiscard]] auto track_columns() -> std::vector<widget::TableColumn> {
            return {
                widget::TableColumn {.width = 150.0F},
                widget::TableColumn {.width = 110.0F},
                widget::TableColumn {},
            };
        }

        /// 同一份内容分别用于宽/窄两个表格，验证列轨道随可用宽度一致伸缩。
        void populate_track_table(
            const widget::BuildContext& ui,
            const PageChrome& chrome,
            widget::Table& table
        ) {
            table.set_name("table-tracks");
            table.set_column_gap(chrome.group);
            table.set_row_gap(chrome.item);
            table.set_header(
                widget::Table::Cells {
                    cell_label(ui, chrome, "轨道", true),
                    cell_label(ui, chrome, "宽度", true),
                    cell_label(ui, chrome, "行为", true),
                }
            );
            table.add_row(
                widget::Table::Cells {
                    cell_label(ui, chrome, "固定列"),
                    cell_label(ui, chrome, "150 px"),
                    wrapping_cell(
                        ui,
                        chrome,
                        "显式 width 提供有限轨道；两个固定列之外，剩余空间交给权重列。"
                    ),
                }
            );
            table.add_row(
                widget::Table::Cells {
                    cell_label(ui, chrome, "固定列"),
                    cell_label(ui, chrome, "110 px"),
                    wrapping_cell(
                        ui,
                        chrome,
                        "单元格高度取该行内容的自然高度，不做等高强制；长文本按可用宽度换行。"
                    ),
                }
            );
            table.add_row(
                widget::Table::Cells {
                    cell_label(ui, chrome, "权重列"),
                    cell_label(ui, chrome, "flex 1"),
                    wrapping_cell(
                        ui,
                        chrome,
                        "权重列按 flex 比例分配有限宽度；测量无宽度上界时退化为内容最大固有宽度。"
                    ),
                }
            );
        }

        [[nodiscard]] auto tracks_table(const widget::BuildContext& ui, const PageChrome& chrome)
            -> std::shared_ptr<widget::Table> {
            return ui.make<widget::Table>(track_columns())
                .configure([&ui, &chrome](widget::Table& table) {
                    populate_track_table(ui, chrome, table);
                })
                .width(widget::authoring::fill)
                .build();
        }

        [[nodiscard]] auto narrow_table(const widget::BuildContext& ui, const PageChrome& chrome)
            -> std::shared_ptr<widget::Table> {
            return ui.make<widget::Table>(track_columns())
                .configure([&ui, &chrome](widget::Table& table) {
                    populate_track_table(ui, chrome, table);
                })
                .width(kNarrowTableWidth)
                .build();
        }

        [[nodiscard]] auto empty_table(const widget::BuildContext& ui, const PageChrome& chrome)
            -> std::shared_ptr<widget::Table> {
            return ui.make<widget::Table>(track_columns())
                .configure([&ui, &chrome](widget::Table& table) {
                    table.set_name("table-empty");
                    table.set_column_gap(chrome.group);
                    table.set_row_gap(chrome.item);
                    table.set_header(
                        widget::Table::Cells {
                            cell_label(ui, chrome, "轨道", true),
                            cell_label(ui, chrome, "宽度", true),
                            cell_label(ui, chrome, "行为", true),
                        }
                    );
                })
                .width(widget::authoring::fill)
                .build();
        }

        [[nodiscard]] auto density_table(const widget::BuildContext& ui, const PageChrome& chrome)
            -> std::shared_ptr<widget::Table> {
            return ui.make<widget::Table>(track_columns())
                .configure([&ui, &chrome](widget::Table& table) {
                    table.set_name("table-density");
                    table.set_column_gap(chrome.group);
                    table.set_row_gap(chrome.item);
                    table.set_header(
                        widget::Table::Cells {
                            cell_label(ui, chrome, "行", true),
                            cell_label(ui, chrome, "自然高度", true),
                            cell_label(ui, chrome, "说明", true),
                        }
                    );
                    table.add_row(
                        widget::Table::Cells {
                            cell_label(ui, chrome, "1"),
                            cell_label(ui, chrome, "单行"),
                            cell_label(ui, chrome, "短内容保持单行高度。"),
                        }
                    );
                    table.add_row(
                        widget::Table::Cells {
                            cell_label(ui, chrome, "2"),
                            cell_label(ui, chrome, "两行"),
                            wrapping_cell(
                                ui,
                                chrome,
                                "同一行的其他单元格仍按最高内容拉伸对齐，行高由本行内容决定。"
                            ),
                        }
                    );
                    table.add_row(
                        widget::Table::Cells {
                            cell_label(ui, chrome, "3"),
                            cell_label(ui, chrome, "单行"),
                            cell_label(ui, chrome, "行数变化后整表重新测量。"),
                        }
                    );
                })
                .width(widget::authoring::fill)
                .build();
        }

        [[nodiscard]] auto intro_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);
            return ui.column()
                .width(widget::authoring::fill)
                .gap(chrome.block)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(page_heading(ui, chrome, "表格（Table）"))
                .children(supporting_text(
                    ui,
                    chrome,
                    "P4.2b 的静态表格：列轨道由固定宽度与 flex 权重共同决定，行高取自然内容高度。"
                    "表格只负责布局与语义，选择、排序与虚拟化留给后续的 DataTable。",
                    8
                ));
        }

        [[nodiscard]] auto tracks_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);
            return section_block(
                ui,
                chrome,
                "列轨道",
                "第 1 列固定 150 px、第 2 列固定 110 px、第 3 列按 flex 分配剩余宽度。"
                "表头与数据行共用同一组轨道。",
                tracks_table(ui, chrome)
            );
        }

        [[nodiscard]] auto finite_width_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);
            return section_block(
                ui,
                chrome,
                "有限宽度与换行",
                "同一个表格放进 560 px 宽的容器：固定列保持宽度，权重列收窄，长文本按可用宽度"
                "换行并抬高该行。",
                ui.column()
                    .width(widget::authoring::fill)
                    .gap(chrome.group)
                    .cross_alignment(widget::LayoutAlignment::start)
                    .children(narrow_table(ui, chrome))
            );
        }

        [[nodiscard]] auto row_count_section(app::PageContext& context)
            -> widget::authoring::NodeBuilder<widget::Column> {
            const auto& ui = context.ui();
            const auto chrome = make_page_chrome(ui);
            return ui.column()
                .width(widget::authoring::fill)
                .gap(chrome.block)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(section_block(
                    ui,
                    chrome,
                    "空表",
                    "只有表头的表格仍保留列轨道与表头语义；没有数据行时高度只由表头决定。",
                    empty_table(ui, chrome)
                ))
                .children(section_block(
                    ui,
                    chrome,
                    "行数与自然行高",
                    "行内容长度不同会得到不同的自然行高；增删行后整表重新测量，而不是固定行高。",
                    density_table(ui, chrome)
                ));
        }
    } // namespace

    auto TablePage::build(app::PageContext& context) -> widget::View {
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
        defer_section(360.0F, [](app::PageContext& ctx) { return tracks_section(ctx).build(); });
        defer_section(460.0F, [](app::PageContext& ctx) {
            return finite_width_section(ctx).build();
        });
        defer_section(720.0F, [](app::PageContext& ctx) { return row_count_section(ctx).build(); });

        return ui.scroll_view()
            .width(widget::authoring::fill)
            .height(widget::authoring::fill)
            .child(ui.padding(foundation::NanInsets::all(chrome.page)).child(content))
            .build();
    }
} // namespace nandina::showcase
