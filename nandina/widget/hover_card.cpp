//
// widget/hover_card - hover-triggered floating card that can host arbitrary content.
//
// 本文件定义内部类型 `internal::HoverCardSurface`：卡片外壳 + 内容槽位 + 指针进出观察。
// 它**只创建一次**（与 Popover 的 surface 同款），开合时只是把它托管 / 撤出浮层，因此
// 用户提供的内容节点始终是同一个孩子，不会在每次开合时被销毁重建。
//

#include "hover_card.hpp"

#include "primitives/box_painter.hpp"
#include "semantics/semantics.hpp"

#include "../render/draw_context.hpp"
#include "../scene/input_event.hpp"
#include "../scene/overlay_host.hpp"
#include "../scene/scene_tree.hpp"
#include "../theme/theme_manager.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace nandina::widget
{
    namespace internal
    {
        /**
         * 卡片表面：面板外壳 + 内容槽位 + 指针进出的上报点。
         *
         * 为什么指针观察落在这里：卡片被托管到浮层后与 HoverCard 节点分处两棵子树，
         * HoverCard 收不到卡片侧的 enter / leave（`_transition_hover` 只在共同祖先
         * **以下**冒泡）。表面是卡片内所有控件的祖先，因此卡片内部的进出都会冒泡到它，
         * 而它内部两个子控件之间移动不会产生多余的 leave（共同祖先正是它自己）。
         *
         * 表面自身不可聚焦：悬停卡片不抢焦点，但内容里的控件仍然照常可聚焦。
         */
        class HoverCardSurface final: public scene::NanControl {
        public:
            /// 指针进入 / 离开卡片。由表面观察，HoverCard 据此推进状态机。
            std::function<void(bool)> on_pointer_inside;

            void set_content(std::shared_ptr<scene::NanControl> content) {
                if (!content) {
                    throw std::invalid_argument("HoverCard::set_content: content is null");
                }
                if (auto* current = content_; current != nullptr) {
                    remove_and_delete(*current);
                }
                content_ = content.get();
                add_child(std::move(content));
                mark_layout_dirty();
                mark_semantics_dirty();
            }

            [[nodiscard]] auto content() const -> scene::NanControl* {
                return content_;
            }

            void set_style(theme::ResolvedHoverCardStyle style) {
                style_ = std::move(style);
                mark_layout_dirty();
                mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
            }

            [[nodiscard]] auto is_focusable() const -> bool override {
                return false;
            }

            [[nodiscard]] auto semantics_properties() const -> semantics::Properties override {
                // 悬停卡片在 ARIA 里没有专用 role；它承载的是可交互内容（不是 tooltip），
                // 所以这里只给一个 non-role 容器 + label，让内容里的控件各自暴露语义。
                return {
                    .role = semantics::Role::generic,
                    .label = std::string {"悬停卡片"},
                };
            }

        protected:
            [[nodiscard]] auto on_measure(const scene::LayoutConstraints constraints)
                -> foundation::NanSize override {
                const auto& m = style_.metrics;
                const float inner_max_width = std::max(
                    0.0F,
                    std::min(m.max_width, constraints.max_width) - m.padding_x * 2.0F
                );
                if (content_ == nullptr) {
                    return constraints.constrain(foundation::NanSize(0.0F, 0.0F));
                }
                const auto measured = content_->measure_layout(scene::LayoutConstraints {
                    .min_width = 0.0F,
                    .max_width = inner_max_width,
                    .min_height = 0.0F,
                    .max_height = constraints.max_height,
                });
                const float width = std::max(
                    measured.get_width() + m.padding_x * 2.0F,
                    std::min(m.min_width, constraints.max_width)
                );
                return constraints.constrain(
                    foundation::NanSize(width, measured.get_height() + m.padding_y * 2.0F)
                );
            }

            void on_layout() override {
                if (content_ == nullptr) {
                    return;
                }
                const auto& m = style_.metrics;
                const auto inner = foundation::NanRect::from_xywh(
                    m.padding_x,
                    m.padding_y,
                    std::max(0.0F, width() - m.padding_x * 2.0F),
                    std::max(0.0F, height() - m.padding_y * 2.0F)
                );
                content_->measure_layout(
                    scene::LayoutConstraints::tight(inner.get_size())
                );
                content_->layout_to(inner);
            }

            auto on_draw(render::DrawContext& context) -> void override {
                const auto world =
                    render::world_bounds_from_local(context.world_transform(), local_rect());
                primitives::BoxPainter::paint(context, world, style_.panel, context.opacity());
            }

            auto on_input(scene::InputEvent& event) -> bool override {
                // 只观察，不消费：卡片里的按钮 / 链接仍要收到自己的事件。
                if (event.type() == scene::EventType::mouse_enter) {
                    if (on_pointer_inside) {
                        on_pointer_inside(true);
                    }
                    return false;
                }
                if (event.type() == scene::EventType::mouse_leave) {
                    if (on_pointer_inside) {
                        on_pointer_inside(false);
                    }
                    return false;
                }
                return false;
            }

        private:
            scene::NanControl* content_ = nullptr;
            theme::ResolvedHoverCardStyle style_;
        };
    } // namespace internal

    HoverCard::HoverCard(
        std::shared_ptr<scene::NanControl> trigger,
        std::shared_ptr<scene::NanControl> content,
        theme::NanTheme theme
    ) {
        system_ =
            std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;

        surface_ = std::make_shared<internal::HoverCardSurface>();
        surface_->on_pointer_inside = [this](const bool inside) {
            set_pointer_in_content(inside);
        };
        surface_->set_style(resolved_style());

        if (trigger) {
            (void)set_trigger(std::move(trigger));
        }
        if (content) {
            (void)set_content(std::move(content));
        }
        set_visible(true);
    }

    HoverCard::~HoverCard() = default;

    auto HoverCard::create(
        std::shared_ptr<scene::NanControl> trigger,
        std::shared_ptr<scene::NanControl> content,
        theme::NanTheme theme
    ) -> std::shared_ptr<HoverCard> {
        return std::make_shared<HoverCard>(std::move(trigger), std::move(content), theme);
    }

    // ─── 槽位 ────────────────────────────────────────────────────────────

    auto HoverCard::set_trigger(std::shared_ptr<scene::NanControl> trigger) -> HoverCard& {
        if (!trigger) {
            throw std::invalid_argument("HoverCard::set_trigger: trigger is null");
        }
        auto current = trigger_.lock();
        trigger_ = trigger;
        replace_child(current.get(), std::move(trigger));
        sync_portal();
        mark_layout_dirty();
        return *this;
    }

    auto HoverCard::trigger() const -> std::shared_ptr<scene::NanControl> {
        return trigger_.lock();
    }

    auto HoverCard::set_content(std::shared_ptr<scene::NanControl> content) -> HoverCard& {
        content_ = content;
        surface_->set_content(std::move(content));
        if (open_) {
            sync_portal();
        }
        return *this;
    }

    auto HoverCard::content() const -> scene::NanControl* {
        return surface_->content();
    }

    // ─── 开合时机 ────────────────────────────────────────────────────────

    void HoverCard::set_open_delay(const float seconds) {
        if (!std::isfinite(seconds) || seconds < 0.0F) {
            throw std::invalid_argument("hover card open delay must be finite and non-negative");
        }
        open_delay_ = seconds;
    }

    auto HoverCard::open_delay() const -> float {
        return open_delay_;
    }

    void HoverCard::set_close_delay(const float seconds) {
        if (!std::isfinite(seconds) || seconds < 0.0F) {
            throw std::invalid_argument("hover card close delay must be finite and non-negative");
        }
        close_delay_ = seconds;
    }

    auto HoverCard::close_delay() const -> float {
        return close_delay_;
    }

    void HoverCard::set_hoverable_content(const bool hoverable) {
        hoverable_content_ = hoverable;
        if (!hoverable && pointer_in_content_) {
            // 关掉"可移入"后，指针留在卡片里不再算作停留依据。
            pointer_in_content_ = false;
        }
    }

    auto HoverCard::hoverable_content() const -> bool {
        return hoverable_content_;
    }

    // ─── 定位 ────────────────────────────────────────────────────────────

    void HoverCard::set_placement(const internal::OverlayPlacement placement) {
        placement_ = placement;
        if (open_) {
            sync_portal();
        }
    }

    auto HoverCard::placement() const -> internal::OverlayPlacement {
        return placement_;
    }

    void HoverCard::set_alignment(const internal::OverlayAlignment alignment) {
        alignment_ = alignment;
        if (open_) {
            sync_portal();
        }
    }

    auto HoverCard::alignment() const -> internal::OverlayAlignment {
        return alignment_;
    }

    void HoverCard::set_gap(const float gap) {
        gap_override_ = std::max(0.0F, gap);
        if (open_) {
            sync_portal();
        }
    }

    auto HoverCard::gap() const -> float {
        return gap_override_ >= 0.0F ? gap_override_ : resolved_style().metrics.gap;
    }

    void HoverCard::set_viewport_padding(const float padding) {
        viewport_padding_ = std::max(0.0F, padding);
        if (open_) {
            sync_portal();
        }
    }

    auto HoverCard::viewport_padding() const -> float {
        return viewport_padding_;
    }

    // ─── 状态与事件 ──────────────────────────────────────────────────────

    void HoverCard::open() {
        if (open_) {
            return;
        }
        open_ = true;
        close_timer_ = 0.0F;
        sync_portal();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
        if (on_open_) {
            on_open_();
        }
    }

    void HoverCard::close() {
        if (!open_) {
            return;
        }
        open_ = false;
        close_portal();
        open_timer_ = 0.0F;
        close_timer_ = 0.0F;
        pointer_in_content_ = false;
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
        if (on_close_) {
            on_close_();
        }
    }

    void HoverCard::toggle() {
        open_ ? close() : open();
    }

    auto HoverCard::is_open() const -> bool {
        return open_;
    }

    void HoverCard::set_on_open(std::function<void()> callback) {
        on_open_ = std::move(callback);
    }

    void HoverCard::set_on_close(std::function<void()> callback) {
        on_close_ = std::move(callback);
    }

    // ─── 主题 ────────────────────────────────────────────────────────────

    void HoverCard::set_theme(theme::NanTheme theme) {
        theme_view_ = theme;
        system_ =
            std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        apply_style();
        mark_layout_dirty();
    }

    auto HoverCard::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void HoverCard::set_override(theme::HoverCardRecipeRule rule) {
        override_ = std::move(rule);
        apply_style();
        mark_layout_dirty();
    }

    auto HoverCard::resolved_style() const -> theme::ResolvedHoverCardStyle {
        auto style = theme::resolve_hover_card(*system_, appearance_);
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void HoverCard::on_style_context_changed(const theme::ResolvedStyleContext& /*context*/) {
        apply_style();
        mark_layout_dirty();
    }

    void HoverCard::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_style();
        mark_layout_dirty();
    }

    // ─── 生命周期与输入 ──────────────────────────────────────────────────

    auto HoverCard::z_index_hint() const -> int {
        return open_ ? 1 : 0;
    }

    auto HoverCard::on_input(scene::InputEvent& event) -> bool {
        // 只观察悬停，不消费输入 —— 触发控件仍需接收点击 / 键盘。
        if (event.type() == scene::EventType::mouse_enter) {
            pointer_on_trigger_ = true;
            close_timer_ = 0.0F;
            return false;
        }
        if (event.type() == scene::EventType::mouse_leave) {
            pointer_on_trigger_ = false;
            open_timer_ = 0.0F;
            return false;
        }
        return false;
    }

    void HoverCard::on_process(const float dt) {
        if (open_) {
            // 锚点可能因为页面滚动 / 布局变化而移动，这里跟着重新定位。
            sync_portal();
        }
        advance_timers(dt);
    }

    void HoverCard::on_exit_tree() {
        // 卡片托管在浮层里而不是本子树之下，离开场景树必须显式收回。
        open_ = false;
        pointer_on_trigger_ = false;
        pointer_in_content_ = false;
        open_timer_ = 0.0F;
        close_timer_ = 0.0F;
        close_portal();
    }

    auto HoverCard::on_measure(const scene::LayoutConstraints constraints) -> foundation::NanSize {
        auto trigger = trigger_.lock();
        if (trigger == nullptr) {
            return constraints.constrain(foundation::NanSize(0.0F, 0.0F));
        }
        return constraints.constrain(trigger->measure_layout(constraints));
    }

    void HoverCard::on_layout() {
        auto trigger = trigger_.lock();
        if (trigger == nullptr) {
            return;
        }
        // 触发控件铺满本节点的矩形：HoverCard 是它在布局里的占位。
        trigger->layout_to(local_rect());
    }

    auto HoverCard::is_focusable() const -> bool {
        // 悬停卡片不参与 Tab 序列：焦点仍归触发控件与卡片内容里的控件。
        return false;
    }

    auto HoverCard::semantics_properties() const -> semantics::Properties {
        // 本节点只是触发控件的占位外壳，语义由触发控件与卡片内容各自暴露。
        return {.role = semantics::Role::none};
    }

    // ─── 内部 ────────────────────────────────────────────────────────────

    void HoverCard::set_overlay_service(scene::OverlayHost* host) noexcept {
        overlay_service_ =
            host != nullptr ? host->weak_self() : std::weak_ptr<scene::OverlayHost> {};
    }

    auto HoverCard::resolve_overlay_host() -> std::shared_ptr<scene::OverlayHost> {
        if (auto injected = overlay_service_.lock()) {
            return injected;
        }
        for (auto* node = parent(); node != nullptr; node = node->parent()) {
            auto* node_2d = node->as_node2d();
            auto* stack = node_2d != nullptr ? node_2d->as_layer_stack() : nullptr;
            if (stack == nullptr) {
                continue;
            }
            if (auto* host = stack->as_overlay_host(); host != nullptr) {
                return host->weak_self().lock();
            }
        }
        return nullptr;
    }

    void HoverCard::apply_style() {
        surface_->set_style(resolved_style());
    }

    void HoverCard::set_pointer_in_content(const bool inside) {
        if (pointer_in_content_ == inside) {
            return;
        }
        pointer_in_content_ = inside;
        if (inside) {
            // 指针从触发器移进了卡片：取消关闭倒计时（这就是"可以移进去"）。
            close_timer_ = 0.0F;
        }
    }

    void HoverCard::advance_timers(const float dt) {
        const float step = std::max(dt, 0.0F);
        if (!open_) {
            if (!pointer_on_trigger_) {
                open_timer_ = 0.0F;
                return;
            }
            open_timer_ += step;
            if (open_delay_ <= 0.0F || open_timer_ >= open_delay_) {
                open();
            }
            return;
        }

        // 已经打开：指针留在触发器上、或（允许移入时）留在卡片里，都算"仍然停留"。
        // 触发器与卡片之间的那一段空隙因此由 close_delay 兜住，不会闪一下。
        if (pointer_on_trigger_ || (hoverable_content_ && pointer_in_content_)) {
            close_timer_ = 0.0F;
            return;
        }
        close_timer_ += step;
        if (close_delay_ <= 0.0F || close_timer_ >= close_delay_) {
            close();
        }
    }

    auto HoverCard::placement_options() const -> internal::AnchoredPositionOptions {
        const auto style = resolved_style();
        return {
            .placement = placement_,
            .alignment = alignment_,
            .gap = gap_override_ >= 0.0F ? gap_override_ : style.metrics.gap,
            .offset = foundation::NanPoint::zero(),
            .viewport_padding = viewport_padding_,
            .flip = true,
            .shift = true,
        };
    }

    void HoverCard::sync_portal() {
        if (!open_) {
            close_portal();
            return;
        }
        auto host = resolve_overlay_host();
        auto trigger = trigger_.lock();
        if (host == nullptr || trigger == nullptr || surface_->content() == nullptr) {
            // 没有窗口浮层服务时不显示（与 Tooltip 同一取舍）：悬停卡片是纯浮层组件，
            // 树内没有可靠的"盖在触发控件旁边且不被裁剪"的位置。
            close_portal();
            return;
        }
        const auto viewport_size = host->viewport_size();
        const auto anchor = trigger->global_bounds();
        if (!viewport_size.is_valid() || !anchor.is_valid()) {
            return;
        }

        const auto viewport = foundation::NanRect::from_origin_size(
            foundation::NanPoint::zero(),
            viewport_size
        );
        const auto options = placement_options();
        const auto size = surface_->measure_layout(scene::LayoutConstraints::loose());
        if (!size.is_valid()) {
            return;
        }
        const auto position = internal::position_anchored_overlay(anchor, size, viewport, options);
        surface_->set_position(position.rect.get_top_left());
        surface_->measure_layout(scene::LayoutConstraints::tight(size));
        surface_->layout_to(foundation::NanRect::from_origin_size(position.rect.get_top_left(), size));

        if (portal_handle_ != nullptr && portal_handle_->mounted() && portal_anchor_ == anchor
            && portal_viewport_ == viewport_size && portal_placement_ == position.placement)
        {
            return;
        }

        // 已经在浮层里就不重新 present：撤掉再挂会让内容节点经历一次无谓的父子切换。
        const auto parent_id = host->overlay_containing(*this);
        const auto level = parent_id != 0 ? scene::OverlayLevel::nested_popup
                                         : scene::OverlayLevel::popup;
        if (portal_handle_ == nullptr || !portal_handle_->mounted()) {
            portal_handle_ = std::make_unique<scene::OverlayHandle>(
                host->present(surface_, {.level = level, .parent = parent_id})
            );
        }
        portal_anchor_ = anchor;
        portal_viewport_ = viewport_size;
        portal_placement_ = position.placement;
    }

    void HoverCard::close_portal() {
        if (portal_handle_ != nullptr) {
            portal_handle_->close();
            portal_handle_.reset();
        }
        portal_anchor_ = foundation::NanRect {};
        portal_viewport_ = foundation::NanSize {};
        portal_placement_ = internal::OverlayPlacement::bottom;
    }
} // namespace nandina::widget
