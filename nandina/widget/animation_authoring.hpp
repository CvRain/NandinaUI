// widget/animation_authoring - reusable, weak-node recipes for L2 animation groups.

#ifndef NANDINA_WIDGET_ANIMATION_AUTHORING_HPP
#define NANDINA_WIDGET_ANIMATION_AUTHORING_HPP

#include "../foundation/motion/spec.hpp"
#include "../scene/animation_group.hpp"
#include "../scene/animation_host.hpp"
#include "../scene/scene_tree.hpp"
#include "visual_property.hpp"

#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace nandina::widget::authoring
{
    /// Composition initially accepts the three stable node presentation paths.
    template<typename Path>
    concept NodeMotionPath = visual::Path<Path>
        && std::same_as<typename std::remove_cvref_t<Path>::part_type, visual::node_t>
        && ((std::same_as<typename std::remove_cvref_t<Path>::field_type, visual::opacity_t>
             && std::same_as<property::value_t<Path>, float>)
            || ((std::same_as<typename std::remove_cvref_t<Path>::field_type, visual::translate_t>
                 || std::same_as<typename std::remove_cvref_t<Path>::field_type, visual::scale_t>)
                && std::same_as<property::value_t<Path>, foundation::NanPoint>));

    template<NodeMotionPath Path>
    class NanAnimationStep {
    public:
        NanAnimationStep(property::value_t<Path> target, motion::TweenSpec spec):
            target_(
                scene::visual::validated_target(typename Path::field_type {}, std::move(target))
            ),
            spec_(std::move(spec)) {}

        template<typename Node>
            requires std::derived_from<Node, scene::NanNode2D>
        [[nodiscard]] auto clip(Node& node) const -> scene::AnimationClip {
            auto endpoint = property::detail::visual_part(node, typename Path::part_type {})
                                .property(typename Path::field_type {});
            return endpoint.clip(target_, spec_.template behavior<property::value_t<Path>>());
        }

    private:
        property::value_t<Path> target_;
        motion::TweenSpec spec_;
    };

    template<NodeMotionPath Path, typename Value>
        requires std::convertible_to<Value, property::value_t<Path>>
    [[nodiscard]] auto to(Path, Value&& target, motion::TweenSpec spec)
        -> NanAnimationStep<std::remove_cvref_t<Path>> {
        return NanAnimationStep<std::remove_cvref_t<Path>>(
            property::value_t<Path>(std::forward<Value>(target)),
            std::move(spec)
        );
    }

    enum class NanAnimationMode { parallel, sequential, stagger };

    template<NodeMotionPath... Paths>
        requires(sizeof...(Paths) > 0)
    class NanAnimationSpec {
    public:
        NanAnimationSpec(
            NanAnimationMode mode,
            const float interval,
            NanAnimationStep<Paths>... steps
        ):
            mode_(mode),
            interval_(interval),
            steps_(std::move(steps)...) {
            if (!std::isfinite(interval) || interval < 0.0F) {
                throw std::invalid_argument(
                    "animation stagger interval must be finite and non-negative"
                );
            }
            constexpr std::array<unsigned, sizeof...(Paths)> fields {field_id<Paths>()...};
            for (std::size_t i = 0; i < fields.size(); ++i) {
                for (std::size_t j = 0; j < i; ++j) {
                    if (fields[i] == fields[j]) {
                        throw std::invalid_argument("an animation group cannot repeat a property");
                    }
                }
            }
        }

        template<typename Node>
            requires std::derived_from<Node, scene::NanNode2D>
        [[nodiscard]] auto instantiate(Node& node) const -> scene::AnimationGroup {
            auto clips = std::apply(
                [&node](const auto&... steps) {
                    return std::vector<scene::AnimationClip> {steps.clip(node)...};
                },
                steps_
            );
            switch (mode_) {
                case NanAnimationMode::parallel:
                    return scene::AnimationGroup::parallel(std::move(clips));
                case NanAnimationMode::sequential:
                    return scene::AnimationGroup::sequential(std::move(clips));
                case NanAnimationMode::stagger:
                    return scene::AnimationGroup::stagger(std::move(clips), interval_);
            }
            throw std::invalid_argument("unknown animation group mode");
        }

    private:
        template<typename Path>
        [[nodiscard]] static constexpr auto field_id() -> unsigned {
            using Field = typename Path::field_type;
            if constexpr (std::same_as<Field, visual::opacity_t>) {
                return 0;
            }
            else if constexpr (std::same_as<Field, visual::translate_t>) {
                return 1;
            }
            else {
                return 2;
            }
        }

        NanAnimationMode mode_;
        float interval_;
        std::tuple<NanAnimationStep<Paths>...> steps_;
    };

    template<NodeMotionPath... Paths>
        requires(sizeof...(Paths) > 0)
    [[nodiscard]] auto parallel(NanAnimationStep<Paths>... steps) -> NanAnimationSpec<Paths...> {
        return NanAnimationSpec<Paths...>(NanAnimationMode::parallel, 0.0F, std::move(steps)...);
    }

    template<NodeMotionPath... Paths>
        requires(sizeof...(Paths) > 0)
    [[nodiscard]] auto sequential(NanAnimationStep<Paths>... steps) -> NanAnimationSpec<Paths...> {
        return NanAnimationSpec<Paths...>(NanAnimationMode::sequential, 0.0F, std::move(steps)...);
    }

    template<NodeMotionPath... Paths>
        requires(sizeof...(Paths) > 0)
    [[nodiscard]] auto stagger(const float interval, NanAnimationStep<Paths>... steps)
        -> NanAnimationSpec<Paths...> {
        return NanAnimationSpec<Paths...>(NanAnimationMode::stagger, interval, std::move(steps)...);
    }

    /// Safe to capture by value in the owner's callback; does not retain the node.
    /// play() borrows endpoints only while the current tree hosts the resulting group.
    template<typename Node, NodeMotionPath... Paths>
        requires std::derived_from<Node, scene::NanNode2D> && (sizeof...(Paths) > 0)
    class NanAnimation {
    public:
        NanAnimation(std::weak_ptr<Node> owner, NanAnimationSpec<Paths...> spec):
            owner_(std::move(owner)),
            spec_(std::move(spec)) {}

        /// Detached or destroyed owners return false, with no deferred playback.
        /// Successful plays rebuild all clip state and use the node's current tree.
        [[nodiscard]] auto play() const -> bool {
            const auto node = owner_.lock();
            if (node == nullptr || node->get_tree() == nullptr) {
                return false;
            }
            auto group = spec_.instantiate(*node);
            node->get_tree()->animation_host().run(*node, std::move(group));
            return true;
        }

    private:
        std::weak_ptr<Node> owner_;
        NanAnimationSpec<Paths...> spec_;
    };
} // namespace nandina::widget::authoring

#endif // NANDINA_WIDGET_ANIMATION_AUTHORING_HPP
