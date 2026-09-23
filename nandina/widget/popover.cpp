//
// widget/popover - non-modal anchored floating container (trigger + content).
//

#include "popover.hpp"

#include "../render/draw_context.hpp"
#include "../scene/input_event.hpp"
#include "../scene/overlay_host.hpp"
#include "../scene/scene_tree.hpp"
#include "../theme/theme_manager.hpp"
#include "internal/dismiss_layer.hpp"
#include "internal/focus_scope.hpp"
#include "key_codes.hpp"
#include "primitives/box_painter.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace nandina::widget
{
    namespace internal
    {
        /**
         * 浮层面板本体：绘制配方面板，承载唯一的 content 槽位，并把自己摆到锚点位置。
         *
         * 内容不直接挂在 Popover 之下，而是挂在浮层里，所以面板是唯一知道"内容边界"的
         * 节点：DismissLayer 用它判断点击是否落在内容内，Popover 用它测量浮层尺寸。
         *
         * 位置由 `anchor_origin` 记住而不是由父级布局决定：承载它的 FocusScope 会把子节点
         * 按自己的局部矩形摆放（`layout_to(local_rect())`），若面板依赖父级给的矩形，
         * 锚点位置会在每次布局时被覆盖回 (0,0)。
         */
        class PopoverSurface final: public scene::NanControl {
        public:
            auto set_content(std::shared_ptr<scene::NanControl> content) -> void {
                if (auto current = content_.lock()) {
                    remove_and_delete(*current);
                }
                content_ = content;
                if (content) {
                    add_child(std::move(content));
                }
                mark_layout_dirty();
            }

            [[nodiscard]] auto content() const -> scene::NanControl* {
                return content_.lock().get();
            }

            void set_style(theme::ResolvedPopoverStyle style) {
                style_ = std::move(style);
                mark_layout_dirty();
                mark_dirty(scene::DirtyFlags::paint);
            }

            /// 锚点在父容器局部坐标里的落点。
            void set_anchor_origin(const foundation::NanPoint origin) {
                if (anchor_origin_ == origin) {
                    return;
                }
                anchor_origin_ = origin;
                if (parent() != nullptr) {
                    set_position(origin);
                }
                mark_layout_dirty();
            }

            [[nodiscard]] auto anchor_origin() const -> const foundation::NanPoint& {
                return anchor_origin_;
            }

            /// 面板自身不参与命中：命中必须落到内容控件上，否则内容里的按钮会被
            /// 这层容器吞掉。DismissLayer 用 global_bounds() 做"内 / 外"判定即可。
            [[nodiscard]] auto contains_point(foundation::NanPoint) const -> bool override {
                return false;
            }

            [[nodiscard]] auto is_focusable() const -> bool override {
                return false;
            }

            auto on_draw(render::DrawContext& context) -> void override {
                const auto world =
                    render::world_bounds_from_local(context.world_transform(), local_rect());
                primitives::BoxPainter::paint(context, world, style_.panel, context.opacity());
            }

        protected:
            [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
                -> foundation::NanSize override {
                const auto inner = content_.lock();
                if (inner == nullptr) {
                    return constraints.constrain(
                        foundation::NanSize(0.0F, style_.metrics.min_height)
                    );
                }
                // 内容按面板内边距内收后再测量；最小高度保证空内容时面板仍可见。
                const auto measured = inner->measure_layout(
                    scene::LayoutConstraints {
                        .min_width = 0.0F,
                        .max_width =
                            std::max(0.0F, constraints.max_width - style_.metrics.padding_x * 2.0F),
                        .min_height = 0.0F,
                        .max_height = std::max(
                            0.0F,
                            constraints.max_height - style_.metrics.padding_y * 2.0F
                        ),
                    }
                );
                return constraints.constrain(
                    foundation::NanSize(
                        measured.get_width() + style_.metrics.padding_x * 2.0F,
                        std::max(
                            measured.get_height() + style_.metrics.padding_y * 2.0F,
                            style_.metrics.min_height
                        )
                    )
                );
            }

            auto on_layout() -> void override {
                // 父级把本节点摆到自己的局部原点；锚点位置是绝对的，必须在这里恢复。
                if (!(position() == anchor_origin_)) {
                    set_position(anchor_origin_);
                }
                auto inner = content_.lock();
                if (inner == nullptr) {
                    return;
                }
                const auto measured = inner->measure_layout(
                    scene::LayoutConstraints {
                        .min_width = 0.0F,
                        .max_width = std::max(0.0F, width() - style_.metrics.padding_x * 2.0F),
                        .min_height = 0.0F,
                        .max_height = std::max(0.0F, height() - style_.metrics.padding_y * 2.0F),
                    }
                );
                inner->layout_to(
                    foundation::NanRect::from_xywh(
                        style_.metrics.padding_x,
                        style_.metrics.padding_y,
                        measured.get_width(),
                        measured.get_height()
                    )
                );
            }

        private:
            std::weak_ptr<scene::NanControl> content_;
            theme::ResolvedPopoverStyle style_;
            foundation::NanPoint anchor_origin_ {};
        };
    } // namespace internal

    Popover::Popover(
        std::shared_ptr<scene::NanControl> trigger,
        std::shared_ptr<scene::NanControl> content,
        theme::NanTheme theme
    ):
        content_(std::move(content)) {
        system_ =
            std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;
        dismiss_layer_ = std::make_shared<internal::DismissLayer>();
        focus_scope_ = std::make_shared<internal::FocusScope>();
        // 内容不在构造期挂到面板上：收起状态下面板不持有任何子节点，槽位内容既不占
        // 布局也不报告为可见；打开时（portal 或树内回退）才挂上去。
        auto surface = std::make_shared<internal::PopoverSurface>();
        surface->set_style(resolved_style());
        surface_ = surface;
        if (trigger) {
            set_trigger(std::move(trigger));
        }
    }

    Popover::~Popover() = default;

    auto Popover::create(
        std::shared_ptr<scene::NanControl> trigger,
        std::shared_ptr<scene::NanControl> content,
        theme::NanTheme theme
    ) -> std::shared_ptr<Popover> {
        return std::make_shared<Popover>(std::move(trigger), std::move(content), theme);
    }

    auto Popover::set_trigger(std::shared_ptr<scene::NanControl> trigger) -> Popover& {
        if (!trigger) {
            throw std::runtime_error("Popover::set_trigger: trigger is null");
        }
        auto current = trigger_.lock();
        trigger_ = trigger;
        replace_child(current.get(), std::move(trigger));
        sync_portal();
        mark_layout_dirty();
        return *this;
    }

    auto Popover::trigger() const -> std::shared_ptr<scene::NanControl> {
        return trigger_.lock();
    }

    auto Popover::set_content(std::shared_ptr<scene::NanControl> content) -> Popover& {
        content_ = std::move(content);
        surface_->set_content(content_);
        refresh_portal();
        mark_layout_dirty();
        mark_semantics_dirty();
        return *this;
    }

    auto Popover::content() const -> scene::NanControl* {
        return content_.get();
    }

    void Popover::open() {
        // 幂等：已打开时什么都不做，也不重复触发 on_open。
        if (open_) {
            return;
        }
        if (mount_mode_ == MountMode::unmounted) {
            mount_mode_ = resolve_overlay_host() != nullptr ? MountMode::overlay : MountMode::tree;
        }
        open_ = true;
        if (mount_mode_ == MountMode::overlay) {
            sync_portal();
        }
        else {
            // 树内回退：内容挂在 Popover 自己的子树里，靠 z 序压过后续兄弟。
            set_visible(true);
            layout_tree_fallback();
        }
        // 浮层可能还没建起来（触发控件尚未布局，或注入的服务已失效）。此时 open_ 仍为
        // 真，on_process() 会继续重试；用户设置的状态不会被静默吞掉。
        if (on_open_) {
            on_open_();
        }
        mark_dirty(
            scene::DirtyFlags::paint | scene::DirtyFlags::layout | scene::DirtyFlags::semantics
        );
    }

    void Popover::close() {
        if (!open_) {
            return;
        }
        open_ = false;
        close_portal();
        detach_tree_fallback();
        finish_close(true);
    }

    void Popover::toggle() {
        if (open_) {
            close();
        }
        else {
            open();
        }
    }

    auto Popover::is_open() const -> bool {
        return open_;
    }

    void Popover::set_placement(const internal::OverlayPlacement placement) {
        placement_ = placement;
        sync_portal();
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Popover::placement() const -> internal::OverlayPlacement {
        return placement_;
    }

    void Popover::set_alignment(const internal::OverlayAlignment alignment) {
        alignment_ = alignment;
        sync_portal();
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Popover::alignment() const -> internal::OverlayAlignment {
        return alignment_;
    }

    void Popover::set_gap(const float gap) {
        if (!std::isfinite(gap) || gap < 0.0F) {
            throw std::invalid_argument("popover gap must be finite and non-negative");
        }
        gap_ = gap;
        sync_portal();
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Popover::gap() const -> float {
        return gap_;
    }

    void Popover::set_viewport_padding(const float padding) {
        if (!std::isfinite(padding) || padding < 0.0F) {
            throw std::invalid_argument("popover viewport padding must be finite and non-negative");
        }
        viewport_padding_ = padding;
        sync_portal();
        mark_dirty(scene::DirtyFlags::paint);
    }

    void Popover::set_dismissible(const bool dismissible) {
        dismissible_ = dismissible;
    }

    auto Popover::dismissible() const -> bool {
        return dismissible_;
    }

    void Popover::set_on_open(std::function<void()> callback) {
        on_open_ = std::move(callback);
    }

    void Popover::set_on_close(std::function<void()> callback) {
        on_close_ = std::move(callback);
    }

    void Popover::set_theme(theme::NanTheme theme) {
        system_ =
            std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        surface_->set_style(resolved_style());
        sync_portal();
        mark_layout_dirty();
    }

    auto Popover::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Popover::set_override(theme::PopoverRecipeRule rule) {
        override_ = std::move(rule);
        surface_->set_style(resolved_style());
        sync_portal();
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::layout);
    }

    auto Popover::resolved_style() const -> theme::ResolvedPopoverStyle {
        auto style = theme::resolve_popover(*system_, appearance_);
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Popover::on_style_context_changed(const theme::ResolvedStyleContext&) {
        sync_portal();
        mark_layout_dirty();
    }

    void Popover::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        surface_->set_style(resolved_style());
        sync_portal();
        mark_layout_dirty();
    }

    auto Popover::z_index_hint() const -> int {
        // 浮层承载时层级由 OverlayLevel 决定；树内回退要靠 z 序压过后续兄弟。
        return open_ && mount_mode_ != MountMode::overlay ? 1 : 0;
    }

    auto Popover::on_input_capture(scene::InputEvent& event) -> bool {
        auto trigger = trigger_.lock();
        if (trigger == nullptr) {
            trigger_pressed_ = false;
            return false;
        }

        const auto trigger_disabled = [&trigger] {
            return trigger->resolved_semantics_properties().state.disabled;
        };
        if (event.type() == scene::EventType::mouse_button) {
            auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
            if (mouse.button() != scene::MouseButtonEvent::Button::left) {
                return false;
            }
            if (mouse.is_pressed()) {
                trigger_pressed_ = !trigger_disabled()
                    && trigger->global_bounds().contains_point(mouse.screen_pos());
                return false;
            }

            const bool activate = trigger_pressed_ && !trigger_disabled()
                && trigger->global_bounds().contains_point(mouse.screen_pos());
            trigger_pressed_ = false;
            if (activate) {
                toggle();
            }
            return false;
        }

        if (event.type() == scene::EventType::key) {
            auto& key = static_cast<scene::KeyEvent&>(event);
            if (key.is_pressed() && !trigger_disabled()
                && (key.keycode() == keys::enter || key.keycode() == keys::space))
            {
                toggle();
            }
        }
        return false;
    }

    auto Popover::on_input(scene::InputEvent& event) -> bool {
        // 触发控件仍需接收自己的点击：这里只观察，不消费。
        (void)event;
        return false;
    }

    void Popover::on_process(const float /*dt*/) {
        if (open_ && mount_mode_ == MountMode::overlay) {
            // 触发控件滚动或重排时保持锚定。
            sync_portal();
        }
    }

    void Popover::on_exit_tree() {
        // 浮层内容不在本子树之下，卸载时必须显式释放，否则会留下指向已消失锚点的孤儿。
        // 这条路径上不回焦点、也不跑用户回调（场景树正在拆）。
        open_ = false;
        close_portal();
        // 面板是持久成员，Popover 走后它仍会活着：内容槽位必须显式清掉，否则旧内容会
        // 挂在一个已经不可达的面板上。
        if (surface_ != nullptr) {
            surface_->set_content(nullptr);
        }
        detach_tree_fallback();
        scene::NanControl::on_exit_tree();
    }

    void Popover::set_overlay_service(scene::OverlayHost* host) noexcept {
        overlay_service_ =
            host != nullptr ? host->weak_self() : std::weak_ptr<scene::OverlayHost> {};
    }

    auto Popover::resolve_overlay_host() -> std::shared_ptr<scene::OverlayHost> {
        if (auto injected = overlay_service_.lock()) {
            return injected;
        }
        // `Popover::create()` has no BuildContext, so fall back to the nearest
        // ancestor OverlayHost (the window installs one as the tree root). The
        // explicit `as_overlay_host()` accessor keeps this RTTI-free: a plain
        // LayerStack returns nullptr and the walk continues upward.
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

    void Popover::sync_portal() {
        if (!open_ || mount_mode_ != MountMode::overlay || content_ == nullptr) {
            return;
        }
        auto host = resolve_overlay_host();
        auto trigger = trigger_.lock();
        if (host == nullptr || trigger == nullptr) {
            return;
        }

        const auto viewport_size = host->viewport_size();
        const auto anchor = trigger->global_bounds();
        // Positioning needs a laid-out viewport and trigger. Either can be missing on
        // the first frame or immediately after set_trigger(); skip until they are
        // valid rather than feeding the positioner an invalid rect.
        if (!viewport_size.is_valid() || !anchor.is_valid()) {
            return;
        }
        // Rebuild only when the portal is gone or unmounted.
        if (portal_handle_ == nullptr || !portal_handle_->mounted()) {
            close_portal();
            present_portal(host, anchor, viewport_size);
            return;
        }

        // Nothing that affects the anchor moved: skip re-measuring the content and
        // re-running the positioner.
        if (anchor == portal_anchor_ && viewport_size == portal_viewport_
            && placement_ == portal_placement_ && alignment_ == portal_alignment_)
        {
            return;
        }
        position_surface(anchor, viewport_size, overlay_dismiss_layer());
    }

    void Popover::present_portal(
        const std::shared_ptr<scene::OverlayHost>& host,
        const foundation::NanRect& anchor,
        const foundation::NanSize& viewport_size
    ) {
        if (host == nullptr || content_ == nullptr) {
            return;
        }
        surface_->set_content(content_);
        surface_->set_style(resolved_style());

        // OverlayHost::present() takes ownership of an already detached control, so
        // the whole visible stack (dismiss surface → focus scope → panel) is built
        // before presenting. The panel is anchored inside that stack, not by the
        // overlay surface, because presenting lays children out at their own origin.
        auto scope = std::make_shared<internal::FocusScope>();
        scope->set_content(surface_);
        auto dismiss = std::make_shared<internal::DismissLayer>(
            [weak = std::weak_ptr<Popover>(std::static_pointer_cast<Popover>(shared_from_this()))](
                const internal::DismissReason
            ) {
                if (auto popover = weak.lock(); popover != nullptr && popover->dismissible_) {
                    popover->close();
                }
            }
        );
        dismiss->set_content(scope);

        // 模态内容里再展开的浮层必须压过模态遮罩，否则会被埋掉且点不到；同时登记为
        // 父浮层的子层，外层收起时内层随之关闭，不会留下指向已消失锚点的孤儿。
        const auto parent_id = host->overlay_containing(*this);
        const auto level =
            parent_id != 0 ? scene::OverlayLevel::nested_popup : scene::OverlayLevel::popup;
        portal_handle_ = std::make_unique<scene::OverlayHandle>(
            host->present(dismiss, {.level = level, .block_below = false, .parent = parent_id})
        );
        position_surface(anchor, viewport_size, dismiss);
    }

    void Popover::position_surface(
        const foundation::NanRect& anchor,
        const foundation::NanSize& viewport_size,
        const std::shared_ptr<internal::DismissLayer>& dismiss
    ) {
        // Measure against loose constraints so the size is known before the overlay
        // layout runs; `flip` / `shift` need a concrete size on the first frame.
        const auto surface_size = surface_->measure_layout(scene::LayoutConstraints::loose());
        if (!surface_size.is_valid()) {
            return;
        }
        const auto viewport =
            foundation::NanRect::from_origin_size(foundation::NanPoint::zero(), viewport_size);
        const auto position =
            internal::position_anchored_overlay(anchor, surface_size, viewport, position_options());

        // 覆盖层的内容层与视口同原点，因此面板在父容器里的局部坐标就是屏幕坐标。
        surface_->set_anchor_origin(position.rect.get_top_left());
        surface_->layout_to(
            foundation::NanRect::from_origin_size(surface_->anchor_origin(), surface_size)
        );
        // 外部点击判定用面板屏幕矩形，而不是 content 的 global_bounds：捕获阶段在布局
        // 落定之前运行，那时 content 的边界还是旧值。
        if (dismiss != nullptr) {
            dismiss->set_hit_bounds(
                foundation::NanRect::from_origin_size(position.rect.get_top_left(), surface_size)
            );
        }
        portal_anchor_ = anchor;
        portal_viewport_ = viewport_size;
        portal_placement_ = placement_;
        portal_alignment_ = alignment_;
    }

    auto Popover::refresh_portal() -> void {
        if (!open_ || mount_mode_ != MountMode::overlay) {
            return;
        }
        // 内容 / 主题变化后重建浮层：内容换了节点，就地改样式只会漏掉尺寸变化。
        close_portal();
        sync_portal();
    }

    auto Popover::overlay_dismiss_layer() const -> std::shared_ptr<internal::DismissLayer> {
        // 面板在浮层里的父链是 Surface → FocusScope → DismissLayer；重定位时从这里取回
        // dismiss 层，比另存一份弱引用更可靠（OverlayHost 只弱引用它托管的控件）。
        if (surface_ == nullptr || surface_->parent() == nullptr
            || surface_->parent()->parent() == nullptr)
        {
            return nullptr;
        }
        auto* dismiss = surface_->parent()->parent()->as_control();
        if (dismiss == nullptr) {
            return nullptr;
        }
        return std::static_pointer_cast<internal::DismissLayer>(dismiss->weak_from_this().lock());
    }

    auto Popover::close_portal() -> void {
        if (portal_handle_ != nullptr) {
            portal_handle_->close();
            portal_handle_.reset();
        }
        portal_anchor_ = foundation::NanRect {};
        portal_viewport_ = foundation::NanSize {};
        portal_placement_ = internal::OverlayPlacement::bottom;
        portal_alignment_ = internal::OverlayAlignment::start;
    }

    auto Popover::position_options() const -> internal::AnchoredPositionOptions {
        return {
            .placement = placement_,
            .alignment = alignment_,
            .gap = gap_,
            .viewport_padding = viewport_padding_,
            .flip = true,
            .shift = true,
        };
    }

    auto Popover::finish_close(const bool notify) -> void {
        if (notify) {
            // 焦点恢复：FocusScope 记录了打开前的焦点节点，浮层卸载时会把焦点放回原处；
            // 这里只在焦点没有落点时兜底到触发控件，避免焦点留在已消失的浮层里。
            if (auto* tree = get_tree(); tree != nullptr && tree->focused_node() == nullptr) {
                if (auto trigger = trigger_.lock(); trigger != nullptr && trigger->is_inside_tree())
                {
                    (void)tree->focus_first_within(*trigger);
                }
            }
            if (on_close_) {
                on_close_();
            }
        }
        mark_dirty(
            scene::DirtyFlags::paint | scene::DirtyFlags::layout | scene::DirtyFlags::semantics
        );
    }

    void Popover::detach_tree_fallback() {
        if (surface_ != nullptr && surface_->parent() != nullptr) {
            (void)surface_->parent()->remove_child(*surface_);
        }
        if (dismiss_layer_ != nullptr && dismiss_layer_->parent() == this) {
            // 用同步的 remove_child 而不是 remove_and_delete：树遍历期间删除是延迟的，
            // 只依赖它会让回退子树在关闭后仍然挂在那里，下一次打开时 parent() 检查失真。
            (void)remove_child(*dismiss_layer_);
        }
    }

    void Popover::layout_tree_fallback() {
        if (dismiss_layer_ == nullptr || focus_scope_ == nullptr || surface_ == nullptr) {
            return;
        }
        // 没有 OverlayHost 时的树内回退：内容留在 Popover 自己的子树里，DismissLayer 与
        // FocusScope 提供同样的关闭与焦点语义（DismissLayer 只覆盖 Popover 自身边界，
        // 因此这个回退只保证"内容能显示、能聚焦、能程序化关闭"，不做全屏外部点击关闭）。
        surface_->set_style(resolved_style());
        surface_->set_content(content_);
        if (focus_scope_->content() != surface_.get()) {
            focus_scope_->set_content(surface_);
        }
        if (dismiss_layer_->content() != focus_scope_.get()) {
            dismiss_layer_->set_content(focus_scope_);
        }
        if (dismiss_layer_->parent() != this) {
            add_child(dismiss_layer_);
        }
        mark_layout_dirty();
    }

    auto Popover::tree_viewport() const -> foundation::NanRect {
        // 回退路径没有宿主视口：用离树最近的祖先边界近似。没有祖先时退回自身边界，
        // 位置器仍会给出一个合法结果，不会崩。
        const scene::NanNode* top = this;
        for (const auto* node = parent(); node != nullptr; node = node->parent()) {
            top = node;
        }
        const auto* control = top->as_control();
        if (control == nullptr) {
            return local_rect();
        }
        const auto bounds = control->global_bounds();
        return bounds.is_valid() ? bounds : local_rect();
    }

    auto Popover::on_measure(const scene::LayoutConstraints constraints) -> foundation::NanSize {
        auto trigger = trigger_.lock();
        if (!trigger) {
            return constraints.constrain(foundation::NanSize(0.0F, 0.0F));
        }
        return constraints.constrain(trigger->measure_layout(constraints));
    }

    auto Popover::on_layout() -> void {
        auto trigger = trigger_.lock();
        if (trigger == nullptr) {
            return;
        }
        // 触发控件始终铺满 Popover 自己的矩形：Popover 是它在布局里的"占位"。
        trigger->layout_to(local_rect());

        if (!open_ || mount_mode_ == MountMode::overlay || dismiss_layer_ == nullptr
            || dismiss_layer_->parent() != this)
        {
            return;
        }

        // DismissLayer 与 FocusScope 铺满 Popover 自己的矩形；面板由下面的锚点定位决定。
        // 必须在最前面：面板挂上来之前这里可能提前返回，回退子树就会停在默认尺寸上。
        dismiss_layer_->layout_to(local_rect());

        const auto surface_size = surface_->measure_layout(scene::LayoutConstraints::loose());
        if (!surface_size.is_valid()) {
            return;
        }
        const auto anchor = trigger->global_bounds();
        const auto viewport = tree_viewport();
        if (!anchor.is_valid() || !viewport.is_valid()) {
            return;
        }
        const auto position =
            internal::position_anchored_overlay(anchor, surface_size, viewport, position_options());
        // 位置器给出的是屏幕坐标；Popover 的局部原点是 DismissLayer 的 (0,0)。
        const auto screen_origin = position.rect.get_top_left();

        surface_->set_anchor_origin(screen_origin - global_bounds().get_top_left());
        surface_->layout_to(
            foundation::NanRect::from_origin_size(surface_->anchor_origin(), surface_size)
        );
        if (dismiss_layer_ != nullptr) {
            dismiss_layer_->set_hit_bounds(
                foundation::NanRect::from_origin_size(screen_origin, surface_size)
            );
        }
    }

    auto Popover::semantics_properties() const -> semantics::Properties {
        // 非模态容器的状态就是展开 / 收起。语义树没有 expanded 位，`checked` 是最接近
        // 的可选布尔状态；触发控件是独立节点，自己暴露 action。
        return {
            .role = semantics::Role::generic,
            .value = open_ ? "expanded" : "collapsed",
            .state = {.checked = open_},
        };
    }
} // namespace nandina::widget
