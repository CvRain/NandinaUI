//
// scene/animation_group - parallel / sequential / stagger composition of animated properties.
//
// 一个 Group 聚合多个「clip」，每个 clip 包装一个 AnimatedProperty<T> 的目标写入与逐帧
// tick。Group 由场景树 AnimationHost 以同一时钟与取消语义推进，内部按每个 clip 的
// `ready(elapsed)` 谓词决定触发时机：
//   - parallel：全部立即触发；
//   - stagger：第 i 个 clip 在 `i * interval` 秒后触发；
//   - sequential：第 i 个 clip 在前一个 clip 完成后触发（与 dt 粒度无关）。
//

#ifndef NANDINA_SCENE_ANIMATION_GROUP_HPP
#define NANDINA_SCENE_ANIMATION_GROUP_HPP

#include "../foundation/motion/animated_property.hpp"
#include "../foundation/motion/behavior.hpp"
#include "animation_clip.hpp"
#include "node2d.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace nandina::scene
{
    class AnimationGroup {
    public:
        using Clip = AnimationClip;

        AnimationGroup() = default;
        explicit AnimationGroup(std::vector<Clip> clips): clips_(std::move(clips)) {}

        // sequential 的 ready 谓词用指针引用相邻 clip，move 会保留底层缓冲地址因此安全，
        // 而 copy 会使指针悬垂，故 Group 只可移动、不可复制。
        AnimationGroup(const AnimationGroup&) = delete;
        auto operator=(const AnimationGroup&) -> AnimationGroup& = delete;
        AnimationGroup(AnimationGroup&&) = default;
        auto operator=(AnimationGroup&&) -> AnimationGroup& = default;

        /// 构造一个「立即触发」的 clip：把 property 动画到 target（安装 behavior）。
        template<typename T>
        static auto clip(
            NanNode2D& owner,
            motion::AnimatedProperty<T>& property,
            T target,
            motion::Behavior<T> behavior,
            const DirtyFlags dirty
        ) -> Clip {
            return Clip {
                .started = false,
                .identity = static_cast<const void*>(std::addressof(property)),
                .ready = [](float) { return true; },
                .start =
                    [&property, target, behavior = std::move(behavior)]() mutable {
                        property.set_behavior(std::move(behavior));
                        property.set_target(std::move(target));
                    },
                .tick =
                    [&property](const float dt) {
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
        static auto parallel(std::vector<Clip> clips) -> AnimationGroup;
        /// 第 i 个 clip 在前一个 clip 完成后触发。
        static auto sequential(std::vector<Clip> clips) -> AnimationGroup;
        /// 第 i 个 clip 在 `i * interval` 秒后触发。
        static auto stagger(std::vector<Clip> clips, float interval) -> AnimationGroup;

        void advance(float dt);
        /// Reject invalid clips before the Host changes any active track. Raw
        /// property clips remain borrowed; their storage must outlive playback.
        void validate_owner(const NanNode2D& owner) const;
        /// 立即触发所有未触发的 clip 并跳到各自目标（取消 / 归约动效）。
        void finish();
        [[nodiscard]] auto finished() const -> bool;

        /// 返回 group 中的属性身份，供 AnimationHost 安装前取消冲突轨道。
        [[nodiscard]] auto identities() const -> std::vector<const void*>;

    private:
        std::vector<Clip> clips_;
        float elapsed_ = 0.0F;
    };
} // namespace nandina::scene

#endif // NANDINA_SCENE_ANIMATION_GROUP_HPP
