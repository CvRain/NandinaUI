//
// widget/aspect_ratio — single-child container preserving a fixed width/height ratio.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_ASPECT_RATIO_HPP
#define NANDINA_EXPERIMENT_WIDGET_ASPECT_RATIO_HPP

#include "../foundation/geometry.hpp"
#include "../scene/control.hpp"
#include "layout.hpp"

#include <memory>

namespace nandina::widget
{
    /**
     * Single-child layout container that sizes and positions its child to an
     * inscribed box with a fixed `width / height` ratio.
     *
     * Unlike `NanControl::set_aspect_ratio(float)` on a leaf control's `ControlSizeSpec`
     * (which constrains `measure_layout` but can still be overwritten when a parent
     * container such as `Expanded`, `Stack`, `Grid`, or `Resizable` assigns a tight
     * slot via `layout_to(rect)`), `AspectRatio` absorbs the outer slot in `on_layout()`
     * and lays out its child tightly to the largest inscribed rectangle matching `ratio()`.
     */
    class AspectRatio: public scene::NanControl {
    public:
        explicit AspectRatio(
            float ratio = 1.0F,
            LayoutAlignment alignment = LayoutAlignment::center
        );

        [[nodiscard]] static auto
        create(float ratio = 1.0F, LayoutAlignment alignment = LayoutAlignment::center)
            -> std::shared_ptr<AspectRatio>;

        auto set_ratio(float ratio) -> AspectRatio&;
        [[nodiscard]] auto ratio() const noexcept -> float;

        auto set_alignment(LayoutAlignment alignment) -> AspectRatio&;
        [[nodiscard]] auto alignment() const noexcept -> LayoutAlignment;

        auto set_child(std::shared_ptr<scene::NanControl> child) -> AspectRatio&;
        auto clear_child() -> AspectRatio&;
        [[nodiscard]] auto child() const noexcept -> scene::NanControl*;

        /// Local rectangle assigned to the child during the most recent layout pass.
        [[nodiscard]] auto content_rect() const noexcept -> foundation::NanRect;
        [[nodiscard]] auto
        child_axis_assignment(const scene::NanControl& child, bool horizontal) const
            -> scene::ChildAxisAssignment override;

        void relayout();

    protected:
        [[nodiscard]] auto on_measure(foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;
        void on_ready() override;

    private:
        [[nodiscard]] auto resolve_box_for_constraints(foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize;
        [[nodiscard]] auto resolve_child_rect(foundation::NanSize available) const
            -> foundation::NanRect;
        void release_child_input(const scene::NanControl& target);

        float ratio_ = 1.0F;
        LayoutAlignment alignment_ = LayoutAlignment::center;
        std::weak_ptr<scene::NanControl> child_;
        foundation::NanRect content_rect_ = foundation::NanRect::from_xywh(0.0F, 0.0F, 0.0F, 0.0F);
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_ASPECT_RATIO_HPP
