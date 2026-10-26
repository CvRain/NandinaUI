#ifndef NANDINAUI_LAB_STYLE_HPP
#define NANDINAUI_LAB_STYLE_HPP

#include <nandina/widget/controls.hpp>

#include <string>
#include <string_view>
#include <utility>

namespace nandina::lab
{
    inline constexpr float PAGE_PADDING = 32.0F;
    inline constexpr float CONTENT_MAX = 980.0F;
    inline constexpr float FORM_MAX = 520.0F;
    inline constexpr float TITLE_SIZE = 30.0F;
    inline constexpr float SECTION_SIZE = 18.0F;
    inline constexpr float BODY_SIZE = 14.0F;
    inline constexpr float CAPTION_SIZE = 12.0F;
    inline constexpr int BODY_MAX_LINES = 16;
    inline constexpr float BLOCK_GAP = 20.0F;

    [[nodiscard]] inline auto
    page_title(const widget::BuildContext& ui, const std::string_view text)
        -> widget::authoring::NodeBuilder<widget::Label> {
        return ui.make<widget::Label>(std::string(text))
            .color_token(theme::ColorToken::foreground)
            .font_size(TITLE_SIZE)
            .width(widget::authoring::fill);
    }

    [[nodiscard]] inline auto
    section_title(const widget::BuildContext& ui, const std::string_view text)
        -> widget::authoring::NodeBuilder<widget::Label> {
        return ui.make<widget::Label>(std::string(text))
            .color_token(theme::ColorToken::foreground)
            .font_size(SECTION_SIZE)
            .width(widget::authoring::fill);
    }

    [[nodiscard]] inline auto body_text(
        const widget::BuildContext& ui,
        const std::string_view text,
        int max_lines = BODY_MAX_LINES
    ) -> widget::authoring::NodeBuilder<widget::Label> {
        return ui.make<widget::Label>(std::string(text))
            .color_token(theme::ColorToken::muted_foreground)
            .font_size(BODY_SIZE)
            .width(widget::authoring::fill)
            .configure([max_lines](widget::Label& label) {
                label.set_overflow(text::TextOverflow::wrap);
                label.set_max_lines(max_lines);
            });
    }

    [[nodiscard]] inline auto caption(const widget::BuildContext& ui, const std::string_view text)
        -> widget::authoring::NodeBuilder<widget::Label> {
        return ui.make<widget::Label>(std::string(text))
            .color_token(theme::ColorToken::muted_foreground)
            .font_size(CAPTION_SIZE)
            .width(widget::authoring::fill);
    }

    template<typename Content>
    [[nodiscard]] inline auto card(const widget::BuildContext& ui, Content&& content)
        -> widget::authoring::NodeBuilder<widget::Card> {
        return ui.make<widget::Card>()
            .width(widget::authoring::fill)
            .child(std::forward<Content>(content));
    }

    /// The standard scrolling page column: readable width, 4px-grid spacing, and no
    /// page-local copies of the design tokens.
    [[nodiscard]] inline auto page_column(const widget::BuildContext& ui)
        -> widget::authoring::NodeBuilder<widget::Column> {
        return ui.column()
            .width(widget::authoring::fill)
            .max_width(CONTENT_MAX)
            .gap(BLOCK_GAP)
            .cross_alignment(widget::LayoutAlignment::stretch);
    }
} // namespace nandina::lab

#endif // NANDINAUI_LAB_STYLE_HPP
