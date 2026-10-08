// scene/anchors — layout relations, without owning their target nodes.
#ifndef NANDINA_SCENE_ANCHORS_HPP
#define NANDINA_SCENE_ANCHORS_HPP

#include "node.hpp"

#include <array>
#include <cmath>
#include <optional>
#include <stdexcept>

namespace nandina::scene
{
    namespace detail
    {
        struct NodeReferenceState {
            std::weak_ptr<NanNode> node;
            bool bound = false;

            [[nodiscard]] auto require() const -> std::shared_ptr<NanNode> {
                if (!bound) {
                    throw std::logic_error("anchor target is unbound");
                }
                auto result = node.lock();
                if (!result) {
                    throw std::logic_error("anchor target is expired");
                }
                return result;
            }
        };
    } // namespace detail

    enum class AnchorLine { left, right, top, bottom, horizontal_center, vertical_center };

    struct AnchorTarget {
        std::shared_ptr<const detail::NodeReferenceState> identity;
        AnchorLine line = AnchorLine::left;
        bool parent = false;
        float offset = 0.0F;

        [[nodiscard]] auto operator+(float delta) const -> AnchorTarget {
            auto result = *this;
            result.offset += delta;
            return result;
        }
        [[nodiscard]] auto operator-(float delta) const -> AnchorTarget {
            return *this + -delta;
        }
    };

    struct AnchorLines {
        const AnchorTarget left;
        const AnchorTarget right;
        const AnchorTarget top;
        const AnchorTarget bottom;
        const AnchorTarget horizontal_center;
        const AnchorTarget vertical_center;

        explicit AnchorLines(
            std::shared_ptr<const detail::NodeReferenceState> identity,
            bool parent = false
        ):
            left {identity, AnchorLine::left, parent},
            right {identity, AnchorLine::right, parent},
            top {identity, AnchorLine::top, parent},
            bottom {identity, AnchorLine::bottom, parent},
            horizontal_center {identity, AnchorLine::horizontal_center, parent},
            vertical_center {identity, AnchorLine::vertical_center, parent} {}
    };

    struct AnchorSpec {
        std::optional<AnchorTarget> left;
        std::optional<AnchorTarget> right;
        std::optional<AnchorTarget> top;
        std::optional<AnchorTarget> bottom;
        std::optional<AnchorTarget> horizontal_center;
        std::optional<AnchorTarget> vertical_center;

        [[nodiscard]] auto targets() const -> std::array<std::optional<AnchorTarget>, 6> {
            return {left, right, top, bottom, horizontal_center, vertical_center};
        }
        [[nodiscard]] auto empty() const -> bool {
            return !left && !right && !top && !bottom && !horizontal_center && !vertical_center;
        }
        void validate() const {
            if ((horizontal_center && (left || right)) || (vertical_center && (top || bottom))) {
                throw std::invalid_argument("anchors: center cannot be combined with edges");
            }
            const auto entries = targets();
            for (std::size_t i = 0; i < entries.size(); ++i) {
                if (!entries[i]) {
                    continue;
                }
                const auto& target = *entries[i];
                const auto line = static_cast<unsigned>(target.line);
                if (!target.identity || line > static_cast<unsigned>(AnchorLine::vertical_center)
                    || !std::isfinite(target.offset))
                {
                    throw std::invalid_argument("anchors: invalid target or non-finite offset");
                }
                const bool horizontal = target.line == AnchorLine::left
                    || target.line == AnchorLine::right
                    || target.line == AnchorLine::horizontal_center;
                if (horizontal != (i == 0 || i == 1 || i == 4)) {
                    throw std::invalid_argument(
                        "anchors: horizontal and vertical lines cannot be mixed"
                    );
                }
            }
        }
    };
} // namespace nandina::scene
#endif
