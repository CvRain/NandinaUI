#ifndef NANDINA_EXPERIMENT_WIDGET_INTERNAL_DISMISS_LAYER_HPP
#define NANDINA_EXPERIMENT_WIDGET_INTERNAL_DISMISS_LAYER_HPP

#include "../../animation/animated_property.hpp"
#include "../../scene/control.hpp"
#include "../../theme/design_system.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace nandina::widget::internal
{
    enum class DismissReason {
        pointer,
        escape,
    };

    /**
     * 模态浮层的全屏表面：铺满宿主，提供点击外部与 Escape 的关闭判定，并可选地绘制遮罩。
     *
     * 它同时是遮罩与内容的共同淡入淡出入口：`fade()` 由宿主用 AnimationHost 推进，
     * `local_opacity()` 把结果乘进整个子树，因此遮罩和面板始终是同一条动画。
     */
    class DismissLayer final: public scene::NanControl {
    public:
        using Callback = std::function<void(DismissReason)>;

        explicit DismissLayer(Callback callback = {}): callback_(std::move(callback)) {}

        auto set_content(std::shared_ptr<scene::NanControl> content) -> scene::NanControl&;
        [[nodiscard]] auto content() const -> scene::NanControl*;

        /// 内容在自己的边界内居中摆放。未开启时按内容自身的位置布局（下拉等锚定浮层用）。
        void set_content_centered(bool centered) {
            content_centered_ = centered;
        }

        void set_callback(Callback callback) {
            callback_ = std::move(callback);
        }

        /// 遮罩：设置后铺满自身边界；未设置时不绘制，保持纯命中层语义。
        void set_scrim(theme::ResolvedBoxStyle scrim) {
            scrim_ = std::move(scrim);
            mark_dirty(scene::DirtyFlags::paint);
        }

        void clear_scrim() {
            scrim_.reset();
            mark_dirty(scene::DirtyFlags::paint);
        }

        [[nodiscard]] auto scrim() const noexcept -> const std::optional<theme::ResolvedBoxStyle>& {
            return scrim_;
        }

        [[nodiscard]] auto fade() noexcept -> animation::AnimatedProperty<float>& {
            return fade_;
        }

        [[nodiscard]] auto local_opacity() const -> float override;

        [[nodiscard]] auto on_input(scene::InputEvent& event) -> bool override;
        [[nodiscard]] auto on_input_capture(scene::InputEvent& event) -> bool override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;
        void on_draw(render::DrawContext& context) override;

    private:
        Callback callback_;
        std::weak_ptr<scene::NanControl> content_;
        bool content_centered_ = false;
        std::optional<theme::ResolvedBoxStyle> scrim_;
        animation::AnimatedProperty<float> fade_ {1.0F};
    };
}

#endif
