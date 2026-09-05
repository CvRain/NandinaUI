//
// animation/animation_host - scene-owned scheduler for active animated properties.
//

#include "animation_host.hpp"

#include "../scene/scene_tree.hpp"
#include "group.hpp"

#include <algorithm>

namespace nandina::animation
{
    AnimationHost::AnimationHost(scene::NanSceneTree& tree) noexcept: tree_(&tree) {}

    void AnimationHost::advance(const float dt) {
        if (reduced_motion()) {
            clear();
            return;
        }
        auto track = tracks_.begin();
        while (track != tracks_.end()) {
            const auto owner = track->owner.lock();
            auto* control = owner != nullptr ? owner->as_control() : nullptr;
            if (control == nullptr || control->get_tree() != tree_) {
                if (owner != nullptr) {
                    track->finish();
                }
                track = tracks_.erase(track);
                continue;
            }

            const auto result = track->tick(dt);
            if (result.changed) {
                control->mark_dirty(track->dirty_flags);
            }
            if (!result.active) {
                track = tracks_.erase(track);
                continue;
            }
            ++track;
        }
    }

    void AnimationHost::cancel_owner(const scene::NanNode& owner) {
        std::erase_if(tracks_, [&owner](const Track& track) {
            const auto current = track.owner.lock();
            if (current != nullptr && current.get() == &owner) {
                track.finish();
                return true;
            }
            return current == nullptr;
        });
    }

    void AnimationHost::clear() {
        for (const auto& track: tracks_) {
            const auto owner = track.owner.lock();
            auto* control = owner != nullptr ? owner->as_control() : nullptr;
            if (control == nullptr) {
                continue;
            }
            track.finish();
            control->mark_dirty(track.dirty_flags);
        }
        tracks_.clear();
    }

    auto AnimationHost::active_count() const noexcept -> std::size_t {
        return tracks_.size();
    }

    void AnimationHost::run(scene::NanControl& owner, Group group) {
        if (owner.get_tree() != tree_) {
            throw std::invalid_argument("animation owner must belong to the host scene tree");
        }
        auto weak_owner = owner.weak_from_this();
        if (weak_owner.expired()) {
            throw std::logic_error("animation owner must be managed by shared_ptr");
        }
        auto shared = std::make_shared<Group>(std::move(group));
        upsert(
            std::move(weak_owner),
            static_cast<const void*>(shared.get()),
            [shared](const float dt) {
                shared->advance(dt);
                return TickResult {
                    .changed = false,
                    .active = !shared->finished(),
                };
            },
            [shared] { shared->finish(); },
            scene::DirtyFlags::none
        );
    }

    void AnimationHost::upsert(
        std::weak_ptr<scene::NanNode> owner,
        const void* identity,
        std::function<TickResult(float)> tick,
        std::function<void()> finish,
        const scene::DirtyFlags dirty_flags
    ) {
        const auto existing =
            std::ranges::find(tracks_, identity, [](const Track& track) { return track.identity; });
        if (existing != tracks_.end()) {
            existing->owner = std::move(owner);
            existing->tick = std::move(tick);
            existing->finish = std::move(finish);
            existing->dirty_flags |= dirty_flags;
            return;
        }
        tracks_.push_back(
            Track {
                .owner = std::move(owner),
                .identity = identity,
                .tick = std::move(tick),
                .finish = std::move(finish),
                .dirty_flags = dirty_flags,
            }
        );
    }

    void AnimationHost::cancel_property(const void* identity) noexcept {
        std::erase_if(tracks_, [identity](const Track& track) {
            return track.identity == identity;
        });
    }

    auto AnimationHost::reduced_motion() const noexcept -> bool {
        const auto* manager = tree_->theme_manager();
        return manager != nullptr && manager->reduced_motion();
    }
} // namespace nandina::animation
