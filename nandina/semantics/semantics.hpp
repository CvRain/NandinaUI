//
// semantics/semantics - platform-independent accessibility tree contract.
//

#ifndef NANDINA_EXPERIMENT_SEMANTICS_SEMANTICS_HPP
#define NANDINA_EXPERIMENT_SEMANTICS_SEMANTICS_HPP

#include "../foundation/geometry.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::semantics
{

    using SemanticsId = std::uint64_t;

    enum class Role : std::uint8_t {
        none,
        generic,
        button,
        static_text,
        text_field,
        list,
        list_item,
        checkbox,
        slider,
        switch_control,
        progress_bar,
        radio,
        tab,
        tooltip,
        combobox,
        separator,
        dialog,
    };

    enum class Composition : std::uint8_t {
        automatic,
        expose,
        merge_descendants,
        hidden,
    };

    enum class Action : std::uint16_t {
        none = 0,
        activate = 1U << 0U,
        focus = 1U << 1U,
        set_value = 1U << 2U,
        increment = 1U << 3U,
        decrement = 1U << 4U,
    };

    [[nodiscard]] constexpr auto operator|(Action lhs, Action rhs) -> Action {
        return static_cast<Action>(
            static_cast<std::uint16_t>(lhs) | static_cast<std::uint16_t>(rhs)
        );
    }

    constexpr auto operator|=(Action& lhs, Action rhs) -> Action& {
        lhs = lhs | rhs;
        return lhs;
    }

    [[nodiscard]] constexpr auto supports(Action actions, Action action) -> bool {
        return (static_cast<std::uint16_t>(actions) & static_cast<std::uint16_t>(action)) != 0;
    }

    struct State {
        bool focusable = false;
        bool focused = false;
        bool disabled = false;
        bool read_only = false;
        bool invalid = false;
        std::optional<bool> checked;
        std::optional<bool> selected;
    };

    struct Properties {
        Role role = Role::none;
        std::string label;
        std::string value;
        std::string hint;
        State state;
        Action actions = Action::none;
    };

    struct ActionRequest {
        Action action = Action::none;
        std::string value;
    };

    struct Node {
        SemanticsId id = 0;
        Properties properties;
        foundation::NanRect bounds;
        std::vector<Node> children;

        [[nodiscard]] auto find(SemanticsId target) const noexcept -> const Node*;
    };

    struct Tree {
        std::uint64_t revision = 0;
        std::vector<Node> roots;

        [[nodiscard]] auto find(SemanticsId target) const noexcept -> const Node*;
        [[nodiscard]] auto empty() const noexcept -> bool;
    };

    [[nodiscard]] auto role_name(Role role) noexcept -> std::string_view;

} // namespace nandina::semantics

#endif // NANDINA_EXPERIMENT_SEMANTICS_SEMANTICS_HPP
