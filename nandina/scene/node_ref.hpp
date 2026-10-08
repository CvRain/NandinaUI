// scene/node_ref — typed, single-binding, non-owning node identity.
#ifndef NANDINA_SCENE_NODE_REF_HPP
#define NANDINA_SCENE_NODE_REF_HPP

#include "anchors.hpp"

#include <concepts>

namespace nandina::scene
{
    template<class T>
        requires std::derived_from<T, NanNode>
    class NodeRef {
        std::shared_ptr<detail::NodeReferenceState> identity_ =
            std::make_shared<detail::NodeReferenceState>();

    public:
        const AnchorLines anchor {identity_};
        struct Parent {
            const AnchorLines anchor;
        };
        const Parent parent {AnchorLines(identity_, true)};

        NodeRef() = default;
        explicit NodeRef(const std::shared_ptr<T>& node) {
            bind(node);
        }
        // Copies alias the slot, including expressions created before binding.
        NodeRef(const NodeRef&) = default;
        auto operator=(const NodeRef&) -> NodeRef& = delete;

        void bind(const std::shared_ptr<T>& node) const {
            if (!node) {
                throw std::invalid_argument("NodeRef: cannot bind null");
            }
            if (identity_->bound) {
                throw std::logic_error("NodeRef: identity is already bound");
            }
            identity_->node = node;
            identity_->bound = true;
        }

        [[nodiscard]] auto lock() const -> std::shared_ptr<T> {
            return std::static_pointer_cast<T>(identity_->require());
        }
    };
} // namespace nandina::scene
#endif
