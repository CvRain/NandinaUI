//
// animation/animation_host - scene-owned scheduler for active animated properties.
//

#ifndef NANDINA_EXPERIMENT_ANIMATION_ANIMATION_HOST_HPP
#define NANDINA_EXPERIMENT_ANIMATION_ANIMATION_HOST_HPP

#include "../scene/control.hpp"
#include "animated_property.hpp"
#include "group.hpp"

#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace nandina::scene
{
    class NanSceneTree;
}

namespace nandina::animation
{
    class AnimationHost {
    public:
        explicit AnimationHost(scene::NanSceneTree& tree) noexcept;

        AnimationHost(const AnimationHost&) = delete;
        auto operator=(const AnimationHost&) -> AnimationHost& = delete;

        template<typename T>
            requires std::copyable<T> && std::equality_comparable<T>
        void set_target(
            scene::NanControl& owner,
            AnimatedProperty<T>& property,
            T target,
            scene::DirtyFlags dirty_flags
        ) {
            if (owner.get_tree() != tree_) {
                throw std::invalid_argument("animation owner must belong to the host scene tree");
            }
            auto weak_owner = owner.weak_from_this();
            if (weak_owner.expired()) {
                throw std::logic_error("animation owner must be managed by shared_ptr");
            }

            const T previous = property.value();
            property.set_target(std::move(target));
            if (reduced_motion()) {
                property.finish();
            }
            if (!(previous == property.value())) {
                owner.mark_dirty(dirty_flags);
            }

            const auto* identity = static_cast<const void*>(std::addressof(property));
            if (!property.is_animating()) {
                cancel_property(identity);
                return;
            }

            upsert(
                std::move(weak_owner),
                identity,
                [&property](const float dt) {
                    const T before = property.value();
                    (void)property.tick(dt);
                    return TickResult {
                        .changed = !(before == property.value()),
                        .active = property.is_animating(),
                    };
                },
                [&property] { property.finish(); },
                dirty_flags
            );
        }

        void advance(float dt);
        void cancel_owner(const scene::NanNode& owner);
        void clear();
        [[nodiscard]] auto active_count() const noexcept -> std::size_t;

        /// 组合动画（parallel/sequential/stagger）：把 Group 作为一条轨道推进，
        /// 复用本 Host 的时钟、归约动效与 owner 取消语义。Group 按值移交并由 Host
        /// 持有（保证其生命周期覆盖轨道），内部负责按延迟触发各 clip 并只对确实
        /// 变化的属性传播 DirtyFlags。
        void run(scene::NanControl& owner, Group group);

    private:
        struct TickResult {
            bool changed = false;
            bool active = false;
        };

        struct Track {
            std::weak_ptr<scene::NanNode> owner;
            const void* identity = nullptr;
            std::function<TickResult(float)> tick;
            std::function<void()> finish;
            scene::DirtyFlags dirty_flags = scene::DirtyFlags::none;
        };

        void upsert(
            std::weak_ptr<scene::NanNode> owner,
            const void* identity,
            std::function<TickResult(float)> tick,
            std::function<void()> finish,
            scene::DirtyFlags dirty_flags
        );
        void cancel_property(const void* identity) noexcept;

        /// 全局 reduced-motion policy：宿主所在场景树的 ThemeManager 归约动效时，
        /// 新目标直跳、在途轨道立即完成。无 ThemeManager 时视为未归约。
        [[nodiscard]] auto reduced_motion() const noexcept -> bool;

        scene::NanSceneTree* tree_;
        std::vector<Track> tracks_;
    };
} // namespace nandina::animation

#endif // NANDINA_EXPERIMENT_ANIMATION_ANIMATION_HOST_HPP
