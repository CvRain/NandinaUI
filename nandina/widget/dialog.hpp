//
// widget/dialog - modal overlay (scrim + centered panel + focus trap).
//

#ifndef NANDINA_EXPERIMENT_WIDGET_DIALOG_HPP
#define NANDINA_EXPERIMENT_WIDGET_DIALOG_HPP

#include "../animation/animated_property.hpp"
#include "../scene/control.hpp"
#include "../theme/design_system.hpp"
#include "primitives/text.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::widget
{
    class Dialog: public scene::NanControl {
    public:
        explicit Dialog(theme::NanTheme theme = theme::default_theme());

        [[nodiscard]] static auto create(theme::NanTheme theme = theme::default_theme())
            -> std::shared_ptr<Dialog>;

        void set_title(std::string title);
        [[nodiscard]] auto title() const -> std::string_view;

        /// 面板主体内容（按钮 / 文本等），成为 Dialog 的子节点并布局在标题下方。
        auto set_content(std::shared_ptr<scene::NanControl> content) -> Dialog&;

        void open();
        void close();
        [[nodiscard]] auto is_open() const -> bool;

        /// false 时禁用 Escape / 点击遮罩关闭（强制模态，需程序化 close）。
        void set_dismissible(bool dismissible);
        [[nodiscard]] auto dismissible() const -> bool;

        void set_on_close(std::function<void()> callback);

        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        void set_override(theme::DialogRecipeRule rule);
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedDialogStyle;

        void set_text_pipeline(primitives::TextPipeline pipeline);
        void apply_default_text_pipeline(const primitives::TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;

        /// 打开时提升 z 序，浮在所有兄弟之上。
        [[nodiscard]] auto z_index_hint() const -> int override;
        /// 打开时把整个父容器纳入包围盒（遮罩覆盖全屏）。
        [[nodiscard]] auto global_bounds() const -> foundation::NanRect override;
        [[nodiscard]] auto contains_point(foundation::NanPoint local_point) const -> bool override;
        [[nodiscard]] auto is_focusable() const -> bool override;
        auto on_input(scene::InputEvent& event) -> bool override;
        auto on_draw(render::DrawContext& context) -> void override;
        void on_process(float dt) override;

        /// 淡入淡出通过 per-node opacity 作用于整个子树（含内容子节点），由场景树
        /// AnimationHost 推进 fade_；此处与任何显式 local_opacity 相乘。
        [[nodiscard]] auto local_opacity() const -> float override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        enum class DialogPhase { closed, opening, opened, closing };

        void apply_text_style();
        [[nodiscard]] auto panel_rect() const -> foundation::NanRect;
        void trap_focus(bool backwards);
        void start_fade(float target);
        [[nodiscard]] auto active() const noexcept -> bool {
            return phase_ != DialogPhase::closed;
        }

        primitives::Text title_text_;
        std::weak_ptr<scene::NanControl> content_;
        animation::AnimatedProperty<float> fade_ {0.0F};
        DialogPhase phase_ = DialogPhase::closed;
        bool dismissible_ = true;
        std::function<void()> on_close_;
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        theme::NanTheme theme_view_;
        std::optional<theme::DialogRecipeRule> override_;
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_DIALOG_HPP
