#pragma once

#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::app::detail
{
    [[nodiscard]] auto is_wayland_session(const char* wayland_display) noexcept -> bool;

    [[nodiscard]] auto read_command_output(std::initializer_list<std::string_view> command)
        -> std::optional<std::string>;
    auto write_command_input(std::initializer_list<std::string_view> command, std::string_view text)
        -> bool;

    [[nodiscard]] auto read_wayland_clipboard() -> std::optional<std::string>;
    auto write_wayland_clipboard(std::string_view text) -> bool;
} // namespace nandina::app::detail
