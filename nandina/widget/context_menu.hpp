//
// widget/context_menu - right-click menu composed from DropdownMenu.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_CONTEXT_MENU_HPP
#define NANDINA_EXPERIMENT_WIDGET_CONTEXT_MENU_HPP

#include "dropdown_menu.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::scene
{
    class OverlayHost;
} // namespace nandina::scene

namespace nandina::widget
{
    template<typename Component>
    struct ComponentTraits;

    /**
     * Wraps one target control and opens a DropdownMenu at the pointer position.
     *
     * ContextMenu owns no parallel menu implementation: items, selection, recursive
     * submenus, keyboard navigation, dismissal and theming all remain DropdownMenu
     * behavior. This wrapper only supplies the target slot and invocation policy.
     */
    class ContextMenu: public scene::NanControl {
    public:
        explicit ContextMenu(
            std::shared_ptr<scene::NanControl> target = nullptr,
            std::vector<MenuItem> items = {},
            theme::NanTheme theme = theme::default_theme()
        );
        ~ContextMenu() override;

        [[nodiscard]] static auto create(
            std::shared_ptr<scene::NanControl> target = nullptr,
            std::vector<MenuItem> items = {},
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<ContextMenu>;

        auto set_target(std::shared_ptr<scene::NanControl> target) -> ContextMenu&;
        [[nodiscard]] auto target() const -> std::shared_ptr<scene::NanControl>;

        void set_items(std::vector<MenuItem> items);
        [[nodiscard]] auto items() const -> const std::vector<MenuItem>&;
        [[nodiscard]] auto item_count() const -> std::size_t;

        void set_selection_mode(MenuSelectionMode mode);
        [[nodiscard]] auto selection_mode() const -> MenuSelectionMode;
        [[nodiscard]] auto checked_ids() const -> std::vector<std::string>;
        auto set_checked(std::string_view id, bool checked) -> bool;

        /// Open at a screen-space pointer position. No target means a safe no-op.
        void open_at(foundation::NanPoint screen_position);
        void close();
        [[nodiscard]] auto is_open() const -> bool;
        [[nodiscard]] auto active_index() const -> int;

        void set_on_select(std::function<void(std::string_view id)> callback);
        [[nodiscard]] auto item_selected() const -> const reactive::Event<std::string>&;
        void set_on_submenu(std::function<void(std::string_view id)> callback);
        void set_on_close(std::function<void()> callback);

        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        void set_override(theme::DropdownMenuRecipeRule rule);
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedDropdownMenuStyle;

        auto on_input_capture(scene::InputEvent& event) -> bool override;
        [[nodiscard]] auto z_index_hint() const -> int override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_layout() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;

    private:
        friend struct ComponentTraits<ContextMenu>;

        void set_overlay_service(scene::OverlayHost* host) noexcept;
        void open_from_keyboard();

        std::weak_ptr<scene::NanControl> target_;
        std::shared_ptr<DropdownMenu> menu_;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_CONTEXT_MENU_HPP
