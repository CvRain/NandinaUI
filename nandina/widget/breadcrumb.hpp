//
// widget/breadcrumb - horizontal trail of links showing the current location.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_BREADCRUMB_HPP
#define NANDINA_EXPERIMENT_WIDGET_BREADCRUMB_HPP

#include "../scene/control.hpp"
#include "../theme/design_system.hpp"
#include "primitives/text.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::widget
{
    namespace internal
    {
        class BreadcrumbLink;
    } // namespace internal

    /**
     * 面包屑：用一行"链接 + 分隔符"标出当前页面在层级中的位置，最后一项是当前页
     * （不是链接）。
     *
     * 可点击条目是内部 `BreadcrumbLink`（`Pressable` 派生）子控件，因此它自己拥有
     * Tab 焦点、指针命中、hover / pressed 反馈与 Enter / Space 激活；无回调的条目是
     * 容器自绘的纯文本。分隔符默认 `/`，可整体替换。
     *
     * 平台语义树没有 nav / breadcrumb role，因此容器暴露 `Role::generic`，label 为
     * 用分隔符连接的整条路径；可点击条目自带 `Role::button`（平台的既有链接角色）。
     */
    class Breadcrumb: public scene::NanControl {
    public:
        explicit Breadcrumb(theme::NanTheme theme = theme::default_theme());

        [[nodiscard]] static auto create(theme::NanTheme theme = theme::default_theme())
            -> std::shared_ptr<Breadcrumb>;

        /// 追加一个条目；`on_click` 为空表示当前页 / 纯文本（不可交互）。
        void add_item(std::string label, std::function<void()> on_click = {});
        /// 移除全部条目（含内部链接子控件）。
        void clear();
        [[nodiscard]] auto item_count() const -> std::size_t;
        [[nodiscard]] auto item_label(std::size_t index) const -> std::string_view;
        /// 该条目是否可交互（带回调）。
        [[nodiscard]] auto item_clickable(std::size_t index) const -> bool;

        /// 分隔符字形（默认 `/`）。
        void set_separator(std::string separator);
        [[nodiscard]] auto separator() const -> std::string_view;

        /// 高级接口：以完整 NanTheme 覆盖控件主题（不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        /// 当前生效主题视图（tokens + 当前外观 palette），遗留读取兼容。
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        /// 类型化字段覆盖：只覆盖明确指定的配方字段，系统切换后保留并跟随新快照重解析。
        void set_override(theme::BreadcrumbRecipeRule rule);
        [[nodiscard]] auto visual_state() const -> theme::BreadcrumbVisualState;
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedBreadcrumbStyle;

        /// 仅供内部 `primitives::Text`；链接子控件由场景树自行传播。
        void apply_default_text_pipeline(const primitives::TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;

        auto on_draw(render::DrawContext& context) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;
        void on_ready() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        struct Item {
            std::string label;
            /// 可点击条目：内部链接子控件（拥有焦点与点击）。
            std::shared_ptr<internal::BreadcrumbLink> link;
            /// 非交互条目：容器自绘的文本。
            std::shared_ptr<primitives::Text> text;
            foundation::NanRect rect;
            float width = 0.0F;
            float height = 0.0F;
        };

        struct TrailLayout {
            float width = 0.0F;
            float height = 0.0F;
        };

        /// 刷新文本样式、测量条目并填好 `rect` / 分隔符位置。返回整体尺寸。
        [[nodiscard]] auto measure_trail(float available_width) -> TrailLayout;
        void apply_text_styles();
        void relayout();

        std::vector<Item> items_;
        std::vector<foundation::NanRect> separator_rects_;
        std::string separator_ = "/";
        primitives::Text separator_text_ {"/"};
        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        /// theme_ref() 兼容视图（tokens + 当前外观 palette）。
        theme::NanTheme theme_view_;
        /// 类型化字段覆盖（每次解析时按当前系统重应用，不冻结）。
        std::optional<theme::BreadcrumbRecipeRule> override_;
        /// set_theme(NanTheme) 整份覆盖后不再跟随系统切换。
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_BREADCRUMB_HPP
