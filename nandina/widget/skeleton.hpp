//
// widget/skeleton - loading placeholder (pure display, no interaction).
//

#ifndef NANDINA_EXPERIMENT_WIDGET_SKELETON_HPP
#define NANDINA_EXPERIMENT_WIDGET_SKELETON_HPP

#include "../scene/control.hpp"
#include "../theme/design_system.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::widget
{
    /**
     * 骨架屏形状：
     * - `text`：文本行占位，可多行堆叠，末行按 `last_line_ratio` 收窄；
     * - `rectangle`：整块矩形占位，填充请求到的尺寸。
     */
    enum class SkeletonVariant: std::uint8_t {
        text,
        rectangle,
    };

    /// 加载占位块：内容就绪前占据最终布局位置，避免加载完成时页面跳动。
    class Skeleton: public scene::NanControl {
    public:
        explicit Skeleton(theme::NanTheme theme = theme::default_theme());

        [[nodiscard]] static auto create(theme::NanTheme theme = theme::default_theme())
            -> std::shared_ptr<Skeleton>;

        void set_variant(SkeletonVariant variant);
        [[nodiscard]] auto variant() const -> SkeletonVariant;

        /// 文本占位的行数；越界值钳制到 >= 1。`rectangle` 变体忽略该值。
        void set_lines(int lines);
        [[nodiscard]] auto lines() const -> int;

        /// 无障碍标签（可选，仅进 semantics）。
        void set_label(std::string label);
        [[nodiscard]] auto label() const -> std::string_view;

        /// 高级接口：以完整 NanTheme 覆盖控件主题（不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        /// 当前生效主题视图（tokens + 当前外观 palette），遗留读取兼容。
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        /// 类型化字段覆盖：只覆盖明确指定的配方字段，系统切换后保留并跟随新快照重解析。
        void set_override(theme::SkeletonRecipeRule rule);
        [[nodiscard]] auto visual_state() const -> theme::SkeletonVisualState;
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedSkeletonStyle;

        void on_theme_changed(const theme::ThemeManager& manager) override;
        auto on_draw(render::DrawContext& context) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        SkeletonVariant variant_ = SkeletonVariant::text;
        int lines_ = 1;
        std::string label_;
        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        /// theme_ref() 兼容视图（tokens + 当前外观 palette）。
        theme::NanTheme theme_view_;
        /// 类型化字段覆盖（每次解析时按当前系统重应用，不冻结）。
        std::optional<theme::SkeletonRecipeRule> override_;
        /// set_theme(NanTheme) 整份覆盖后不再跟随系统切换。
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_SKELETON_HPP
