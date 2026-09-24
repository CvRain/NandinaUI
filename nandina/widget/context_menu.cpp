//
// widget/context_menu - right-click menu composed from DropdownMenu.
//

#include "context_menu.hpp"

#include "key_codes.hpp"

#include "../scene/input_event.hpp"

#include <stdexcept>
#include <utility>

namespace nandina::widget
{
    ContextMenu::ContextMenu(
        std::shared_ptr<scene::NanControl> target,
        std::vector<MenuItem> items,
        theme::NanTheme theme
    ):
        menu_(DropdownMenu::create(nullptr, std::move(items), std::move(theme))) {
        menu_->set_gap(2.0F);
        add_child(menu_);
        if (target != nullptr) {
            set_target(std::move(target));
        }
    }

    ContextMenu::~ContextMenu() = default;

    auto ContextMenu::create(
        std::shared_ptr<scene::NanControl> target,
        std::vector<MenuItem> items,
        theme::NanTheme theme
    ) -> std::shared_ptr<ContextMenu> {
        return std::make_shared<ContextMenu>(
            std::move(target), std::move(items), std::move(theme)
        );
    }

    auto ContextMenu::set_target(std::shared_ptr<scene::NanControl> target) -> ContextMenu& {
        if (target == nullptr) {
            throw std::invalid_argument("ContextMenu::set_target: target is null");
        }
        menu_->close();
        auto current = target_.lock();
        target_ = target;
        replace_child(current.get(), std::move(target));
        mark_layout_dirty();
        return *this;
    }

    auto ContextMenu::target() const -> std::shared_ptr<scene::NanControl> {
        return target_.lock();
    }

    void ContextMenu::set_items(std::vector<MenuItem> items) {
        menu_->set_items(std::move(items));
    }

    auto ContextMenu::items() const -> const std::vector<MenuItem>& {
        return menu_->items();
    }

    auto ContextMenu::item_count() const -> std::size_t {
        return menu_->item_count();
    }

    void ContextMenu::set_selection_mode(const MenuSelectionMode mode) {
        menu_->set_selection_mode(mode);
    }

    auto ContextMenu::selection_mode() const -> MenuSelectionMode {
        return menu_->selection_mode();
    }

    auto ContextMenu::checked_ids() const -> std::vector<std::string> {
        return menu_->checked_ids();
    }

    auto ContextMenu::set_checked(const std::string_view id, const bool checked) -> bool {
        return menu_->set_checked(id, checked);
    }

    void ContextMenu::open_at(const foundation::NanPoint screen_position) {
        const auto owner = target_.lock();
        if (owner == nullptr) {
            return;
        }
        menu_->set_external_anchor_rect(
            owner,
            foundation::NanRect::from_xywh(
                screen_position.get_x(), screen_position.get_y(), 1.0F, 1.0F
            )
        );
        menu_->open();
        mark_semantics_dirty();
    }

    void ContextMenu::close() {
        menu_->close();
        mark_semantics_dirty();
    }

    auto ContextMenu::is_open() const -> bool {
        return menu_->is_open();
    }

    auto ContextMenu::active_index() const -> int {
        return menu_->active_index();
    }

    void ContextMenu::set_on_select(std::function<void(std::string_view)> callback) {
        menu_->set_on_select(std::move(callback));
    }

    auto ContextMenu::item_selected() const -> const reactive::Event<std::string>& {
        return menu_->item_selected();
    }

    void ContextMenu::set_on_submenu(std::function<void(std::string_view)> callback) {
        menu_->set_on_submenu(std::move(callback));
    }

    void ContextMenu::set_on_close(std::function<void()> callback) {
        menu_->set_on_close(std::move(callback));
    }

    void ContextMenu::set_theme(theme::NanTheme theme) {
        menu_->set_theme(std::move(theme));
    }

    auto ContextMenu::theme_ref() const -> const theme::NanTheme& {
        return menu_->theme_ref();
    }

    void ContextMenu::set_override(theme::DropdownMenuRecipeRule rule) {
        menu_->set_override(std::move(rule));
    }

    auto ContextMenu::resolved_style() const -> theme::ResolvedDropdownMenuStyle {
        return menu_->resolved_style();
    }

    auto ContextMenu::on_input_capture(scene::InputEvent& event) -> bool {
        if (event.type() == scene::EventType::mouse_button) {
            auto& pointer = static_cast<scene::MouseButtonEvent&>(event);
            if (pointer.is_pressed()
                && pointer.button() == scene::MouseButtonEvent::Button::right)
            {
                open_at(pointer.screen_pos());
                event.accept();
                return true;
            }
            return false;
        }
        if (event.type() == scene::EventType::key) {
            auto& key = static_cast<scene::KeyEvent&>(event);
            if (key.is_pressed()
                && (key.keycode() == keys::menu
                    || (key.keycode() == keys::f10 && key.modifiers().shift)))
            {
                open_from_keyboard();
                event.accept();
                return true;
            }
        }
        return false;
    }

    auto ContextMenu::z_index_hint() const -> int {
        return menu_->z_index_hint();
    }

    auto ContextMenu::on_measure(const scene::LayoutConstraints constraints)
        -> foundation::NanSize {
        const auto current = target_.lock();
        return current != nullptr ? constraints.constrain(current->measure_layout(constraints))
                                  : constraints.constrain(foundation::NanSize::zero());
    }

    void ContextMenu::on_layout() {
        if (const auto current = target_.lock(); current != nullptr) {
            current->layout_to(local_rect());
        }
        menu_->layout_to(foundation::NanRect::empty());
    }

    auto ContextMenu::semantics_properties() const -> semantics::Properties {
        return {
            .role = semantics::Role::generic,
            .value = menu_->is_open() ? "expanded" : "collapsed",
            .state = {.checked = menu_->is_open()},
        };
    }

    void ContextMenu::set_overlay_service(scene::OverlayHost* host) noexcept {
        menu_->set_overlay_service(host);
    }

    void ContextMenu::open_from_keyboard() {
        const auto owner = target_.lock();
        if (owner == nullptr) {
            return;
        }
        const auto bounds = owner->global_bounds();
        if (!bounds.is_valid()) {
            return;
        }
        open_at(bounds.get_bottom_left());
    }
} // namespace nandina::widget
