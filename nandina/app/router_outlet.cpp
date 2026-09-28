#include "router_outlet.hpp"

#include "../scene/scene_tree.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace nandina::app
{
    auto RouterOutlet::set_page(std::shared_ptr<scene::NanNode2D> page) -> scene::NanNode2D& {
        if (!page) {
            throw std::invalid_argument("RouterOutlet::set_page: page is null");
        }
        if (child_count() > 1) {
            throw std::logic_error("RouterOutlet::set_page: outlet contains multiple pages");
        }
        auto* requested = page.get();
        pending_page_ = std::move(page);

        if (auto* tree = get_tree(); tree != nullptr && tree->defers_tree_mutation()) {
            // 遍历期间 remove_child 会直接抛，所以现在不能换根。只排一个 mutation：
            // 同阶段内再次 set_page 只会覆盖 pending_page_，flush 时换的是最后一个，
            // 不会出现"两个替换各自捕获陈旧 current"而摘错节点、留下僵尸页面。
            if (swap_queued_) {
                return *requested;
            }
            swap_queued_ = true;
            auto self = std::static_pointer_cast<RouterOutlet>(shared_from_this());
            tree->defer_tree_mutation([self] {
                self->swap_queued_ = false;
                self->apply_page_swap();
            });
            return *requested;
        }

        apply_page_swap();
        return *requested;
    }

    void RouterOutlet::apply_page_swap() {
        auto next = std::move(pending_page_);
        pending_page_.reset();
        if (!next) {
            // 排队之后又被 clear_page() 取消了。
            return;
        }
        // current 在此刻解析，而不是在排队时捕获。
        replace_child(get_child(0), std::move(next));
    }

    void RouterOutlet::clear_page() {
        pending_page_.reset();
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
