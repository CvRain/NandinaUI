#include "focus_scope.hpp"

#include "../../scene/input_event.hpp"
#include "../../scene/scene_tree.hpp"

#include <stdexcept>
#include <utility>

namespace nandina::widget::internal
{
    namespace
    {
        constexpr int key_tab = 258;
    }

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
        if (!key.is_pressed() || key.keycode() != key_tab || get_tree() == nullptr) {
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
