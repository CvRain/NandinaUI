//
// Created by cvrain on 2026/7/2.
//
// Screen-space axis-aligned rectangle clip stack.
//
// Semantics:
//  - The stack is a list of screen-space NanRects. push() intersects the new
//    rect with the current top, so a child clip never exceeds an ancestor clip.
//  - Whenever the top changes, device.set_clip(top) is called; when empty,
//    device.clear_clip().
//  - push/pop are managed via an RAII Guard, preventing clip leaks from a
//    forgotten pop.
//

#ifndef NANDINA_EXPERIMENT_CLIP_STACK_HPP
#define NANDINA_EXPERIMENT_CLIP_STACK_HPP

#include "../foundation/geometry.hpp"
#include "render_device.hpp"

#include <vector>

namespace nandina::render
{
    using foundation::NanRect;

    class ClipStack {
    public:
        explicit ClipStack(IRenderDevice& device): device_(device) {}

        ClipStack(const ClipStack&) = delete;
        auto operator=(const ClipStack&) -> ClipStack& = delete;
        ClipStack(ClipStack&&) = delete;
        auto operator=(ClipStack&&) -> ClipStack& = delete;
        ~ClipStack() = default;

        /// RAII handle: pops the pushed clip when it goes out of scope.
        class Guard {
        public:
            Guard(ClipStack* owner, bool active): owner_(owner), active_(active) {}
            ~Guard() {
                if (active_ && owner_ != nullptr) {
                    owner_->pop();
                }
            }
            Guard(Guard&& other) noexcept: owner_(other.owner_), active_(other.active_) {
                other.owner_ = nullptr;
                other.active_ = false;
            }
            auto operator=(Guard&&) -> Guard& = delete;
            Guard(const Guard&) = delete;
            auto operator=(const Guard&) -> Guard& = delete;

        private:
            ClipStack* owner_;
            bool active_;
        };

        /// Intersect with the current top, push the result, return an RAII guard.
        [[nodiscard]] auto push(const NanRect& screen_rect) -> Guard {
            const NanRect effective =
                stack_.empty() ? screen_rect : stack_.back().intersected(screen_rect);
            stack_.push_back(effective);
            device_.set_clip(effective);
            return Guard {this, true};
        }

        /// Current effective clip, or empty() if the stack is empty.
        [[nodiscard]] auto current() const -> NanRect {
            return stack_.empty() ? NanRect::empty() : stack_.back();
        }

        [[nodiscard]] auto depth() const -> std::size_t {
            return stack_.size();
        }

    private:
        void pop() {
            if (stack_.empty()) {
                return;
            }
            stack_.pop_back();
            if (stack_.empty()) {
                device_.clear_clip();
            }
            else {
                device_.set_clip(stack_.back());
            }
        }

        IRenderDevice& device_;
        std::vector<NanRect> stack_;
    };

} // namespace nandina::render

#endif // NANDINA_EXPERIMENT_CLIP_STACK_HPP
