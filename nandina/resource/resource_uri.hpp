#ifndef NANDINA_EXPERIMENT_RESOURCE_RESOURCE_URI_HPP
#define NANDINA_EXPERIMENT_RESOURCE_RESOURCE_URI_HPP

#include "resource.hpp"

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::resource
{
    enum class ResourceUriScheme { res, builtin, user, cache, file };

    enum class ResourceUriErrorCode {
        missing_scheme,
        unknown_scheme,
        empty_path,
        invalid_path,
        invalid_resource_key,
    };

    struct ResourceUriError {
        ResourceUriErrorCode code;
        std::string message;
    };

    class ResourceUri {
    public:
        [[nodiscard]] static auto parse(std::string_view value)
            -> std::expected<ResourceUri, ResourceUriError>;

        [[nodiscard]] auto scheme() const noexcept -> ResourceUriScheme {
            return scheme_;
        }
        [[nodiscard]] auto path() const noexcept -> std::string_view {
            return path_;
        }
        [[nodiscard]] auto resource_key() const -> std::optional<ResourceKey>;
        [[nodiscard]] auto file_path() const -> std::optional<std::filesystem::path>;
        [[nodiscard]] auto to_string() const -> std::string;
        auto operator<=>(const ResourceUri&) const = default;

    private:
        ResourceUri(ResourceUriScheme scheme, std::string path):
            scheme_(scheme),
            path_(std::move(path)) {}

        ResourceUriScheme scheme_;
        std::string path_;
    };
} // namespace nandina::resource

#endif
