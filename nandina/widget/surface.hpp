// Surface is a single-child layout primitive with a theme-resolved box background.
#ifndef NANDINA_EXPERIMENT_WIDGET_SURFACE_HPP
#define NANDINA_EXPERIMENT_WIDGET_SURFACE_HPP

#include "layout.hpp"
#include "primitives/box_presentation.hpp"

namespace nandina::widget
{
    /// Border and radius are paint-only. Child clipping remains an explicit rectangular
    /// ControlOverflow policy; a rounded background does not create a rounded child mask.
    class Surface: public Padding {
    public:
        explicit Surface(theme::NanTheme theme = theme::default_theme());
        [[nodiscard]] static auto create(theme::NanTheme theme = theme::default_theme())
            -> std::shared_ptr<Surface>;

        auto set_padding(foundation::NanInsets insets) -> Surface&;
        void set_box_style(theme::BoxStyle style);
        [[nodiscard]] auto box_style() const noexcept -> const theme::BoxStyle&;
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedBoxStyle;
        [[nodiscard]] auto visual_part(visual::container_t) noexcept
            -> primitives::BoxPresentation&;

        void on_theme_changed(const theme::ThemeManager& manager) override;
        void on_draw(render::DrawContext& context) override;

    private:
        theme::NanTheme theme_;
        theme::BoxStyle box_ {
            .fill = theme::ThemeColor::token(theme::ColorToken::surface),
            .border = theme::ThemeColor::token(theme::ColorToken::border),
            .border_width = theme::ThemeScalar::literal(0.0F),
            .radius = theme::ThemeScalar::literal(0.0F),
        };
        primitives::BoxPresentation presentation_;
    };
} // namespace nandina::widget
#endif // NANDINA_EXPERIMENT_WIDGET_SURFACE_HPP
