// scene/anchor_canvas — explicit sibling-relative layout container.
#ifndef NANDINA_SCENE_ANCHOR_CANVAS_HPP
#define NANDINA_SCENE_ANCHOR_CANVAS_HPP

#include "control.hpp"

#include <limits>
#include <span>

namespace nandina::scene
{
    class AnchorCanvas: public NanControl {
    public:
        struct Update {
            std::shared_ptr<NanControl> node;
            AnchorSpec anchors;
        };

        /// Attach without solving: forward references may bind later in the builder expression.
        auto add(std::shared_ptr<NanControl> child) -> AnchorCanvas& {
            add_child(std::move(child));
            return *this;
        }

        [[nodiscard]] auto is_anchor_canvas() const -> bool override {
            return true;
        }
        [[nodiscard]] auto child_axis_assignment(const NanControl& child, bool horizontal) const
            -> ChildAxisAssignment override;
        /// Validates at the current canvas size; commits descriptions, not geometry.
        /// Intended for runtime mode changes after the canvas has received a layout size.
        void set_child_anchors(std::span<const Update> updates);

    protected:
        [[nodiscard]] auto accepts_child(const NanNode& child) const -> bool override;
        [[nodiscard]] auto on_measure(foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;

    private:
        enum class AxisMode {
            definite,
            content,
        };

        struct SolveAxis {
            AxisMode mode = AxisMode::definite;
            float extent = 0.0F;
            float max_limit = std::numeric_limits<float>::infinity();
        };

        struct Placement {
            NanControl* node;
            foundation::NanRect rect;
        };

        struct SolveOutcome {
            std::vector<Placement> placements;
            foundation::NanSize content_bounds;
        };

        [[nodiscard]] auto has_definite_canvas_axis(bool horizontal) const -> bool;
        [[nodiscard]] auto children_need_canvas_extent(bool horizontal) const -> bool;
        [[nodiscard]] auto rest_solve_axis(bool horizontal) const -> SolveAxis;
        [[nodiscard]] auto
        solve(SolveAxis axis_x, SolveAxis axis_y, std::span<const Update> updates = {})
            -> SolveOutcome;

        foundation::NanSize committed_size_ {};
        const NanNode* measured_parent_ = nullptr;
        float last_content_max_width_ = std::numeric_limits<float>::infinity();
        float last_content_max_height_ = std::numeric_limits<float>::infinity();
        bool content_sized_x_ = false;
        bool content_sized_y_ = false;
        bool had_content_spec_x_ = true;
        bool had_content_spec_y_ = true;
        bool deferred_width_ = false;
        bool deferred_height_ = false;
        bool has_measure_state_ = false;
        bool has_committed_layout_ = false;
    };
} // namespace nandina::scene
#endif
