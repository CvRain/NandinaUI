//
// widget/menu_item - 菜单条目模型与勾选规则的实现。
//

#include "menu_item.hpp"

#include <algorithm>

namespace nandina::widget
{
    namespace
    {
        /// checkbox / radio 是唯一承载勾选状态的两种 kind。
        [[nodiscard]] auto is_checkable(const MenuItemKind kind) noexcept -> bool {
            return kind == MenuItemKind::checkbox || kind == MenuItemKind::radio;
        }
    } // namespace

    auto menu_item_is_focusable(const MenuItem& item) noexcept -> bool {
        return item.kind != MenuItemKind::separator && item.kind != MenuItemKind::label;
    }

    auto menu_item_is_activatable(const MenuItem& item) noexcept -> bool {
        return menu_item_is_focusable(item) && !item.disabled;
    }

    auto menu_item_typeahead_text(const MenuItem& item) noexcept -> std::string_view {
        return menu_item_is_focusable(item) ? std::string_view(item.label) : std::string_view {};
    }

    auto find_menu_item(std::vector<MenuItem>& items, const std::string_view id) -> MenuItem* {
        const auto found = std::ranges::find_if(items, [id](const MenuItem& item) {
            return item.id == id;
        });
        return found == items.end() ? nullptr : &*found;
    }

    auto find_menu_item(const std::vector<MenuItem>& items, const std::string_view id)
        -> const MenuItem* {
        const auto found = std::ranges::find_if(items, [id](const MenuItem& item) {
            return item.id == id;
        });
        return found == items.end() ? nullptr : &*found;
    }

    auto MenuSelection::set_checked(
        std::vector<MenuItem>& items,
        const std::string_view id,
        const bool checked
    ) -> bool {
        auto* item = find_menu_item(items, id);
        if (item == nullptr || !is_checkable(item->kind)) {
            return false;
        }
        item->checked = checked;
        return true;
    }

    auto MenuSelection::toggle(std::vector<MenuItem>& items, const std::string_view id) -> bool {
        // none 模式表示这是一个纯动作菜单：不保留任何勾选语义，toggle 一律不生效。
        if (mode_ == MenuSelectionMode::none) {
            return false;
        }

        auto* item = find_menu_item(items, id);
        if (item == nullptr || !is_checkable(item->kind) || item->disabled) {
            return false;
        }

        const bool next = !item->checked;

        // single 的互斥只在 radio 之间进行：同一层可以同时存在 radio 组与独立 checkbox。
        if (mode_ == MenuSelectionMode::single && item->kind == MenuItemKind::radio && next) {
            for (auto& sibling: items) {
                if (sibling.kind == MenuItemKind::radio && sibling.id != item->id) {
                    sibling.checked = false;
                }
            }
        }

        item->checked = next;
        changed_.emit(item->id);
        return true;
    }

    auto MenuSelection::is_checked(const std::vector<MenuItem>& items, const std::string_view id)
        -> bool {
        const auto* item = find_menu_item(items, id);
        return item != nullptr && item->checked;
    }

    auto MenuSelection::checked_ids(const std::vector<MenuItem>& items)
        -> std::vector<std::string> {
        std::vector<std::string> result;
        for (const auto& item: items) {
            if (is_checkable(item.kind) && item.checked) {
                result.push_back(item.id);
            }
        }
        return result;
    }

} // namespace nandina::widget
