#include "router_outlet.hpp"

#include <cmath>
#include <stdexcept>

namespace nandina::app
{
    auto RouterOutlet::set_page(std::shared_ptr<scene::NanNode2D> page) -> scene::NanNode2D& {
        if (!page) {
            throw std::invalid_argument("RouterOutlet::set_page: page is null");
        }
        if (child_count() > 1) {
            throw std::logic_error("RouterOutlet::set_page: outlet contains multiple pages");
        }
        return static_cast<scene::NanNode2D&>(
            replace_child(child_count() == 0 ? nullptr : get_child(0), std::move(page))
        );
    }

    void RouterOutlet::clear_page() {
        if (auto* current = get_child(0); current != nullptr) {
            remove_and_delete(*current);
        }
    }

    auto RouterOutlet::page() const -> scene::NanNode2D* {
        return child_count() > 0 ? get_child(child_count() - 1)->as_node2d() : nullptr;
    }

    void RouterOutlet::on_process(const float dt) {
        (void)dt;
        if (on_tick) {
            on_tick();
        }
    }

    auto RouterOutlet::on_measure(const scene::LayoutConstraints constraints)
        -> foundation::NanSize {
        // An outlet is a shell mount point, so it fills the space offered by its
        // parent rather than measuring to its own (initially zero) size. Keep the
        // current size as a fallback for an unbounded axis, where there is no
        // meaningful viewport to fill.
        const auto available_width = std::isfinite(constraints.max_width)
            ? constraints.max_width
            : size().get_width();
        const auto available_height = std::isfinite(constraints.max_height)
            ? constraints.max_height
            : size().get_height();
        return constraints.constrain(
            foundation::NanSize(available_width, available_height)
        );
    }

    auto RouterOutlet::on_layout() -> void {
        for (std::size_t i = 0; i < child_count(); ++i) {
            auto* child = get_child(i) != nullptr ? get_child(i)->as_control() : nullptr;
            if (!child || !child->visible()) {
                continue;
            }
            (void)child->measure_layout(scene::LayoutConstraints::tight(size()));
            child->layout_to(local_rect());
        }
    }
} // namespace nandina::app
