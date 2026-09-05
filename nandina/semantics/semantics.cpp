//
// semantics/semantics - platform-independent accessibility tree contract.
//

#include "semantics.hpp"

namespace nandina::semantics
{

    auto Node::find(const SemanticsId target) const noexcept -> const Node* {
        if (id == target) {
            return this;
        }
        for (const auto& child: children) {
            if (const auto* found = child.find(target); found != nullptr) {
                return found;
            }
        }
        return nullptr;
    }

    auto Tree::find(const SemanticsId target) const noexcept -> const Node* {
        for (const auto& root: roots) {
            if (const auto* found = root.find(target); found != nullptr) {
                return found;
            }
        }
        return nullptr;
    }

    auto Tree::empty() const noexcept -> bool {
        return roots.empty();
    }

    auto role_name(const Role role) noexcept -> std::string_view {
        switch (role) {
            case Role::none:
                return "none";
            case Role::generic:
                return "generic";
            case Role::button:
                return "button";
            case Role::static_text:
                return "static-text";
            case Role::text_field:
                return "text-field";
            case Role::list:
                return "list";
            case Role::list_item:
                return "list-item";
            case Role::checkbox:
                return "checkbox";
            case Role::slider:
                return "slider";
            case Role::switch_control:
                return "switch";
            case Role::progress_bar:
                return "progress-bar";
            case Role::radio:
                return "radio";
            case Role::tab:
                return "tab";
            case Role::tooltip:
                return "tooltip";
            case Role::combobox:
                return "combobox";
            case Role::separator:
                return "separator";
            case Role::dialog:
                return "dialog";
        }
        return "none";
    }

} // namespace nandina::semantics
