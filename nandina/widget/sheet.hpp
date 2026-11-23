//
// widget/sheet - edge-docked modal overlay drawer (scrim + docked panel + focus trap).
//

#ifndef NANDINA_EXPERIMENT_WIDGET_SHEET_HPP
#define NANDINA_EXPERIMENT_WIDGET_SHEET_HPP

#include "../reactive/event.hpp"
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
} // namespace nandina::scene

namespace nandina::widget
{
    template<typename Component>
    struct ComponentTraits;

    namespace internal
    {
        class DismissLayer;
        class FocusScope;
        class SheetPanel;
    } // namespace internal

    /// 抽屉面板停靠的视口边缘（默认 right）。
    enum class SheetSide {
        top,
        right,
        bottom,
        left,
    };

    /**
     * 贴边模态抽屉（Sheet / Drawer）：遮罩、视口四边停靠面板与焦点限制。
     *
     * 打开时将面板托管到窗口级浮层（OverlayLevel::modal + block_below = true）：
     *   - 遮罩、外部点击与 Escape 关闭由 DismissLayer 提供；
     *   - 初始焦点、Tab 循环与关闭后焦点恢复由 FocusScope 提供；
     *   - 左右停靠（left / right）铺满视口高度，footer 贴底、content 占据中间剩余高度；
     *     上下停靠（top / bottom）铺满视口宽度，主轴厚度支持 set_extent(float) 覆盖；
     *   - 复用 DialogRecipe / DialogRecipeRule（贴边默认直角 radius = 0，可由 set_override 覆盖）。
     * 无窗口浮层服务时（detached 上下文）回退为树内模态。
     */
    class Sheet final: public scene::NanControl {
    public:
        explicit Sheet(
            SheetSide side = SheetSide::right,
            theme::NanTheme theme = theme::default_theme()
        );
        explicit Sheet(
            std::string title,
            SheetSide side = SheetSide::right,
            theme::NanTheme theme = theme::default_theme()
        );
        ~Sheet() override;

        [[nodiscard]] static auto
        create(SheetSide side = SheetSide::right, theme::NanTheme theme = theme::default_theme())
            -> std::shared_ptr<Sheet>;
        [[nodiscard]] static auto create(
            std::string title,
            SheetSide side = SheetSide::right,
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<Sheet>;

        // ─── 停靠方向与主轴厚度 ──────────────────────────────────────────

        void set_side(SheetSide side);
        [[nodiscard]] auto side() const noexcept -> SheetSide;

        /// 显式设置主轴厚度（left/right 为面板宽度，top/bottom 为面板高度）；必须为有限正数。
        void set_extent(float extent);
        void clear_extent() noexcept;
        [[nodiscard]] auto has_explicit_extent() const noexcept -> bool;
        /// 显式 extent，或未设置时 DialogRecipe 的 panel_width 配置回退值。
        /// top/bottom 清除显式值后的实际自然高度由布局结果决定，不由此查询返回。
        [[nodiscard]] auto extent() const -> float;

        // ─── 标题 / 说明 / 命名槽位 ──────────────────────────────────────

        /// header 槽位的文本便捷入口；与 set_header() 互斥，后设置者生效。
        void set_title(std::string title);
        [[nodiscard]] auto title() const noexcept -> std::string_view;
        void set_description(std::string description);
        [[nodiscard]] auto description() const noexcept -> std::string_view;

        /// 固定语义槽位：header / content / footer，缺省为空且不占高度。
        auto set_header(std::shared_ptr<scene::NanControl> header) -> Sheet&;
        auto set_content(std::shared_ptr<scene::NanControl> content) -> Sheet&;
        auto set_footer(std::shared_ptr<scene::NanControl> footer) -> Sheet&;

        [[nodiscard]] auto header() const -> scene::NanControl*;
        [[nodiscard]] auto content() const -> scene::NanControl*;
        [[nodiscard]] auto footer() const -> scene::NanControl*;

        // ─── 打开 / 关闭 / 事件 ──────────────────────────────────────────

        void open();
        void close();
        void set_open(bool open);
        [[nodiscard]] auto is_open() const noexcept -> bool;

        /// false 时禁用 Escape / 点击遮罩关闭（需程序化 close）。
        void set_dismissible(bool dismissible) noexcept;
        [[nodiscard]] auto dismissible() const noexcept -> bool;

        void set_on_close(std::function<void()> callback);
        [[nodiscard]] auto closed() const noexcept -> const reactive::Event<>&;

        // ─── 主题与配方覆盖（复用 DialogRecipe / DialogRecipeRule）──────

        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        void set_override(theme::DialogRecipeRule rule);
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedDialogStyle;

        void set_text_pipeline(text::TextPipeline pipeline);
        void apply_default_text_pipeline(const text::TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;

        /// 树内回退时提升 z 序，使模态面板压过后续兄弟；浮层承载时由 OverlayLevel 决定。
        [[nodiscard]] auto z_index_hint() const -> int override;
        void on_process(float dt) override;
        void on_exit_tree() override;

    protected:
        [[nodiscard]] auto on_measure(foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;

    private:
        friend struct ComponentTraits<Sheet>;

        enum class SheetPhase { closed, opening, opened, closing };
        enum class MountMode { unmounted, tree, overlay };

        void set_overlay_service(scene::OverlayHost* host) noexcept;
        [[nodiscard]] auto resolve_overlay_host() -> std::shared_ptr<scene::OverlayHost>;
        [[nodiscard]] auto mount() -> bool;
        void unmount();
        void install_dismiss_callback();
        void install_origin_resolver();
        void apply_style();
        void start_fade(float target);
        void restore_previous_focus();
        [[nodiscard]] auto active() const noexcept -> bool {
            return phase_ != SheetPhase::closed;
        }

        std::shared_ptr<internal::DismissLayer> dismiss_layer_;
        std::shared_ptr<internal::FocusScope> focus_scope_;
        std::shared_ptr<internal::SheetPanel> panel_;
        std::weak_ptr<scene::OverlayHost> overlay_service_;
        std::unique_ptr<scene::OverlayHandle> portal_handle_;
        MountMode mount_mode_ = MountMode::unmounted;
        SheetPhase phase_ = SheetPhase::closed;
        SheetSide side_ = SheetSide::right;
        std::optional<float> extent_;
        bool dismissible_ = true;
        std::weak_ptr<scene::NanNode> previous_focus_;
        reactive::Event<> closed_;
        reactive::Subscription user_close_sub_;
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        theme::NanTheme theme_view_;
        std::optional<theme::DialogRecipeRule> override_;
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_SHEET_HPP
