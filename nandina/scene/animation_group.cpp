//
// scene/animation_group - parallel / sequential / stagger composition implementation.
//

#include "animation_group.hpp"

#include <utility>

namespace nandina::scene
{
    auto AnimationGroup::parallel(std::vector<Clip> clips) -> AnimationGroup {
        for (auto& clip: clips) {
            clip.ready = [](float) { return true; };
        }
        return AnimationGroup(std::move(clips));
    }

    auto AnimationGroup::sequential(std::vector<Clip> clips) -> AnimationGroup {
        if (clips.empty()) {
            return AnimationGroup {};
        }
        clips.front().ready = [](float) { return true; };
        for (std::size_t i = 1; i < clips.size(); ++i) {
            const Clip* previous = &clips[i - 1];
            clips[i].ready = [previous](float) {
                return previous->started && !previous->animating();
            };
        }
        return AnimationGroup(std::move(clips));
    }

    auto AnimationGroup::stagger(std::vector<Clip> clips, const float interval) -> AnimationGroup {
        float delay = 0.0F;
        for (auto& clip: clips) {
            const float at = delay;
            clip.ready = [at](const float elapsed) { return elapsed >= at; };
            delay += interval;
        }
        return AnimationGroup(std::move(clips));
    }

    void AnimationGroup::advance(const float dt) {
        elapsed_ += dt;
        for (auto& clip: clips_) {
            if (!clip.started && clip.ready(elapsed_)) {
                clip.start();
                clip.started = true;
            }
        }
        for (auto& clip: clips_) {
            if (!clip.started) {
                continue;
            }
            if (clip.tick(dt)) {
                clip.owner->mark_dirty(clip.dirty);
            }
        }
    }

    void AnimationGroup::finish() {
        for (auto& clip: clips_) {
            if (!clip.started) {
                clip.start();
                clip.started = true;
            }
            clip.finish();
            clip.owner->mark_dirty(clip.dirty);
        }
    }

    auto AnimationGroup::finished() const -> bool {
        for (const auto& clip: clips_) {
            if (!clip.started) {
                return false;
            }
            if (clip.animating()) {
                return false;
            }
        }
        return true;
    }
} // namespace nandina::scene
