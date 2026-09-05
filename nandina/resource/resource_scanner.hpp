#ifndef NANDINA_EXPERIMENT_RESOURCE_RESOURCE_SCANNER_HPP
#define NANDINA_EXPERIMENT_RESOURCE_RESOURCE_SCANNER_HPP

#include "resource.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::resource
{
    enum class ScanDiagnosticSeverity { warning, error };

    enum class ScanDiagnosticCode {
        root_unavailable,
        filesystem_error,
        symlink_rejected,
        hidden_path_rejected,
        generated_path_rejected,
        invalid_resource_key,
        normalized_key_collision,
        output_recursion,
    };

    struct ScanDiagnostic {
        ScanDiagnosticSeverity severity;
        ScanDiagnosticCode code;
        std::filesystem::path path;
        std::string message;
        auto operator<=>(const ScanDiagnostic&) const = default;
    };

    struct ResourceTypeRule {
        std::string glob;
        std::string media_type;
    };

    struct ResourceScanRoot {
        std::filesystem::path path;
        std::string key_prefix;
    };

    struct ResourceScanOptions {
        std::vector<ResourceScanRoot> roots;
        std::vector<std::string> excludes;
        std::vector<ResourceTypeRule> type_rules;
        std::vector<std::filesystem::path> output_paths;
        bool include_hidden = false;
        bool allow_symlinks = false;
    };

    struct ScannedResource {
        ResourceKey key;
        std::filesystem::path source_path;
        std::string media_type;
        std::uint64_t size = 0;
        auto operator<=>(const ScannedResource&) const = default;
    };

    struct ResourceScanResult {
        std::vector<ScannedResource> resources;
        std::vector<ScanDiagnostic> diagnostics;
        [[nodiscard]] auto valid() const noexcept -> bool;
    };

    [[nodiscard]] auto scan_resources(const ResourceScanOptions& options) -> ResourceScanResult;
    [[nodiscard]] auto detect_resource_media_type(
        const std::filesystem::path& path,
        std::string_view logical_key,
        const std::vector<ResourceTypeRule>& rules
    ) -> std::string;
} // namespace nandina::resource

#endif
