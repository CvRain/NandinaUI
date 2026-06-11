module;

#include <memory>
#include <string>
#include <string_view>

export module nandina.widgets.tag;

import nandina.widgets.surface;
import nandina.widgets.text;
import nandina.foundation.nan_insets;
import nandina.text.nan_font;
import nandina.theme.nan_style;

export namespace nandina::widgets {

    using ColorVariant = nandina::theme::ColorVariant;
    using TagSize = nandina::theme::TagSize;

    class Tag : public Surface {
    public:
        using Ptr = std::unique_ptr<Tag>;

        ~Tag() override = default;

        static auto create() -> Ptr {
            return Ptr{new Tag()};
        }

        auto text(std::string_view value) -> Tag& {
            m_label->set_text(value);
            return *this;
        }

        [[nodiscard]] auto text() const noexcept -> const std::string& {
            return m_label->text();
        }

        auto size(TagSize value) -> Tag& {
            if (m_size == value) {
                return *this;
            }

            m_size = value;
            apply_size_style();
            return *this;
        }

        [[nodiscard]] auto size() const noexcept -> TagSize {
            return m_size;
        }

        auto color_variant(ColorVariant value) -> Tag& {
            if (m_color_variant == value) {
                return *this;
            }

            m_color_variant = value;
            sync_visual_state();
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto color_variant() const noexcept -> ColorVariant {
            return m_color_variant;
        }

        auto disabled(bool value) -> Tag& {
            if (m_disabled == value) {
                return *this;
            }

            m_disabled = value;
            sync_visual_state();
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto disabled() const noexcept -> bool {
            return m_disabled;
        }

        auto font(text::NanFont value) -> Tag& {
            m_label->set_font(std::move(value));
            sync_visual_state();
            mark_layout_dirty();
            return *this;
        }

        [[nodiscard]] auto font() const noexcept -> const text::NanFont& {
            return m_label->font();
        }

        [[nodiscard]] auto label() noexcept -> Text& {
            return *m_label;
        }

        [[nodiscard]] auto label() const noexcept -> const Text& {
            return *m_label;
        }

    private:
        struct ResolvedColors {
            nandina::NanColor bg;
            nandina::NanColor text;
            nandina::NanColor border;
        };

        Tag() {
            auto label = Text::create();
            label->set_align(TextAlign::Center);
            label->set_vertical_align(TextVerticalAlign::Center);
            m_label = label.get();
            add_child(std::move(label));

            const auto& style = nandina::theme::NanStylePrimitives::current().tag;
            m_size = style.size;
            m_color_variant = style.color_variant;

            set_corner_radius(style.corner_radius);
            set_border_width(style.border_width);
            apply_size_style();
            sync_visual_state();
        }

        [[nodiscard]] auto size_style() const -> const nandina::theme::NanTagStyle::SizeStyle& {
            const auto& style = nandina::theme::NanStylePrimitives::current().tag;
            switch (m_size) {
            case TagSize::sm:
                return style.sm;
            case TagSize::lg:
                return style.lg;
            case TagSize::md:
                break;
            }
            return style.md;
        }

        [[nodiscard]] auto resolved_colors() const -> ResolvedColors {
            const auto& style = nandina::theme::NanStylePrimitives::current().tag;
            const auto resolved_variant = m_color_variant == ColorVariant::inherit
                ? ColorVariant::primary
                : m_color_variant;

            const auto make_colors = [this](const nandina::theme::NanTagStyle::ColorFamilyStyle& family) {
                if (m_disabled) {
                    return ResolvedColors{
                        .bg = family.bg_disabled,
                        .text = family.text_disabled,
                        .border = family.border_disabled,
                    };
                }

                return ResolvedColors{
                    .bg = family.bg,
                    .text = family.text,
                    .border = family.border,
                };
            };

            switch (resolved_variant) {
            case ColorVariant::secondary:
                return make_colors(style.secondary_family);
            case ColorVariant::neutral:
                return make_colors(style.neutral_family);
            case ColorVariant::destructive:
                return make_colors(style.destructive_family);
            case ColorVariant::primary:
            case ColorVariant::inherit:
                break;
            }

            if (m_disabled) {
                return ResolvedColors{
                    .bg = style.bg_disabled,
                    .text = style.text_disabled,
                    .border = style.border_disabled,
                };
            }

            return ResolvedColors{
                .bg = style.bg,
                .text = style.text,
                .border = style.border,
            };
        }

        auto apply_size_style() -> void {
            const auto& style = nandina::theme::NanStylePrimitives::current().tag;
            const auto& size = size_style();

            set_padding(geometry::NanInsets{size.padding_h, size.padding_v, size.padding_h, size.padding_v});
            m_label->update_font([&](text::NanFont& font) {
                font.size(size.font_size)
                    .weight(style.font_weight)
                    .overflow(style.overflow)
                    .single_line(style.single_line);
            });
            sync_visual_state();
            mark_layout_dirty();
        }

        auto sync_visual_state() -> void {
            const auto& style = nandina::theme::NanStylePrimitives::current().tag;
            const auto colors = resolved_colors();

            set_bg_color(colors.bg);
            set_border_color(colors.border);
            set_border_width(style.border_width);
            set_corner_radius(style.corner_radius);
            m_label->update_font([&](text::NanFont& font) {
                font.color(colors.text);
            });
        }

        Text* m_label{nullptr};
        TagSize m_size{TagSize::md};
        ColorVariant m_color_variant{ColorVariant::inherit};
        bool m_disabled{false};
    };

} // namespace nandina::widgets