//
// animation/property_endpoint - scene-aware endpoint for an optional animated value.
//

#ifndef NANDINA_EXPERIMENT_ANIMATION_PROPERTY_ENDPOINT_HPP
#define NANDINA_EXPERIMENT_ANIMATION_PROPERTY_ENDPOINT_HPP

#include "../scene/scene_tree.hpp"
#include "animation_host.hpp"
#include "spring.hpp"

#include <concepts>
#include <optional>
#include <type_traits>
#include <utility>

namespace nandina::animation
{
    template<typename T>
        requires std::copyable<T> && std::equality_comparable<T>
    class PropertyEndpoint {
    public:
        PropertyEndpoint(scene::NanControl& owner, const scene::DirtyFlags dirty_flags) noexcept:
            owner_(&owner),
            dirty_flags_(dirty_flags) {}

        PropertyEndpoint(scene::NanControl& owner, T initial, const scene::DirtyFlags dirty_flags):
            owner_(&owner),
            dirty_flags_(dirty_flags),
            property_(std::move(initial)) {}

        void set(T target) {
            if (!property_) {
                property_.emplace(std::move(target));
                install_behavior();
                owner_->mark_dirty(dirty_flags_);
                return;
            }

            if (auto* tree = owner_->get_tree(); tree != nullptr) {
                tree->animation_host()
                    .set_target(*owner_, *property_, std::move(target), dirty_flags_);
                return;
            }

            const T previous = property_->value();
            property_->set_target(std::move(target));
            property_->finish();
            if (!(previous == property_->value())) {
                owner_->mark_dirty(dirty_flags_);
            }
        }

        void set_behavior(Behavior<T> behavior) {
            behavior_ = std::move(behavior);
            if constexpr (std::is_floating_point_v<T>) {
                spring_.reset();
            }
            if (!property_) {
                return;
            }
            const T previous = property_->value();
            property_->set_behavior(*behavior_);
            reconcile(previous);
        }

        void clear_behavior() {
            behavior_.reset();
            if (!property_) {
                return;
            }
            const T previous = property_->value();
            property_->clear_behavior();
            reconcile(previous);
        }

        /// 为浮点类型安装弹簧行为（与 Behavior 互斥）。非浮点类型不可调用。
        void set_spring(SpringSpec spec)
            requires std::is_floating_point_v<T>
        {
            spring_ = std::move(spec);
            behavior_.reset();
            if (!property_) {
                return;
            }
            const T previous = property_->value();
            property_->set_spring(*spring_);
            reconcile(previous);
        }

        void clear_spring()
            requires std::is_floating_point_v<T>
        {
            spring_.reset();
            if (!property_) {
                return;
            }
            const T previous = property_->value();
            property_->clear_spring();
            reconcile(previous);
        }

        void clear() {
            if (!property_) {
                return;
            }
            property_->clear_behavior();
            if constexpr (std::is_floating_point_v<T>) {
                property_->clear_spring();
            }
            if (auto* tree = owner_->get_tree(); tree != nullptr) {
                tree->animation_host()
                    .set_target(*owner_, *property_, property_->target(), dirty_flags_);
            }
            property_.reset();
            owner_->mark_dirty(dirty_flags_);
        }

        [[nodiscard]] auto has_value() const noexcept -> bool {
            return property_.has_value();
        }

        [[nodiscard]] auto value() const noexcept -> const T* {
            return property_ ? std::addressof(property_->value()) : nullptr;
        }

        [[nodiscard]] auto target() const noexcept -> const T* {
            return property_ ? std::addressof(property_->target()) : nullptr;
        }

        [[nodiscard]] auto behavior() const noexcept -> const std::optional<Behavior<T>>& {
            return behavior_;
        }

        [[nodiscard]] auto spring() const noexcept -> std::optional<SpringSpec> {
            return spring_;
        }

    private:
        void install_behavior() {
            if (behavior_) {
                property_->set_behavior(*behavior_);
            }
            if constexpr (std::is_floating_point_v<T>) {
                if (spring_) {
                    property_->set_spring(*spring_);
                }
            }
        }

        void reconcile(const T& previous) {
            if (auto* tree = owner_->get_tree(); tree != nullptr) {
                tree->animation_host()
                    .set_target(*owner_, *property_, property_->target(), dirty_flags_);
            }
            else if (property_->is_animating()) {
                property_->finish();
            }
            if (!(previous == property_->value())) {
                owner_->mark_dirty(dirty_flags_);
            }
        }

        scene::NanControl* owner_;
        scene::DirtyFlags dirty_flags_;
        std::optional<AnimatedProperty<T>> property_;
        std::optional<Behavior<T>> behavior_;
        std::optional<SpringSpec> spring_;
    };
} // namespace nandina::animation

#endif // NANDINA_EXPERIMENT_ANIMATION_PROPERTY_ENDPOINT_HPP
