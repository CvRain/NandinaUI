#include "resource/resource_manifest.hpp"
#include "resource/resource_scanner.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using namespace nandina;

namespace
{
    struct Arguments {
        std::string command;
        std::vector<resource::ResourceScanRoot> roots;
        std::vector<std::string> excludes;
        std::vector<resource::ResourceTypeRule> type_rules;
        std::vector<std::filesystem::path> outputs;
        bool include_hidden = false;
        bool allow_symlinks = false;
        bool has_scan_overrides = false;
    };

    void usage() {
        std::cerr << "usage: nanres <init|scan|validate|pack|watch> [options]\n"
                     "  --root <path>[:key-prefix]\n"
                     "  --exclude <glob>\n"
                     "  --type <glob>=<media-type>\n"
                     "  --output <path>\n"
                     "  --include-hidden\n"
                     "  --allow-symlinks\n";
    }

    [[nodiscard]] auto parse_arguments(int argc, char** argv) -> std::optional<Arguments> {
        if (argc < 2) {
            return std::nullopt;
        }
        Arguments result {.command = argv[1]};
        for (int index = 2; index < argc; ++index) {
            const std::string_view argument(argv[index]);
            if (argument == "--include-hidden") {
                result.include_hidden = true;
                continue;
            }
            if (argument == "--allow-symlinks") {
                result.allow_symlinks = true;
                continue;
            }
            if (index + 1 >= argc) {
                return std::nullopt;
            }
            const std::string value(argv[++index]);
            if (argument == "--root") {
                result.has_scan_overrides = true;
                const auto separator = value.find(':');
                result.roots.push_back({
                    .path = value.substr(0, separator),
                    .key_prefix = separator == std::string::npos ? "" : value.substr(separator + 1),
                });
            }
            else if (argument == "--exclude") {
                result.has_scan_overrides = true;
                result.excludes.push_back(value);
            }
            else if (argument == "--output") {
                result.has_scan_overrides = true;
                result.outputs.emplace_back(value);
            }
            else if (argument == "--type") {
                result.has_scan_overrides = true;
                const auto separator = value.find('=');
                if (separator == std::string::npos || separator == 0
                    || separator + 1 == value.size())
                {
                    return std::nullopt;
                }
                result.type_rules.push_back({
                    value.substr(0, separator),
                    value.substr(separator + 1),
                });
            }
            else {
                return std::nullopt;
            }
        }
        return result;
    }

    [[nodiscard]] auto diagnostic_name(resource::ScanDiagnosticCode code) -> std::string_view {
        using enum resource::ScanDiagnosticCode;
        switch (code) {
            case root_unavailable:
                return "root-unavailable";
            case filesystem_error:
                return "filesystem-error";
            case symlink_rejected:
                return "symlink-rejected";
            case hidden_path_rejected:
                return "hidden-path-rejected";
            case generated_path_rejected:
                return "generated-path-rejected";
            case invalid_resource_key:
                return "invalid-resource-key";
            case normalized_key_collision:
                return "normalized-key-collision";
            case output_recursion:
                return "output-recursion";
        }
        return "unknown";
    }

    void print_diagnostics(const resource::ResourceScanResult& result) {
        for (const auto& diagnostic: result.diagnostics) {
            std::cerr << "error[" << diagnostic_name(diagnostic.code) << "] "
                      << diagnostic.path.generic_string() << ": " << diagnostic.message << '\n';
        }
    }

    [[nodiscard]] auto init() -> int {
        const std::filesystem::path path = "resources.toml";
        if (std::filesystem::exists(path)) {
            std::cerr << "nanres: resources.toml already exists\n";
            return 2;
        }
        std::ofstream output(path, std::ios::binary);
        if (!output) {
            std::cerr << "nanres: cannot create resources.toml\n";
            return 2;
        }
        output << "package = \"com.example.application\"\n";
        output.close();
        if (!output) { return 2; }
        std::error_code error;
        std::filesystem::create_directories("assets", error);
        if (error) {
            std::cerr << "nanres: cannot create assets directory: " << error.message() << '\n';
            return 2;
        }
        return 0;
    }

    [[nodiscard]] auto same_lock(
        const resource::ResourceLockManifest& left,
        const resource::ResourceLockManifest& right
    ) -> bool {
        if (left.package_id != right.package_id || left.resources.size() != right.resources.size())
        {
            return false;
        }
        for (std::size_t index = 0; index < left.resources.size(); ++index) {
            const auto& a = left.resources[index];
            const auto& b = right.resources[index];
            if (a.id != b.id || a.key != b.key || a.source != b.source
                || a.media_type != b.media_type || a.size != b.size || a.sha256 != b.sha256
                || a.storage != b.storage || a.streaming != b.streaming || a.revision != b.revision)
            {
                return false;
            }
        }
        return true;
    }
} // namespace

auto main(int argc, char** argv) -> int {
    const auto arguments = parse_arguments(argc, argv);
    if (!arguments) {
        usage();
        return 2;
    }
    if (arguments->command == "init") {
        return init();
    }
    if (arguments->command == "watch") {
        std::cerr << "nanres: watch requires stable hot-reload invalidation semantics\n";
        return 3;
    }
    if (arguments->command != "scan" && arguments->command != "validate"
        && arguments->command != "pack")
    {
        usage();
        return 2;
    }

    const bool manifest_mode = arguments->command == "pack"
        || (!arguments->has_scan_overrides && !arguments->include_hidden
            && !arguments->allow_symlinks);
    if (manifest_mode) {
        if (arguments->command == "pack"
            && (!arguments->roots.empty() || !arguments->excludes.empty()
                || !arguments->type_rules.empty() || arguments->include_hidden
                || arguments->allow_symlinks))
        {
            std::cerr << "nanres: pack only accepts --output as an override\n";
            return 2;
        }
        const std::filesystem::path policy_path = "resources.toml";
        const std::filesystem::path lock_path = "resources.lock.toml";
        auto policy = resource::load_resource_policy(policy_path);
        if (!policy) {
            std::cerr << "nanres: " << policy.error() << '\n';
            return 1;
        }
        const auto scan = resource::scan_resources(resource::resource_scan_options(*policy));
        print_diagnostics(scan);
        if (!scan.valid()) {
            return 1;
        }
        std::optional<resource::ResourceLockManifest> previous;
        if (std::filesystem::exists(lock_path)) {
            auto loaded = resource::load_resource_lock(lock_path);
            if (!loaded) {
                std::cerr << "nanres: " << loaded.error() << '\n';
                return 1;
            }
            previous = std::move(*loaded);
        }
        auto generated = resource::build_resource_lock(*policy, scan, previous);
        if (!generated) {
            std::cerr << "nanres: " << generated.error() << '\n';
            return 1;
        }
        if (arguments->command == "validate" || arguments->command == "pack") {
            if (!previous || !same_lock(*previous, *generated)) {
                std::cerr << "nanres: resources.lock.toml is missing or stale; run nanres scan\n";
                return 1;
            }
            if (arguments->command == "pack") {
                if (!arguments->outputs.empty()) {
                    if (arguments->outputs.size() != 1) {
                        std::cerr << "nanres: pack accepts at most one --output\n";
                        return 2;
                    }
                    policy->package_directory = arguments->outputs.front();
                }
                auto packed = resource::pack_resource_package(*policy, *previous);
                if (!packed) {
                    std::cerr << "nanres: " << packed.error() << '\n';
                    return 1;
                }
                std::cout << (packed->unchanged ? "unchanged" : "packed") << '\t'
                          << packed->database.generic_string() << '\t' << packed->embedded_count
                          << '\t' << packed->external_count << '\n';
            }
            return 0;
        }
        auto written = resource::write_resource_lock_atomic(*generated, lock_path);
        if (!written) {
            std::cerr << "nanres: " << written.error() << '\n';
            return 1;
        }
        for (const auto& item: generated->resources) {
            std::cout << item.id.to_string() << '\t' << item.key.value() << '\t' << item.media_type
                      << '\t' << item.size << '\t' << item.sha256 << '\t'
                      << item.source.generic_string() << '\n';
        }
        return 0;
    }

    auto options = resource::ResourceScanOptions {
        .roots = arguments->roots.empty()
            ? std::vector<resource::ResourceScanRoot> {{{.path = "res"}}}
            : arguments->roots,
        .excludes = arguments->excludes,
        .type_rules = arguments->type_rules,
        .output_paths = arguments->outputs,
        .include_hidden = arguments->include_hidden,
        .allow_symlinks = arguments->allow_symlinks,
    };
    const auto result = resource::scan_resources(options);
    print_diagnostics(result);
    if (arguments->command == "scan") {
        for (const auto& item: result.resources) {
            std::cout << item.key.value() << '\t' << item.media_type << '\t' << item.size << '\t'
                      << item.source_path.generic_string() << '\n';
        }
    }
    return result.valid() ? 0 : 1;
}
