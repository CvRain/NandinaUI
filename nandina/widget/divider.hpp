//
// widget/divider - horizontal/vertical separator line (pure display).
//

#ifndef NANDINA_EXPERIMENT_WIDGET_DIVIDER_HPP
#define NANDINA_EXPERIMENT_WIDGET_DIVIDER_HPP

#include "../scene/control.hpp"
#include "../theme/design_system.hpp"

#include <memory>
#include <optional>

namespace nandina::widget
{
    class Divider: public scene::NanControl {
    public:
        enum class Orientation {
            horizontal,
            vertical,
        };

        enum class Pattern {
            solid,
            dashed,
            double_line,
            double_dashed,
        };

        explicit Divider(theme::NanTheme theme = theme::default_theme());

        [[nodiscard]] static auto create(theme::NanTheme theme = theme::default_theme())
            -> std::shared_ptr<Divider>;

        void set_orientation(Orientation orientation);
        [[nodiscard]] auto orientation() const -> Orientation;
        void set_pattern(Pattern pattern);
        [[nodiscard]] auto pattern() const -> Pattern;
        /// 虚线每段的长度（逻辑单位）；≤0 时按厚度 ×4 推导。双线的间距 = 厚度。
        void set_dash_length(float length);
        [[nodiscard]] auto dash_length() const -> float;

        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        void set_override(theme::DividerRecipeRule rule);
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedDividerStyle;
        void on_theme_changed(const theme::ThemeManager& manager) override;

        auto on_draw(render::DrawContext& context) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        Orientation orientation_ = Orientation::horizontal;
        Pattern pattern_ = Pattern::solid;
        float dash_length_ = 0.0F;
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        theme::NanTheme theme_view_;
        std::optional<theme::DividerRecipeRule> override_;
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_DIVIDER_HPP
