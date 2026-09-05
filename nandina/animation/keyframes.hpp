//
// animation/keyframes - keyframe interpolation for copyable values.
//
// Keyframes<T> 在若干 (time, value) 关键帧之间按时间插值（复用 tween.hpp 的 lerp，因此
// 算术类型与 NanColor 都可用）。time 必须严格递增且首帧为 0；当前插值结果缓存在
// current_ 中，使 value() 始终能返回稳定引用。
//

#ifndef NANDINA_EXPERIMENT_ANIMATION_KEYFRAMES_HPP
#define NANDINA_EXPERIMENT_ANIMATION_KEYFRAMES_HPP

#include "tween.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace nandina::animation
{
    template<typename T>
    struct Keyframe {
        float time = 0.0F;
        T value {};
    };

    template<typename T>
        requires std::copyable<T> && std::equality_comparable<T>
    class Keyframes {
    public:
        Keyframes() = default;

        void start(std::vector<Keyframe<T>> keyframes) {
            validate(keyframes);
            keyframes_ = std::move(keyframes);
            elapsed_ = 0.0F;
            finished_ = false;
            current_ = keyframes_.front().value;
        }

        auto tick(const float dt) -> const T& {
            if (finished_) {
                return current_;
            }
            if (dt > 0.0F) {
                elapsed_ += dt;
            }
            if (elapsed_ >= keyframes_.back().time) {
                elapsed_ = keyframes_.back().time;
                finished_ = true;
            }
            current_ = value_at(elapsed_);
            return current_;
        }

        void finish() {
            elapsed_ = keyframes_.back().time;
            finished_ = true;
            current_ = keyframes_.back().value;
        }

        [[nodiscard]] auto value() const -> const T& {
            return current_;
        }

        [[nodiscard]] auto is_finished() const -> bool {
            return finished_;
        }

        /// 末帧值（作为逻辑 target）。
        [[nodiscard]] auto target() const -> const T& {
            return keyframes_.back().value;
        }

    private:
        [[nodiscard]] auto value_at(const float t) const -> T {
            for (std::size_t i = 0; i + 1 < keyframes_.size(); ++i) {
                const float start = keyframes_[i].time;
                const float end = keyframes_[i + 1].time;
                if (t <= end) {
                    const float local =
                        end > start ? std::clamp((t - start) / (end - start), 0.0F, 1.0F) : 0.0F;
                    return lerp(keyframes_[i].value, keyframes_[i + 1].value, local);
                }
            }
            return keyframes_.back().value;
        }

        static void validate(const std::vector<Keyframe<T>>& keyframes) {
            if (keyframes.empty()) {
                throw std::invalid_argument("keyframes must not be empty");
            }
            if (keyframes.front().time != 0.0F) {
                throw std::invalid_argument("first keyframe time must be 0");
            }
            for (std::size_t i = 1; i < keyframes.size(); ++i) {
                if (!(keyframes[i].time > keyframes[i - 1].time)) {
                    throw std::invalid_argument("keyframe times must be strictly increasing");
                }
            }
        }

        std::vector<Keyframe<T>> keyframes_;
        float elapsed_ = 0.0F;
        T current_ {};
        bool finished_ = true;
    };
} // namespace nandina::animation

#endif // NANDINA_EXPERIMENT_ANIMATION_KEYFRAMES_HPP
