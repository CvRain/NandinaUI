// scene/anchor_canvas — explicit sibling-relative layout container.
#ifndef NANDINA_SCENE_ANCHOR_CANVAS_HPP
#define NANDINA_SCENE_ANCHOR_CANVAS_HPP

#include "control.hpp"

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
        /// Validates at the current canvas size; commits descriptions, not geometry.
        /// Intended for runtime mode changes after the canvas has received a layout size.
        void set_child_anchors(std::span<const Update> updates);

    protected:
        [[nodiscard]] auto accepts_child(const NanNode& child) const -> bool override;
        [[nodiscard]] auto on_measure(foundation::NanLayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;

    private:
        struct Placement {
            NanControl* node;
            foundation::NanRect rect;
        };
        [[nodiscard]] auto
        solve(foundation::NanSize available, std::span<const Update> updates = {})
            -> std::vector<Placement>;
    };
} // namespace nandina::scene
#endif
