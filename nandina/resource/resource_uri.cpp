#include "resource_uri.hpp"

#include <array>

namespace nandina::resource
{
    namespace
    {
        struct SchemeName {
            std::string_view name;
            ResourceUriScheme scheme;
        };
        constexpr std::array schemes {
            SchemeName {"res", ResourceUriScheme::res},
            SchemeName {"builtin", ResourceUriScheme::builtin},
            SchemeName {"user", ResourceUriScheme::user},
            SchemeName {"cache", ResourceUriScheme::cache},
            SchemeName {"file", ResourceUriScheme::file},
        };

        [[nodiscard]] auto scheme_name(ResourceUriScheme scheme) -> std::string_view {
            for (const auto& candidate: schemes) {
                if (candidate.scheme == scheme) {
                    return candidate.name;
                }
            }
            return {};
        }

        [[nodiscard]] auto invalid(ResourceUriErrorCode code, std::string message)
            -> std::unexpected<ResourceUriError> {
            return std::unexpected(ResourceUriError {code, std::move(message)});
        }
    } // namespace

    auto ResourceUri::parse(const std::string_view value)
        -> std::expected<ResourceUri, ResourceUriError> {
        const auto separator = value.find("://");
        if (separator == std::string_view::npos) {
            return invalid(ResourceUriErrorCode::missing_scheme, "resource URI has no scheme");
        }

        std::optional<ResourceUriScheme> scheme;
        const auto scheme_text = value.substr(0, separator);
        for (const auto& candidate: schemes) {
            if (candidate.name == scheme_text) {
                scheme = candidate.scheme;
                break;
            }
        }
        if (!scheme) {
            return invalid(ResourceUriErrorCode::unknown_scheme, "resource URI scheme is unknown");
        }

        auto path = value.substr(separator + 3);
        if (path.empty()) {
            return invalid(ResourceUriErrorCode::empty_path, "resource URI path is empty");
        }
        if (path.contains('?') || path.contains('#') || path.contains('\\')
            || path.find('\0') != std::string_view::npos)
        {
            return invalid(ResourceUriErrorCode::invalid_path, "resource URI path is invalid");
        }

        if (*scheme == ResourceUriScheme::file) {
            if (!path.starts_with('/')) {
                return invalid(
                    ResourceUriErrorCode::invalid_path,
                    "file URI requires an absolute path"
                );
            }
            return ResourceUri(*scheme, std::string(path));
        }

        if (path.starts_with('/') || !ResourceKey::parse(path)) {
            return invalid(
                ResourceUriErrorCode::invalid_resource_key,
                "resource URI path is not a canonical resource key"
            );
        }
        return ResourceUri(*scheme, std::string(path));
    }

    auto ResourceUri::resource_key() const -> std::optional<ResourceKey> {
        return scheme_ == ResourceUriScheme::file ? std::nullopt : ResourceKey::parse(path_);
    }

    auto ResourceUri::file_path() const -> std::optional<std::filesystem::path> {
        return scheme_ == ResourceUriScheme::file
            ? std::optional<std::filesystem::path>(std::filesystem::path(path_))
            : std::nullopt;
    }

    auto ResourceUri::to_string() const -> std::string {
        return std::string(scheme_name(scheme_)) + "://" + path_;
    }
} // namespace nandina::resource
