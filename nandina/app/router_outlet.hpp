//
// app/router_outlet — the stable page mount owned by a window shell.
//

#ifndef NANDINA_EXPERIMENT_APP_ROUTER_OUTLET_HPP
#define NANDINA_EXPERIMENT_APP_ROUTER_OUTLET_HPP

#include "../scene/control.hpp"

#include <functional>
#include <memory>

namespace nandina::app
{
    /// A shell-owned mount point for router content.
    ///
    /// The Router currently uses the same node for its legacy stack frames and
    /// for the typed current-route path. Shell code only needs the stable node;
    /// it never owns or replaces the page children directly.
    class RouterOutlet final: public scene::NanControl {
    public:
        std::function<void()> on_tick;

        auto set_page(std::shared_ptr<scene::NanNode2D> page) -> scene::NanNode2D&;
        void clear_page();
        [[nodiscard]] auto page() const -> scene::NanNode2D*;

    protected:
        void on_process(float dt) override;
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        auto on_layout() -> void override;
    };
} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_ROUTER_OUTLET_HPP
