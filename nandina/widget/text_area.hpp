//
// widget/text_area — semantic multi-line text input shell.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_TEXT_AREA_HPP
#define NANDINA_EXPERIMENT_WIDGET_TEXT_AREA_HPP

#include "../reactive/event.hpp"
#include "../theme/design_system.hpp"
#include "../theme/theme.hpp"
#include "primitives/editable_text.hpp"
#include "primitives/text.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::widget
{

    /**
     * 多行纯文本输入。
     *
     * 与 TextField 共享 `primitives::EditableText` 的值 / 插入符 / 选区 / 组合输入 /
     * 撤销重做 / 剪贴板 / 命中测试；自身只负责多行外壳：按宽度软换行、`\n` 硬换行、
     * 垂直滚动与行数驱动的默认高度。Enter 插入换行，不触发提交。
     */
    class TextArea: public scene::NanControl {
    public:
        explicit TextArea(std::string value = {}, theme::NanTheme theme = theme::default_theme());

        [[nodiscard]] static auto create(
            std::string value = {},
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<TextArea>;

        void set_value(std::string value);
        [[nodiscard]] auto value() const -> std::string_view;

        void set_placeholder(std::string placeholder);
        [[nodiscard]] auto placeholder() const -> std::string_view;

        void set_read_only(bool value);
        [[nodiscard]] auto read_only() const -> bool;

        void set_disabled(bool value);
        [[nodiscard]] auto disabled() const -> bool;

        /// 可见行数：决定默认高度（行数 × 行高 + 垂直内边距），内容超出后垂直滚动。
        void set_rows(int rows);
        [[nodiscard]] auto rows() const -> int;

        void set_on_change(std::function<void(std::string_view)> callback);
        [[nodiscard]] auto value_changed() const -> const reactive::Event<std::string_view>&;

        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        /// 类型化字段覆盖：只覆盖明确指定的配方字段，系统切换后保留并跟随新快照重解析。
        void set_override(theme::TextAreaRecipeRule rule);

        [[nodiscard]] auto visual_state() const -> theme::TextAreaVisualState;
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedTextAreaStyle;

        [[nodiscard]] auto editable_text() -> primitives::EditableText&;
        [[nodiscard]] auto editable_text() const -> const primitives::EditableText&;

        /// 当前垂直滚动偏移（逻辑像素，x 恒为 0）。与 ScrollView::scroll_offset 同口径。
        [[nodiscard]] auto scroll_offset() const -> foundation::NanPoint;

        void set_text_pipeline(primitives::TextPipeline pipeline);
        [[nodiscard]] auto text_pipeline() const -> primitives::TextPipeline;
        void apply_default_text_pipeline(const primitives::TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;
        void set_font(text::FontRequest request);
        void set_font_family(resource::ResourceKey family);
        void set_font_weight(int weight);
        void set_font_slant(text::FontSlant slant);

        [[nodiscard]] auto is_focusable() const -> bool override;
        auto on_input(scene::InputEvent& event) -> bool override;
        auto on_draw(render::DrawContext& ctx) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;
        auto on_semantics_action(const semantics::ActionRequest& request) -> bool override;

    private:
        void apply_theme();
        void place_caret(foundation::NanPoint screen_point, bool extend);
        void update_scroll(float viewport_height);
        [[nodiscard]] auto content_constraints(scene::LayoutConstraints constraints) const
            -> scene::LayoutConstraints;
        [[nodiscard]] auto content_height() const -> float;
        [[nodiscard]] auto viewport_height() const -> float;
        [[nodiscard]] auto maximum_scroll_y() const -> float;

        primitives::EditableText edit_;
        primitives::Text placeholder_;
        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        /// theme_ref() 兼容视图（tokens + 当前外观 palette）。
        theme::NanTheme theme_view_;
        /// 类型化字段覆盖（每次解析时按当前系统重应用，不冻结）。
        std::optional<theme::TextAreaRecipeRule> override_;
        /// set_theme(NanTheme) 整份覆盖后不再跟随系统切换。
        bool system_explicit_ = false;
        bool font_explicit_ = false;
        /// set_rows 显式覆盖后不再跟随配方默认行数。
        bool rows_explicit_ = false;
        float padding_x_ = 0.0F;
        float padding_y_ = 0.0F;
        float line_height_ = 0.0F;
        int rows_ = 3;
        float scroll_y_ = 0.0F;
        bool focused_ = false;
        bool dragging_ = false;
        bool read_only_ = false;
        bool disabled_ = false;
        std::function<void(std::string_view)> on_change_;
        reactive::Event<std::string_view> value_changed_;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_TEXT_AREA_HPP
