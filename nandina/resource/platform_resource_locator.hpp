#ifndef NANDINA_EXPERIMENT_RESOURCE_PLATFORM_RESOURCE_LOCATOR_HPP
#define NANDINA_EXPERIMENT_RESOURCE_PLATFORM_RESOURCE_LOCATOR_HPP

#include <expected>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::resource
{
    enum class ResourceLocationKind { executable_relative, user_data, system_data };

    struct ResourceLocation {
        ResourceLocationKind kind;
        std::filesystem::path root;
        auto operator<=>(const ResourceLocation&) const = default;
    };

    struct PlatformResourceLocatorOptions {
        std::string application_id;
        std::filesystem::path executable_path;
        std::map<std::string, std::string, std::less<>> environment;
    };

    class PlatformResourceLocator {
    public:
        [[nodiscard]] static auto create(PlatformResourceLocatorOptions options)
            -> std::expected<PlatformResourceLocator, std::string>;

        [[nodiscard]] auto application_id() const noexcept -> std::string_view {
            return application_id_;
        }
        [[nodiscard]] auto resource_roots() const -> std::vector<ResourceLocation>;
        [[nodiscard]] auto config_root() const -> std::filesystem::path;
        [[nodiscard]] auto data_root() const -> std::filesystem::path;
        [[nodiscard]] auto state_root() const -> std::filesystem::path;
        [[nodiscard]] auto cache_root() const -> std::filesystem::path;

    private:
        explicit PlatformResourceLocator(PlatformResourceLocatorOptions options);
        [[nodiscard]] auto environment(std::string_view name) const -> std::string_view;

        std::string application_id_;
        std::filesystem::path executable_path_;
        std::map<std::string, std::string, std::less<>> environment_;
    };
} // namespace nandina::resource

#endif
