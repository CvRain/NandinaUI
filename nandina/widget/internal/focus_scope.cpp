#include "focus_scope.hpp"

#include "../key_codes.hpp"

#include "../../scene/input_event.hpp"
#include "../../scene/scene_tree.hpp"

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace nandina::widget::internal
{
    namespace
    {

        /// 内容子树里是否存在可见且可聚焦的控件：作用域据此决定是否用自身兜底焦点。
        [[nodiscard]] auto has_focusable_descendant(const scene::NanNode& node) -> bool {
            for (std::size_t index = 0; index < node.child_count(); ++index) {
                const auto* child = node.get_child(index);
                if (child == nullptr) {
                    continue;
                }
                if (const auto* node_2d = child->as_node2d();
                    node_2d != nullptr && node_2d->is_visible_in_tree() && node_2d->is_focusable())
                {
                    return true;
                }
                if (has_focusable_descendant(*child)) {
                    return true;
                }
            }
            return false;
        }
    } // namespace

    auto FocusScope::set_content(std::shared_ptr<scene::NanControl> content)
        -> scene::NanControl& {
        if (!content) {
            throw std::invalid_argument("FocusScope::set_content: content is null");
        }
        if (content->parent() != nullptr) {
            throw std::logic_error("FocusScope::set_content: content must be detached");
        }
        if (auto current = content_.lock()) {
            remove_and_delete(*current);
        }
        auto* result = content.get();
        content_ = content;
        add_child(std::move(content));
        return *result;
    }

    auto FocusScope::content() const -> scene::NanControl* {
        return content_.lock().get();
    }

    auto FocusScope::on_input_capture(scene::InputEvent& event) -> bool {
        if (event.type() != scene::EventType::key) {
            return false;
        }
        auto& key = static_cast<scene::KeyEvent&>(event);
        if (!key.is_pressed() || key.keycode() != keys::tab || get_tree() == nullptr) {
            return false;
        }
        const bool moved = key.modifiers().shift
            ? get_tree()->focus_previous_within(*this)
            : get_tree()->focus_next_within(*this);
        if (moved) {
            event.accept();
        }
        return moved;
    }

    auto FocusScope::is_focusable() const -> bool {
        const auto content = content_.lock();
        return content == nullptr || !has_focusable_descendant(*content);
    }

    void FocusScope::on_ready() {
        auto* tree = get_tree();
        if (tree == nullptr) {
            return;
        }
        if (auto* focused = tree->focused_node(); focused != nullptr && focused != this
            && !is_ancestor_of(*focused)) {
            previous_focus_ = focused->weak_from_this();
        }
        (void)tree->focus_first_within(*this);
    }

    void FocusScope::on_exit_tree() {
        auto* tree = get_tree();
        auto previous = previous_focus_.lock();
        auto* target = previous != nullptr ? previous->as_node2d() : nullptr;
        if (tree != nullptr && target != nullptr) {
            tree->set_focus(target);
        }
        previous_focus_.reset();
    }

    auto FocusScope::on_measure(const scene::LayoutConstraints constraints)
        -> foundation::NanSize {
        auto current = content_.lock();
        return current != nullptr ? current->measure_layout(constraints)
                                  : constraints.constrain(foundation::NanSize::zero());
    }

    void FocusScope::on_layout() {
        if (auto current = content_.lock()) {
            current->layout_to(local_rect());
        }
    }
}
