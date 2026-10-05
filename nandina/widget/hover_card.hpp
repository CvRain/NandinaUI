//
// widget/hover_card - hover-triggered floating card that can host arbitrary content.
//
// 与 Tooltip 的分工：Tooltip 是"一段只读提示，指针离不开触发器"；HoverCard 承载的是
// **可以交互的内容**（用户预览、链接预览、统计摘要），因此指针要能从触发器移进卡片里
// 而不关闭。两者都用同一套悬停触发 + 延迟 + 锚定停靠的机制，都不抢焦点。
//
// 为什么不用 Popover：`Popover` 服务于"显式打开"的锚定浮层 —— 它会给内容加 FocusScope
// （悬停卡片不能抢焦点），也会把触发控件上的 Space / Enter 当成开合开关（悬停卡片不该
// 响应）。这些对 Popover 是正确的语义，对悬停卡片不是。所以本组件自己管理 portal，
// 与 Tooltip 走同一条路。
//
// 开关状态机（时间全部走 `on_process`，不依赖帧率）：
//
//     指针进入触发器   → 取消关闭计时；未打开则累计 open_delay 后打开
//     指针离开触发器   → 若指针不在卡片内，累计 close_delay 后关闭
//     指针进入卡片     → 取消关闭计时（这就是"可以移进去"）
//     指针离开卡片     → 累计 close_delay 后关闭
//
// 触发器与卡片分处两棵子树（卡片在浮层里），所以事件来源也是两个：触发器侧由本节点
// 的 `on_input` 观察冒泡上来的 enter / leave，卡片侧由 `internal::HoverCardSurface`
// 观察并回调进来。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_HOVER_CARD_HPP
#define NANDINA_EXPERIMENT_WIDGET_HOVER_CARD_HPP

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
    class InputEvent;
} // namespace nandina::scene

namespace nandina::widget
{
    template<typename Component>
    struct ComponentTraits;

    namespace internal
    {
        class HoverCardSurface;
    } // namespace internal

    /**
     * 悬停卡片：指针在触发控件上停留一段时间后，在其旁边展开一张可交互的卡片。
     *
     * 内容槽位是任意控件（`set_content`），组件只负责外壳、定位与开合时机；卡片不抢
     * 焦点、不加遮罩、不阻断下层输入。
     *
     * 没有窗口浮层服务时（detached 上下文）**不显示卡片** —— 与 Tooltip 同一取舍：
     * 树内没有既能锚在触发控件旁、又不会被 ScrollView / Card 裁剪的位置，硬放会给出
     * 一个比"不显示"更糟的结果（错位且被裁掉一半）。
     */
    class HoverCard: public scene::NanControl {
    public:
        explicit HoverCard(
            std::shared_ptr<scene::NanControl> trigger = nullptr,
            std::shared_ptr<scene::NanControl> content = nullptr,
            theme::NanTheme theme = theme::default_theme()
        );
        ~HoverCard() override;

        [[nodiscard]] static auto create(
            std::shared_ptr<scene::NanControl> trigger = nullptr,
            std::shared_ptr<scene::NanControl> content = nullptr,
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<HoverCard>;

        /// 替换触发控件（单子；空指针拒绝）。
        auto set_trigger(std::shared_ptr<scene::NanControl> trigger) -> HoverCard&;
        [[nodiscard]] auto trigger() const -> std::shared_ptr<scene::NanControl>;

        /// 内容槽位：内容不挂在本节点之下，而是随浮层托管（detached 时回退树内）。
        auto set_content(std::shared_ptr<scene::NanControl> content) -> HoverCard&;
        [[nodiscard]] auto content() const -> scene::NanControl*;

        // ─── 开合时机 ────────────────────────────────────────────────────

        /// 指针停留多久后展开（秒，≥0，默认 0.3）。0 表示立即展开。
        void set_open_delay(float seconds);
        [[nodiscard]] auto open_delay() const -> float;
        /// 指针离开后多久收起（秒，≥0，默认 0.2）。这段延迟就是"移进卡片"的窗口。
        void set_close_delay(float seconds);
        [[nodiscard]] auto close_delay() const -> float;
        /// 指针移入卡片时是否保持展开（默认 true）。false 时行为退化为 Tooltip。
        void set_hoverable_content(bool hoverable);
        [[nodiscard]] auto hoverable_content() const -> bool;

        // ─── 定位 ────────────────────────────────────────────────────────

        void set_placement(internal::OverlayPlacement placement);
        [[nodiscard]] auto placement() const -> internal::OverlayPlacement;
        void set_alignment(internal::OverlayAlignment alignment);
        [[nodiscard]] auto alignment() const -> internal::OverlayAlignment;
        /// 卡片与触发控件之间的间距。
        void set_gap(float gap);
        [[nodiscard]] auto gap() const -> float;
        /// 卡片与窗口边缘至少保留的距离（非负）。
        void set_viewport_padding(float padding);
        [[nodiscard]] auto viewport_padding() const -> float;

        // ─── 状态与事件 ──────────────────────────────────────────────────

        void open();
        void close();
        void toggle();
        [[nodiscard]] auto is_open() const -> bool;

        void set_on_open(std::function<void()> callback);
        void set_on_close(std::function<void()> callback);

        // ─── 主题 ────────────────────────────────────────────────────────

        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        void set_override(theme::HoverCardRecipeRule rule);
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedHoverCardStyle;

        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;
        /// 树内回退时提升 z 序，使卡片压过后续兄弟；浮层承载时由层级顺序决定。
        [[nodiscard]] auto z_index_hint() const -> int override;
        /// 只观察悬停，不消费输入 —— 触发控件仍要接收点击与键盘。
        auto on_input(scene::InputEvent& event) -> bool override;
        void on_process(float dt) override;
        void on_exit_tree() override;

    protected:
        [[nodiscard]] auto on_measure(foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;
        [[nodiscard]] auto is_focusable() const -> bool override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        friend struct ComponentTraits<HoverCard>;
        friend class internal::HoverCardSurface;

        /// Internal: bind the owning window's overlay portal. Called by
        /// `ComponentTraits<HoverCard>` so page authors never create an OverlayHost.
        void set_overlay_service(scene::OverlayHost* host) noexcept;
        [[nodiscard]] auto resolve_overlay_host() -> std::shared_ptr<scene::OverlayHost>;

        /// 把解析后的配方转成卡片表面的样式。
        void apply_style();
        /// 建立 / 更新浮层托管；锚点或视口变化时重新定位。
        void sync_portal();
        void close_portal();
        [[nodiscard]] auto placement_options() const -> internal::AnchoredPositionOptions;

        /// 卡片表面的指针进出回调（见文件头的状态机）。
        void set_pointer_in_content(bool inside);
        /// 定时器推进与到点动作。
        void advance_timers(float dt);

        std::weak_ptr<scene::NanControl> trigger_;
        std::shared_ptr<scene::NanControl> content_;
        std::shared_ptr<internal::HoverCardSurface> surface_;

        std::unique_ptr<scene::OverlayHandle> portal_handle_;
        foundation::NanRect portal_anchor_ {};
        foundation::NanSize portal_viewport_ {};
        internal::OverlayPlacement portal_placement_ = internal::OverlayPlacement::bottom;

        std::weak_ptr<scene::OverlayHost> overlay_service_;

        bool open_ = false;
        bool pointer_on_trigger_ = false;
        bool pointer_in_content_ = false;
        float open_timer_ = 0.0F;
        float close_timer_ = 0.0F;

        float open_delay_ = 0.3F;
        float close_delay_ = 0.2F;
        bool hoverable_content_ = true;

        internal::OverlayPlacement placement_ = internal::OverlayPlacement::bottom;
        internal::OverlayAlignment alignment_ = internal::OverlayAlignment::center;
        /// < 0 表示跟随配方的 `metrics.gap`。
        float gap_override_ = -1.0F;
        float viewport_padding_ = 0.0F;

        std::function<void()> on_open_;
        std::function<void()> on_close_;

        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        theme::NanTheme theme_view_;
        std::optional<theme::HoverCardRecipeRule> override_;
        bool system_explicit_ = false;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_HOVER_CARD_HPP
