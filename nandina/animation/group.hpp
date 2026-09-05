//
// animation/group - parallel / sequential / stagger composition of animated properties.
//
// 一个 Group 聚合多个「clip」，每个 clip 包装一个 AnimatedProperty<T> 的目标写入与逐帧
// tick。Group 由场景树 AnimationHost 以同一时钟与取消语义推进，内部按每个 clip 的
// `ready(elapsed)` 谓词决定触发时机：
//   - parallel：全部立即触发；
//   - stagger：第 i 个 clip 在 `i * interval` 秒后触发；
//   - sequential：第 i 个 clip 在前一个 clip 完成后触发（与 dt 粒度无关）。
//

#ifndef NANDINA_EXPERIMENT_ANIMATION_GROUP_HPP
#define NANDINA_EXPERIMENT_ANIMATION_GROUP_HPP

#include "../scene/control.hpp"
#include "animated_property.hpp"
#include "behavior.hpp"

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace nandina::animation
{
    class Group {
    public:
        struct Clip {
            bool started = false;
            /// elapsed(秒) → 是否该触发本 clip（time-based 或 completion-based）。
            std::function<bool(float)> ready;
            std::function<void()> start;
            /// 返回本帧 value 是否确实变化。
            std::function<bool(float)> tick;
            std::function<bool()> animating;
            std::function<void()> finish;
            scene::NanControl* owner = nullptr;
            scene::DirtyFlags dirty = scene::DirtyFlags::none;
        };

        Group() = default;
        explicit Group(std::vector<Clip> clips): clips_(std::move(clips)) {}

        // sequential 的 ready 谓词用指针引用相邻 clip，move 会保留底层缓冲地址因此安全，
        // 而 copy 会使指针悬垂，故 Group 只可移动、不可复制。
        Group(const Group&) = delete;
        auto operator=(const Group&) -> Group& = delete;
        Group(Group&&) = default;
        auto operator=(Group&&) -> Group& = default;

        /// 构造一个「立即触发」的 clip：把 property 动画到 target（安装 behavior）。
        template<typename T>
        static auto clip(
            scene::NanControl& owner,
            AnimatedProperty<T>& property,
            T target,
            Behavior<T> behavior,
            const scene::DirtyFlags dirty
        ) -> Clip {
            return Clip {
                .started = false,
                .ready = [](float) { return true; },
                .start = [&property, target, behavior = std::move(behavior)]() mutable {
                    property.set_behavior(std::move(behavior));
                    property.set_target(std::move(target));
                },
                .tick = [&property](const float dt) {
                    const T before = property.value();
                    (void)property.tick(dt);
                    return !(before == property.value());
                },
                .animating = [&property] { return property.is_animating(); },
                .finish = [&property] { property.finish(); },
                .owner = &owner,
                .dirty = dirty,
            };
        }

        /// 全部 clip 立即触发。
        static auto parallel(std::vector<Clip> clips) -> Group;
        /// 第 i 个 clip 在前一个 clip 完成后触发。
        static auto sequential(std::vector<Clip> clips) -> Group;
        /// 第 i 个 clip 在 `i * interval` 秒后触发。
        static auto stagger(std::vector<Clip> clips, float interval) -> Group;

        void advance(float dt);
        /// 立即触发所有未触发的 clip 并跳到各自目标（取消 / 归约动效）。
        void finish();
        [[nodiscard]] auto finished() const -> bool;

    private:
        std::vector<Clip> clips_;
        float elapsed_ = 0.0F;
    };
} // namespace nandina::animation

#endif // NANDINA_EXPERIMENT_ANIMATION_GROUP_HPP
