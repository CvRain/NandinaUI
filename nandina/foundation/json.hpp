//
// foundation/json — thin wrapper over nlohmann/json.
//
// The project does not carry its own JSON parser: nlohmann/json is vendored as a git
// submodule (`subprojects/nlohmann_json`) and exposed here through a minimal,
// exception-to-`std::expected` bridge so callers can read JSON without spelling the
// third-party type at every call site. Only parsing + error mapping is wrapped;
// callers keep the full `nlohmann::json` value for navigation.
//

#ifndef NANDINA_EXPERIMENT_FOUNDATION_JSON_HPP
#define NANDINA_EXPERIMENT_FOUNDATION_JSON_HPP

#include <nlohmann/json.hpp>

#include <expected>
#include <string>
#include <string_view>

namespace nandina::foundation
{
    /// Parse a complete JSON document. Syntax errors are returned as `std::expected`
    /// errors (with the library's byte-positioned message) instead of throwing.
    [[nodiscard]] inline auto parse_json(std::string_view text)
        -> std::expected<nlohmann::json, std::string> {
        try {
            return nlohmann::json::parse(text);
        }
        catch (const nlohmann::json::parse_error& error) {
            return std::unexpected(std::string(error.what()));
        }
    }
} // namespace nandina::foundation

#endif // NANDINA_EXPERIMENT_FOUNDATION_JSON_HPP
