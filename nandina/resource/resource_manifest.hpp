#ifndef NANDINA_EXPERIMENT_RESOURCE_RESOURCE_MANIFEST_HPP
#define NANDINA_EXPERIMENT_RESOURCE_RESOURCE_MANIFEST_HPP

#include "resource_scanner.hpp"

#include <expected>
#include <map>

namespace nandina::resource
{
    enum class ManifestStorage { automatic, embedded, external };

    struct ResourcePolicyRule {
        std::string glob;
        std::optional<std::string> media_type;
        std::optional<ManifestStorage> storage;
        std::optional<bool> streaming;
    };

    /// 针对单个资源 key 的显式覆盖（`[[resources]]`），优先级高于 glob 规则。
    struct ResourceOverride {
        std::optional<std::string> media_type;
        std::optional<ManifestStorage> storage;
        std::optional<bool> streaming;
    };

    struct ResourcePolicy {
        std::string package_id;
        std::filesystem::path base_directory;
        std::vector<ResourceScanRoot> roots;
        std::vector<std::string> excludes;
        std::vector<ResourcePolicyRule> rules;
        std::map<ResourceKey, ResourceOverride> overrides;
        std::map<ResourceKey, ResourceKey> aliases;
        bool include_hidden = false;
        bool allow_symlinks = false;
        std::filesystem::path package_directory = "package";
        std::uint64_t embed_threshold = 1024U * 1024U;
    };

    struct LockedResource {
        ResourceId id;
        ResourceKey key;
        std::filesystem::path source;
        std::string media_type;
        std::uint64_t size = 0;
        std::string sha256;
        ManifestStorage storage = ManifestStorage::automatic;
        bool streaming = false;
        std::uint64_t revision = 1;
    };

    struct ResourceLockManifest {
        std::string package_id;
        std::vector<LockedResource> resources;
    };

    [[nodiscard]] auto load_resource_policy(const std::filesystem::path& path)
        -> std::expected<ResourcePolicy, std::string>;
    [[nodiscard]] auto load_resource_lock(const std::filesystem::path& path)
        -> std::expected<ResourceLockManifest, std::string>;
    [[nodiscard]] auto build_resource_lock(
        const ResourcePolicy& policy,
        const ResourceScanResult& scan,
        const std::optional<ResourceLockManifest>& previous
    ) -> std::expected<ResourceLockManifest, std::string>;
    [[nodiscard]] auto write_resource_lock_atomic(
        const ResourceLockManifest& manifest,
        const std::filesystem::path& path
    ) -> std::expected<void, std::string>;
    [[nodiscard]] auto sha256_file(const std::filesystem::path& path)
        -> std::expected<std::string, std::string>;
    [[nodiscard]] auto resource_scan_options(const ResourcePolicy& policy) -> ResourceScanOptions;

    struct ResourcePackageResult {
        std::filesystem::path database;
        std::filesystem::path external_root;
        std::size_t embedded_count = 0;
        std::size_t external_count = 0;
        bool unchanged = false;
    };

    [[nodiscard]] auto
    pack_resource_package(const ResourcePolicy& policy, const ResourceLockManifest& manifest)
        -> std::expected<ResourcePackageResult, std::string>;
} // namespace nandina::resource

#endif
