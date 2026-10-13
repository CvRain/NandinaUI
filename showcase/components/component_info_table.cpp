//
// Created by cvrain on 2026/10/11.
//

#include "component_info_table.hpp"
#include "nandina/widget/label.hpp"

#include <utility>

namespace nandina::showcase
{
    ComponentInfoTable::ComponentInfoTable(app::PageContext& context): context_(context) {}

    auto ComponentInfoTable::set_file_location(const std::string& file_location)
        -> ComponentInfoTable& {
        this->file_location_ = file_location;
        return *this;
    }

    auto ComponentInfoTable::set_inherits(const std::string& inherits) -> ComponentInfoTable& {
        this->inherits_ = inherits;
        return *this;
    }
    auto ComponentInfoTable::set_tips(const std::vector<std::string>& tips) -> ComponentInfoTable& {
        this->tips_ = tips;
        return *this;
    }
    auto ComponentInfoTable::get_file_location() const -> std::string {
        return this->file_location_;
    }
    auto ComponentInfoTable::get_inherits() const -> std::string {
        return this->inherits_;
    }
    auto ComponentInfoTable::get_tips() const -> std::vector<std::string> {
        return this->tips_;
    }

    auto
    ComponentInfoTable::field_row(const std::string_view key, const std::string_view value) const
        -> widget::authoring::NodeBuilder<widget::Row> {
        const auto& ui = context_.ui();
        const auto chrome = make_page_chrome(ui);

        const auto key_label = ui.make<widget::Label>(std::string(key))
                                   .color_token(theme::ColorToken::muted_foreground)
                                   .font_size(chrome.supporting)
                                   .width(kKeyColumnWidth);

        const auto value_label = ui.make<widget::Label>(std::string(value))
                                     .color_token(theme::ColorToken::foreground)
                                     .font_size(chrome.body)
                                     .width(widget::authoring::fill)
                                     .configure([](widget::Label& label) {
                                         label.set_overflow(text::TextOverflow::wrap);
                                         label.set_max_lines(4);
                                     });

        return ui.row()
            .width(widget::authoring::fill)
            .gap(chrome.group)
            .cross_alignment(widget::LayoutAlignment::start)
            .children(key_label, value_label);
    }

    auto ComponentInfoTable::build() -> widget::authoring::NodeBuilder<widget::Card> {
        const auto& ui = context_.ui();
        const auto chrome = make_page_chrome(ui);

        auto rows = ui.column()
                        .width(widget::authoring::fill)
                        .gap(chrome.group)
                        .cross_alignment(widget::LayoutAlignment::stretch);

        rows.children(field_row("File location", this->file_location_));
        rows.children(field_row("Inherits", this->inherits_));

        if (not this->tips_.empty()) {
            auto bullets = ui.column()
                               .width(widget::authoring::fill)
                               .gap(chrome.item)
                               .cross_alignment(widget::LayoutAlignment::stretch);

            for (const auto& one_tip: tips_) {
                bullets.children(ui.row()
                                     .width(widget::authoring::fill)
                                     .gap(chrome.item)
                                     .cross_alignment(widget::LayoutAlignment::start)
                                     .children(
                                         ui.make<widget::Label>("•")
                                             .color_token(theme::ColorToken::muted_foreground)
                                             .font_size(chrome.supporting),
                                         supporting_text(ui, chrome, one_tip, 3)
                                     ));
            }

            rows.children(ui.column()
                              .width(widget::authoring::fill)
                              .gap(chrome.item)
                              .cross_alignment(widget::LayoutAlignment::stretch)
                              .children(caption_text(ui, chrome, "Tips"))
                              .children(bullets));
        }

        return example_surface(ui, rows);
    }
} // namespace nandina::showcase
