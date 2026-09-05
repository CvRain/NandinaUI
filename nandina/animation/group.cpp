//
// animation/group - parallel / sequential / stagger composition implementation.
//

#include "group.hpp"

#include <utility>

namespace nandina::animation
{
    auto Group::parallel(std::vector<Clip> clips) -> Group {
        for (auto& clip: clips) {
            clip.ready = [](float) { return true; };
        }
        return Group(std::move(clips));
    }

    auto Group::sequential(std::vector<Clip> clips) -> Group {
        if (clips.empty()) {
            return Group {};
        }
        clips.front().ready = [](float) { return true; };
        for (std::size_t i = 1; i < clips.size(); ++i) {
            const Clip* previous = &clips[i - 1];
            clips[i].ready = [previous](float) {
                return previous->started && !previous->animating();
            };
        }
        return Group(std::move(clips));
    }

    auto Group::stagger(std::vector<Clip> clips, const float interval) -> Group {
        float delay = 0.0F;
        for (auto& clip: clips) {
            const float at = delay;
            clip.ready = [at](const float elapsed) { return elapsed >= at; };
            delay += interval;
        }
        return Group(std::move(clips));
    }

    void Group::advance(const float dt) {
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

    void Group::finish() {
        for (auto& clip: clips_) {
            if (!clip.started) {
                clip.start();
                clip.started = true;
            }
            clip.finish();
            clip.owner->mark_dirty(clip.dirty);
        }
    }

    auto Group::finished() const -> bool {
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
} // namespace nandina::animation
