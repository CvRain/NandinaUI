//
// widget/dialog - modal overlay (scrim + centered panel + focus trap).
//

#ifndef NANDINA_EXPERIMENT_WIDGET_DIALOG_HPP
#define NANDINA_EXPERIMENT_WIDGET_DIALOG_HPP

#include "../scene/control.hpp"
#include "../theme/design_system.hpp"
#include "primitives/text.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::scene
{
    class OverlayHandle;
    class OverlayHost;
}

namespace nandina::widget
{
    template<typename Component>
    struct ComponentTraits;

    namespace internal
    {
        class DialogPanel;
        class DismissLayer;
        class FocusScope;
    } // namespace internal

    /**
     * 模态对话框：遮罩、居中面板与焦点限制。
     *
     * 打开时把面板托管到窗口级浮层：遮罩与输入阻断由 DismissLayer 提供，焦点限制与关闭后的
     * 焦点恢复由 FocusScope 提供，因此面板不会被 ScrollView / Card 之类的裁剪容器截断。
     * 没有窗口浮层服务时（detached 上下文）回退为树内模态，公开 API 与行为保持不变。
     */
    class Dialog: public scene::NanControl {
    public:
        explicit Dialog(theme::NanTheme theme = theme::default_theme());
        ~Dialog() override;

        [[nodiscard]] static auto create(theme::NanTheme theme = theme::default_theme())
            -> std::shared_ptr<Dialog>;

        /// header 槽位的文本便捷入口；与 set_header() 互斥，后设置者生效。
        void set_title(std::string title);
        [[nodiscard]] auto title() const -> std::string_view;

        /// 固定语义槽位：header / content / footer，缺省为空且不占高度。
        auto set_header(std::shared_ptr<scene::NanControl> header) -> Dialog&;
        auto set_content(std::shared_ptr<scene::NanControl> content) -> Dialog&;
        auto set_footer(std::shared_ptr<scene::NanControl> footer) -> Dialog&;

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

        /// 树内回退时提升 z 序，使模态面板浮在后续兄弟之上；浮层承载时由层级顺序决定。
        [[nodiscard]] auto z_index_hint() const -> int override;
        void on_process(float dt) override;
        void on_exit_tree() override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        friend struct ComponentTraits<Dialog>;

        enum class DialogPhase { closed, opening, opened, closing };

        void set_overlay_service(scene::OverlayHost* host) noexcept;
        [[nodiscard]] auto resolve_overlay_host() -> std::shared_ptr<scene::OverlayHost>;
        /// 确定承载方式并挂载；已挂载时返回 true。
        [[nodiscard]] auto mount() -> bool;
        /// 释放浮层托管；树内承载只隐藏，不移除子树。
        void unmount();
        void apply_style();
        void start_fade(float target);
        void request_close(internal::DismissLayer& layer);
        [[nodiscard]] auto active() const noexcept -> bool {
            return phase_ != DialogPhase::closed;
        }

        std::shared_ptr<internal::DismissLayer> dismiss_layer_;
        std::shared_ptr<internal::FocusScope> focus_scope_;
        std::shared_ptr<internal::DialogPanel> panel_;
        std::weak_ptr<scene::OverlayHost> overlay_service_;
        std::unique_ptr<scene::OverlayHandle> portal_handle_;
        /// 承载方式在首次挂载时确定：有窗口浮层就托管，否则留在树内做模态回退。
        bool overlay_mode_ = false;
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
