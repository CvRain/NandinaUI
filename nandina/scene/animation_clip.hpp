// scene/animation_clip - type-erased property animation clip.

#ifndef NANDINA_SCENE_ANIMATION_CLIP_HPP
#define NANDINA_SCENE_ANIMATION_CLIP_HPP

#include "frame_scheduler.hpp"

#include <functional>

namespace nandina::scene
{
    class NanNode2D;

    struct AnimationClip {
        bool started = false;
        /// Property identity used by AnimationHost to arbitrate conflicting tracks.
        const void* identity = nullptr;
        /// elapsed (seconds) -> whether this clip should start.
        std::function<bool(float)> ready;
        std::function<void()> start;
        /// Returns whether the property's value changed during this tick.
        std::function<bool(float)> tick;
        std::function<bool()> animating;
        std::function<void()> finish;
        NanNode2D* owner = nullptr;
        DirtyFlags dirty = DirtyFlags::none;
    };
} // namespace nandina::scene

#endif // NANDINA_SCENE_ANIMATION_CLIP_HPP
