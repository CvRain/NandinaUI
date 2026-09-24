//
// widget/popover - non-modal anchored floating container (trigger + content).
//

#ifndef NANDINA_EXPERIMENT_WIDGET_POPOVER_HPP
#define NANDINA_EXPERIMENT_WIDGET_POPOVER_HPP

#include "../scene/control.hpp"
#include "../theme/design_system.hpp"
#include "internal/anchored_positioner.hpp"

#include <functional>
#include <memory>
#include <optional>

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
        class PopoverSurface;
    } // namespace internal

    /**
     * 非模态锚定浮层：触发控件留在原布局里，任意内容挂到窗口浮层并锚在触发控件旁边。
     *
     * 打开时把内容托管给 OverlayHost（层级由 popup / nested_popup 决定，`block_below`
     * 始终为 false —— 它不阻断下层输入），定位复用 `internal::position_anchored_overlay`
     * 的四向 placement、三种 alignment、gap、flip 与 viewport shift。外部点击与 Escape
     * 由 DismissLayer 提供，初始焦点、Tab 循环与关闭后的焦点恢复由 FocusScope 提供；
     * 没有窗口浮层服务时（detached 上下文）回退为树内锚定展示，公开 API 与行为不变。
     */
    class Popover: public scene::NanControl {
    public:
        explicit Popover(
            std::shared_ptr<scene::NanControl> trigger = nullptr,
            std::shared_ptr<scene::NanControl> content = nullptr,
            theme::NanTheme theme = theme::default_theme()
        );
        ~Popover() override;

        [[nodiscard]] static auto create(
            std::shared_ptr<scene::NanControl> trigger = nullptr,
            std::shared_ptr<scene::NanControl> content = nullptr,
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<Popover>;

        /// 替换触发控件（单子，与 Tooltip 同款机制；空指针拒绝）。
        auto set_trigger(std::shared_ptr<scene::NanControl> trigger) -> Popover&;
        [[nodiscard]] auto trigger() const -> std::shared_ptr<scene::NanControl>;

        /// 内容槽位：内容不挂在 Popover 之下，而是随浮层托管（detached 时回退树内）。
        auto set_content(std::shared_ptr<scene::NanControl> content) -> Popover&;
        [[nodiscard]] auto content() const -> scene::NanControl*;

        void open();
        void close();
        void toggle();
        [[nodiscard]] auto is_open() const -> bool;

        void set_placement(internal::OverlayPlacement placement);
        [[nodiscard]] auto placement() const -> internal::OverlayPlacement;
        void set_alignment(internal::OverlayAlignment alignment);
        [[nodiscard]] auto alignment() const -> internal::OverlayAlignment;
        /// 浮层与触发控件之间的间距。
        void set_gap(float gap);
        [[nodiscard]] auto gap() const -> float;
        /// 浮层与窗口边缘至少保留的距离（非负）。
        void set_viewport_padding(float padding);

        /// false 时禁用外部点击与 Escape 关闭（需要程序化 close）。
        void set_dismissible(bool dismissible);
        [[nodiscard]] auto dismissible() const -> bool;

        void set_on_open(std::function<void()> callback);
        void set_on_close(std::function<void()> callback);

        /// 高级接口：以完整 NanTheme 覆盖控件主题（不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        void set_override(theme::PopoverRecipeRule rule);
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedPopoverStyle;

        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;

        /// 树内回退时提升 z 序，使浮层内容压过后续兄弟；浮层承载时由层级顺序决定。
        [[nodiscard]] auto z_index_hint() const -> int override;
        auto on_input_capture(scene::InputEvent& event) -> bool override;
        auto on_input(scene::InputEvent& event) -> bool override;
        void on_process(float dt) override;
        void on_exit_tree() override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        friend struct ComponentTraits<Popover>;
        /// DropdownMenu 组合一个 Popover 并把 BuildContext 注入的浮层服务转发给它；
        /// 服务注入点保持 private，因此这里需要友元（不改变任何行为）。
        friend class DropdownMenu;

        /// 内容的承载方式，在首次打开时确定后固定下来。
        enum class MountMode { unmounted, tree, overlay };

        /// Internal: bind the owning window's overlay portal. Called by
        /// `ComponentTraits<Popover>` so page authors never create an OverlayHost.
        void set_overlay_service(scene::OverlayHost* host) noexcept;
        /// Anchor to an already mounted control without adopting it as the trigger
        /// child. DropdownMenu uses this for submenu rows that remain in the parent
        /// menu surface.
        void set_external_anchor(const std::shared_ptr<scene::NanControl>& anchor) noexcept;
        /// Nested menus let pointer hits outside the child panel reach the parent
        /// menu; the root Popover keeps the default full-screen dismiss surface.
        void set_pointer_passthrough_outside(bool enabled) noexcept;
        /// Detached submenu popovers do not receive tree theme propagation. Copy the
        /// already resolved runtime source and immediately refresh the panel recipe.
        void inherit_runtime_style_from(const Popover& source);

        /// Nearest usable overlay portal: the injected window service, otherwise the
        /// closest ancestor OverlayHost (covers `Popover::create()` built popovers).
        [[nodiscard]] auto resolve_overlay_host() -> std::shared_ptr<scene::OverlayHost>;

        /// Create, reposition or drop the portal surface to match `open_`.
        void sync_portal();
        /// Build and present the floating stack for the current anchor/viewport.
        void present_portal(
            const std::shared_ptr<scene::OverlayHost>& host,
            const foundation::NanRect& anchor,
            const foundation::NanSize& viewport_size
        );
        /// Anchor the persistent surface inside the presented stack and hand the
        /// panel's screen rect to `dismiss` (null outside the portal path).
        void position_surface(
            const foundation::NanRect& anchor,
            const foundation::NanSize& viewport_size,
            const std::shared_ptr<internal::DismissLayer>& dismiss
        );
        /// The mounted portal's dismiss layer, recovered from the surface's ancestors.
        [[nodiscard]] auto overlay_dismiss_layer() const -> std::shared_ptr<internal::DismissLayer>;
        /// Rebuild the surface after content/theme changes while it is on screen.
        auto refresh_portal() -> void;
        auto close_portal() -> void;
        [[nodiscard]] auto position_options() const -> internal::AnchoredPositionOptions;

        /// 关闭收尾：兜底焦点、触发 on_close、标记脏位。`notify` 为 false 时用于
        /// 控件正在销毁 / 离开场景树的路径，那时既不能回焦点也不该跑用户回调。
        auto finish_close(bool notify) -> void;
        /// 树内回退时安装 DismissLayer / FocusScope 子树，并摆到锚点下方。
        void layout_tree_fallback();
        /// 同步拆掉树内回退子树（持久成员，必须立刻脱离，不能依赖延迟删除）。
        void detach_tree_fallback();
        [[nodiscard]] auto tree_viewport() const -> foundation::NanRect;

        std::weak_ptr<scene::NanControl> trigger_;
        std::weak_ptr<scene::NanControl> anchor_;
        /// 内容槽位的唯一持有者。随浮层托管时同时被 PopoverSurface 引用，关闭时收回，
        /// 因此内容不会在收起瞬间被销毁，可以再次打开。
        std::shared_ptr<scene::NanControl> content_;
        bool open_ = false;
        internal::OverlayPlacement placement_ = internal::OverlayPlacement::bottom;
        internal::OverlayAlignment alignment_ = internal::OverlayAlignment::start;
        float gap_ = 8.0F;
        float viewport_padding_ = 8.0F;
        bool dismissible_ = true;
        bool trigger_pressed_ = false;
        bool pointer_passthrough_outside_ = false;
        std::function<void()> on_open_;
        std::function<void()> on_close_;
        MountMode mount_mode_ = MountMode::unmounted;

        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        theme::NanTheme theme_view_;
        std::optional<theme::PopoverRecipeRule> override_;
        bool system_explicit_ = false;

        /// Window-owned portal injected by `ComponentTraits<Popover>`. A weak
        /// reference also makes detached/manual contexts safe when their host dies.
        std::weak_ptr<scene::OverlayHost> overlay_service_;
        std::unique_ptr<scene::OverlayHandle> portal_handle_;
        /// 面板本体。持久持有：浮层收起时只把 handle 放掉，内容和面板留待下次打开复用。
        /// 必须强持有 —— 面板是 Popover 的私有状态，不是浮层句柄的一部分。
        std::shared_ptr<internal::PopoverSurface> surface_;
        std::shared_ptr<internal::DismissLayer> dismiss_layer_;
        std::shared_ptr<internal::FocusScope> focus_scope_;
        /// Last anchor / viewport / placement the surface was positioned against.
        /// Repositioning is skipped while all three are unchanged, so an open
        /// popover neither re-measures its content nor re-runs the positioner every
        /// frame. The placement and alignment must be part of the key: the setters
        /// only change them.
        foundation::NanRect portal_anchor_ {};
        foundation::NanSize portal_viewport_ {};
        internal::OverlayPlacement portal_placement_ = internal::OverlayPlacement::bottom;
        internal::OverlayAlignment portal_alignment_ = internal::OverlayAlignment::start;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_POPOVER_HPP
