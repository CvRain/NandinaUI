//
// widget/dialog - modal overlay (scrim + centered panel + focus trap).
//

#include "dialog.hpp"

#include "../animation/animation_host.hpp"
#include "../scene/overlay_host.hpp"
#include "../scene/scene_tree.hpp"
#include "../theme/theme_manager.hpp"
#include "internal/dialog_panel.hpp"
#include "internal/dismiss_layer.hpp"
#include "internal/focus_scope.hpp"

#include <algorithm>
#include <utility>

namespace nandina::widget
{
    namespace
    {
        /// 遮罩与面板共用的淡入淡出行为：进场 ease_out，退场 ease_in。
        [[nodiscard]] auto fade_behavior(const theme::DesignSystem& system, const bool entering)
            -> animation::Behavior<float> {
            return animation::Behavior<float>(
                system.tokens.motion.long_duration,
                entering ? animation::Easing::ease_out : animation::Easing::ease_in
            );
        }

        [[nodiscard]] auto scrim_style(const theme::ResolvedDialogStyle& style)
            -> theme::ResolvedBoxStyle {
            return theme::ResolvedBoxStyle {
                .fill = style.scrim,
                .border = style.scrim,
                .border_width = 0.0F,
                .radius = 0.0F,
            };
        }
    } // namespace

    Dialog::Dialog(theme::NanTheme theme) {
        system_ =
            std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        theme_view_ = theme;

        panel_ = std::make_shared<internal::DialogPanel>();
        focus_scope_ = std::make_shared<internal::FocusScope>();
        focus_scope_->set_content(panel_);
        dismiss_layer_ = std::make_shared<internal::DismissLayer>();
        dismiss_layer_->set_content_centered(true);
        dismiss_layer_->set_content(focus_scope_);
        dismiss_layer_->set_visible(false);

        apply_style();
        set_visible(false); // 初始关闭：面板与内容子节点都不可见。
    }

    Dialog::~Dialog() = default;

    auto Dialog::create(theme::NanTheme theme) -> std::shared_ptr<Dialog> {
        return std::make_shared<Dialog>(theme);
    }

    void Dialog::set_title(std::string title) {
        panel_->set_title(std::move(title));
        mark_semantics_dirty();
    }

    auto Dialog::title() const -> std::string_view {
        return panel_->title();
    }

    auto Dialog::set_header(std::shared_ptr<scene::NanControl> header) -> Dialog& {
        (void)panel_->set_header(std::move(header));
        mark_layout_dirty();
        return *this;
    }

    auto Dialog::set_content(std::shared_ptr<scene::NanControl> content) -> Dialog& {
        (void)panel_->set_content(std::move(content));
        mark_layout_dirty();
        return *this;
    }

    auto Dialog::set_footer(std::shared_ptr<scene::NanControl> footer) -> Dialog& {
        (void)panel_->set_footer(std::move(footer));
        mark_layout_dirty();
        return *this;
    }

    void Dialog::open() {
        if (phase_ == DialogPhase::opening || phase_ == DialogPhase::opened) {
            return;
        }
        if (mount_mode_ == MountMode::unmounted) {
            mount_mode_ = resolve_overlay_host() != nullptr ? MountMode::overlay : MountMode::tree;
        }
        // 可见性先于挂载确定：
        // - 树内回退要求 Dialog 自身可见，FocusScope 入树时才能收集到可聚焦控件并解析初始
        //   焦点，否则焦点留在浮层之外，Escape 与 Tab 都进不来；
        // - 浮层承载时 Dialog 只是页面里的锚点，内容全部在浮层里，保持不可见，否则父级布局
        //   会为这个零高子节点多算一个 gap。
        set_visible(mount_mode_ == MountMode::tree);
        dismiss_layer_->set_visible(true);
        if (!mount()) {
            dismiss_layer_->set_visible(false);
            set_visible(false);
            return;
        }
        phase_ = DialogPhase::opening;
        apply_style();

        // 浮层在淡出期间仍然挂载，重新打开不会再次触发 FocusScope 的入树初始化；而点击
        // 遮罩关闭时焦点已被清空。这里把焦点重新放回浮层内部，Escape 与 Tab 才可达。
        if (auto* tree = get_tree(); tree != nullptr) {
            auto* focused = tree->focused_node();
            if (focused == nullptr || !focus_scope_->is_ancestor_of(*focused)) {
                (void)tree->focus_first_within(*focus_scope_);
            }
        }

        // 上一轮退场把 fade 停在 0；重新打开时先把属性拉回起点再启动进场。
        auto& fade = dismiss_layer_->fade();
        fade.clear_behavior();
        fade.set_target(0.0F);
        fade.set_behavior(fade_behavior(*system_, true));
        start_fade(1.0F);

        mark_dirty(
            scene::DirtyFlags::paint | scene::DirtyFlags::layout | scene::DirtyFlags::semantics
        );
    }

    void Dialog::close() {
        if (!active() || phase_ == DialogPhase::closing) {
            return;
        }
        phase_ = DialogPhase::closing;
        dismiss_layer_->fade().set_behavior(fade_behavior(*system_, false));
        start_fade(0.0F);
        mark_dirty(scene::DirtyFlags::paint | scene::DirtyFlags::semantics);
    }

    auto Dialog::is_open() const -> bool {
        return phase_ == DialogPhase::opening || phase_ == DialogPhase::opened;
    }

    void Dialog::set_dismissible(const bool dismissible) {
        dismissible_ = dismissible;
    }

    auto Dialog::dismissible() const -> bool {
        return dismissible_;
    }

    void Dialog::set_on_close(std::function<void()> callback) {
        on_close_ = std::move(callback);
    }

    void Dialog::set_theme(theme::NanTheme theme) {
        system_ =
            std::make_shared<const theme::DesignSystem>(theme::design_system_from_theme(theme));
        system_explicit_ = true;
        theme_view_ = theme;
        apply_style();
    }

    auto Dialog::theme_ref() const -> const theme::NanTheme& {
        return theme_view_;
    }

    void Dialog::set_override(theme::DialogRecipeRule rule) {
        override_ = std::move(rule);
        apply_style();
    }

    auto Dialog::resolved_style() const -> theme::ResolvedDialogStyle {
        auto style = theme::resolve_dialog(*system_, appearance_);
        if (override_) {
            theme::apply_rule(*system_, appearance_, style, *override_);
        }
        return style;
    }

    void Dialog::set_text_pipeline(primitives::TextPipeline pipeline) {
        panel_->set_text_pipeline(std::move(pipeline));
    }

    void Dialog::apply_default_text_pipeline(const primitives::TextPipeline& pipeline) {
        panel_->apply_default_text_pipeline(pipeline);
    }

    void Dialog::apply_font_context(text::FontPipelineCache& context) {
        panel_->apply_font_context(context);
    }

    void Dialog::on_style_context_changed(const theme::ResolvedStyleContext&) {
        apply_style();
    }

    void Dialog::on_theme_changed(const theme::ThemeManager& manager) {
        appearance_ = manager.appearance();
        if (!system_explicit_) {
            system_ = manager.design_system_shared();
            theme_view_ = theme::NanTheme {system_->tokens, system_->palette(appearance_)};
        }
        apply_style();
    }

    void Dialog::apply_style() {
        const auto style = resolved_style();
        panel_->set_style(style);
        dismiss_layer_->set_scrim(scrim_style(style));
    }

    auto Dialog::z_index_hint() const -> int {
        // 浮层承载时层级由 OverlayLevel 决定；树内回退要靠 z 序压过后续兄弟。
        return active() && mount_mode_ != MountMode::overlay ? 1 : 0;
    }

    auto Dialog::on_measure(const scene::LayoutConstraints constraints) -> foundation::NanSize {
        // 浮层承载时本节点只是页面里的锚点，不占位；树内回退时铺满父容器作为遮罩范围。
        if (!active() || mount_mode_ == MountMode::overlay) {
            return constraints.constrain(foundation::NanSize {0.0F, 0.0F});
        }
        return constraints.constrain(
            foundation::NanSize(constraints.max_width, constraints.max_height)
        );
    }

    void Dialog::on_layout() {
        if (!active() || dismiss_layer_->parent() != this) {
            return;
        }
        dismiss_layer_->layout_to(local_rect());
    }

    void Dialog::on_process(const float /*dt*/) {
        if (!active()) {
            return;
        }
        auto& fade = dismiss_layer_->fade();
        if (phase_ == DialogPhase::opening && !fade.is_animating()) {
            phase_ = DialogPhase::opened;
        }
        else if (phase_ == DialogPhase::closing && !fade.is_animating()) {
            phase_ = DialogPhase::closed;
            dismiss_layer_->set_visible(false);
            unmount();
            set_visible(false);
            if (on_close_) {
                on_close_();
            }
            mark_dirty(
                scene::DirtyFlags::paint | scene::DirtyFlags::layout | scene::DirtyFlags::semantics
            );
        }
    }

    void Dialog::on_exit_tree() {
        phase_ = DialogPhase::closed;
        unmount();
        set_visible(false);
        scene::NanControl::on_exit_tree();
    }

    void Dialog::set_overlay_service(scene::OverlayHost* host) noexcept {
        overlay_service_ =
            host != nullptr ? host->weak_self() : std::weak_ptr<scene::OverlayHost> {};
    }

    auto Dialog::resolve_overlay_host() -> std::shared_ptr<scene::OverlayHost> {
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

    auto Dialog::mount() -> bool {
        if (mount_mode_ == MountMode::overlay) {
            if (portal_handle_ != nullptr) {
                return true;
            }
            auto host = resolve_overlay_host();
            if (host == nullptr) {
                return false;
            }
            install_dismiss_callback();
            portal_handle_ = std::make_unique<scene::OverlayHandle>(host->present(
                dismiss_layer_,
                scene::OverlayOptions {
                    .level = scene::OverlayLevel::modal,
                    .block_below = true,
                }
            ));
            return true;
        }

        // detached 回退：面板留在树内，靠 z 序与铺满父容器维持模态语义。
        if (dismiss_layer_->parent() == this) {
            return true;
        }
        install_dismiss_callback();
        add_child(dismiss_layer_);
        mark_layout_dirty();
        return true;
    }

    void Dialog::install_dismiss_callback() {
        // 关闭请求回到本对话框；弱引用保证浮层比 Dialog 活得久时不会悬空。
        auto weak = std::weak_ptr<Dialog>(std::static_pointer_cast<Dialog>(shared_from_this()));
        dismiss_layer_->set_callback([weak](const internal::DismissReason) {
            if (auto dialog = weak.lock(); dialog != nullptr && dialog->dismissible_) {
                dialog->close();
            }
        });
    }

    void Dialog::unmount() {
        if (portal_handle_ != nullptr) {
            portal_handle_->close();
            portal_handle_.reset();
        }
    }

    void Dialog::start_fade(const float target) {
        if (auto* tree = dismiss_layer_->get_tree(); tree != nullptr) {
            tree->animation_host().set_target(
                *dismiss_layer_, dismiss_layer_->fade(), target, scene::DirtyFlags::paint
            );
            return;
        }
        dismiss_layer_->fade().clear_behavior();
        dismiss_layer_->fade().set_target(target);
    }
} // namespace nandina::widget
