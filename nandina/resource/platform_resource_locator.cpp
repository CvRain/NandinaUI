#include "platform_resource_locator.hpp"

#include "resource.hpp"

#include <algorithm>

namespace nandina::resource
{
    namespace
    {
        [[nodiscard]] auto split_paths(std::string_view value)
            -> std::vector<std::filesystem::path> {
            std::vector<std::filesystem::path> result;
            while (!value.empty()) {
                const auto separator = value.find(':');
                const auto item = value.substr(0, separator);
                if (!item.empty()) {
                    result.emplace_back(item);
                }
                if (separator == std::string_view::npos) {
                    break;
                }
                value.remove_prefix(separator + 1);
            }
            return result;
        }

        void append_unique(
            std::vector<ResourceLocation>& locations,
            ResourceLocationKind kind,
            std::filesystem::path root
        ) {
            root = root.lexically_normal();
            if (std::ranges::none_of(
                    locations,
                    [&](const auto& location) { return location.root == root; }
                ))
            {
                locations.push_back({kind, std::move(root)});
            }
        }

        [[nodiscard]] auto is_absolute_path_list(std::string_view value) -> bool {
            const auto paths = split_paths(value);
            return std::ranges::all_of(paths, [](const auto& path) { return path.is_absolute(); });
        }
    } // namespace

    PlatformResourceLocator::PlatformResourceLocator(PlatformResourceLocatorOptions options):
        application_id_(std::move(options.application_id)),
        executable_path_(std::move(options.executable_path)),
        environment_(std::move(options.environment)) {}

    auto PlatformResourceLocator::create(PlatformResourceLocatorOptions options)
        -> std::expected<PlatformResourceLocator, std::string> {
        if (!ResourceKey::parse(options.application_id) || options.application_id.contains('/')) {
            return std::unexpected(
                "application id must be a canonical single resource-key segment"
            );
        }
        if (options.executable_path.empty() || !options.executable_path.is_absolute()) {
            return std::unexpected("executable path must be absolute");
        }
        const auto home = options.environment.find("HOME");
        if (home == options.environment.end() || !std::filesystem::path(home->second).is_absolute())
        {
            return std::unexpected("HOME must be an absolute path");
        }
        for (const auto* name:
             {"XDG_CONFIG_HOME", "XDG_DATA_HOME", "XDG_STATE_HOME", "XDG_CACHE_HOME"})
        {
            const auto found = options.environment.find(name);
            if (found != options.environment.end() && !found->second.empty()
                && !std::filesystem::path(found->second).is_absolute())
            {
                return std::unexpected(std::string(name) + " must be an absolute path");
            }
        }
        if (const auto found = options.environment.find("XDG_DATA_DIRS");
            found != options.environment.end() && !found->second.empty()
            && !is_absolute_path_list(found->second))
        {
            return std::unexpected("XDG_DATA_DIRS must contain only absolute paths");
        }
        return PlatformResourceLocator(std::move(options));
    }

    auto PlatformResourceLocator::environment(const std::string_view name) const
        -> std::string_view {
        const auto found = environment_.find(name);
        return found == environment_.end() ? std::string_view {} : std::string_view(found->second);
    }

    auto PlatformResourceLocator::config_root() const -> std::filesystem::path {
        if (const auto xdg = environment("XDG_CONFIG_HOME"); !xdg.empty()) {
            return std::filesystem::path(xdg) / application_id_;
        }
        return std::filesystem::path(environment("HOME")) / ".config" / application_id_;
    }

    auto PlatformResourceLocator::data_root() const -> std::filesystem::path {
        if (const auto xdg = environment("XDG_DATA_HOME"); !xdg.empty()) {
            return std::filesystem::path(xdg) / application_id_;
        }
        return std::filesystem::path(environment("HOME")) / ".local/share" / application_id_;
    }

    auto PlatformResourceLocator::state_root() const -> std::filesystem::path {
        if (const auto xdg = environment("XDG_STATE_HOME"); !xdg.empty()) {
            return std::filesystem::path(xdg) / application_id_;
        }
        return std::filesystem::path(environment("HOME")) / ".local/state" / application_id_;
    }

    auto PlatformResourceLocator::cache_root() const -> std::filesystem::path {
        if (const auto xdg = environment("XDG_CACHE_HOME"); !xdg.empty()) {
            return std::filesystem::path(xdg) / application_id_;
        }
        return std::filesystem::path(environment("HOME")) / ".cache" / application_id_;
    }

    auto PlatformResourceLocator::resource_roots() const -> std::vector<ResourceLocation> {
        std::vector<ResourceLocation> result;
        append_unique(
            result,
            ResourceLocationKind::executable_relative,
            executable_path_.parent_path() / "resources"
        );
        append_unique(result, ResourceLocationKind::user_data, data_root());

        auto system_roots = split_paths(environment("XDG_DATA_DIRS"));
        if (system_roots.empty()) {
            system_roots = {"/usr/local/share", "/usr/share"};
        }
        for (auto& root: system_roots) {
            append_unique(
                result,
                ResourceLocationKind::system_data,
                std::move(root) / application_id_
            );
        }
        append_unique(
            result,
            ResourceLocationKind::system_data,
            std::filesystem::path("/usr/local/share") / application_id_
        );
        append_unique(
            result,
            ResourceLocationKind::system_data,
            std::filesystem::path("/usr/share") / application_id_
        );
        return result;
    }
} // namespace nandina::resource
