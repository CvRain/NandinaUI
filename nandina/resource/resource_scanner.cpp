#include "resource_scanner.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <map>

namespace nandina::resource
{
    namespace
    {
        [[nodiscard]] auto ascii_lower(std::string value) -> std::string {
            std::ranges::transform(value, value.begin(), [](const unsigned char value) {
                return static_cast<char>(std::tolower(value));
            });
            return value;
        }

        [[nodiscard]] auto glob_matches(std::string_view pattern, std::string_view value) -> bool {
            std::size_t pattern_index = 0;
            std::size_t value_index = 0;
            std::size_t star = std::string_view::npos;
            std::size_t retry = 0;
            while (value_index < value.size()) {
                if (pattern_index < pattern.size()
                    && (pattern[pattern_index] == '?'
                        || pattern[pattern_index] == value[value_index]))
                {
                    ++pattern_index;
                    ++value_index;
                }
                else if (pattern_index < pattern.size() && pattern[pattern_index] == '*') {
                    star = pattern_index++;
                    retry = value_index;
                }
                else if (star != std::string_view::npos) {
                    pattern_index = star + 1;
                    value_index = ++retry;
                }
                else {
                    return false;
                }
            }
            while (pattern_index < pattern.size() && pattern[pattern_index] == '*') {
                ++pattern_index;
            }
            return pattern_index == pattern.size();
        }

        [[nodiscard]] auto is_hidden(const std::filesystem::path& relative) -> bool {
            return std::ranges::any_of(relative, [](const auto& part) {
                const auto value = part.string();
                return value.size() > 1 && value.front() == '.' && value != "..";
            });
        }

        [[nodiscard]] auto is_generated(const std::filesystem::path& relative) -> bool {
            return std::ranges::any_of(relative, [](const auto& part) {
                const auto value = ascii_lower(part.string());
                return value == "build" || value.starts_with("build-") || value == "meson-private"
                    || value == "meson-logs" || value == "__pycache__";
            });
        }

        [[nodiscard]] auto
        path_is_within(const std::filesystem::path& child, const std::filesystem::path& parent)
            -> bool {
            const auto normalized_child = std::filesystem::absolute(child).lexically_normal();
            const auto normalized_parent = std::filesystem::absolute(parent).lexically_normal();
            auto child_part = normalized_child.begin();
            for (auto parent_part = normalized_parent.begin();
                 parent_part != normalized_parent.end();
                 ++parent_part, ++child_part)
            {
                if (child_part == normalized_child.end() || *child_part != *parent_part) {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] auto extension_type(const std::filesystem::path& path) -> std::string_view {
            const auto extension = ascii_lower(path.extension().string());
            static constexpr std::array types {
                std::pair {std::string_view(".ttf"), std::string_view("font/ttf")},
                std::pair {std::string_view(".otf"), std::string_view("font/otf")},
                std::pair {std::string_view(".ttc"), std::string_view("font/collection")},
                std::pair {std::string_view(".png"), std::string_view("image/png")},
                std::pair {std::string_view(".jpg"), std::string_view("image/jpeg")},
                std::pair {std::string_view(".jpeg"), std::string_view("image/jpeg")},
                std::pair {std::string_view(".webp"), std::string_view("image/webp")},
                std::pair {std::string_view(".json"), std::string_view("application/json")},
                std::pair {std::string_view(".toml"), std::string_view("application/toml")},
                std::pair {std::string_view(".txt"), std::string_view("text/plain")},
                std::pair {std::string_view(".glsl"), std::string_view("text/x-glsl")},
                std::pair {std::string_view(".wav"), std::string_view("audio/wav")},
                std::pair {std::string_view(".ogg"), std::string_view("audio/ogg")},
                std::pair {std::string_view(".mp4"), std::string_view("video/mp4")},
            };
            const auto found =
                std::ranges::find(types, extension, &decltype(types)::value_type::first);
            return found == types.end() ? std::string_view {} : found->second;
        }

        [[nodiscard]] auto signature_type(const std::filesystem::path& path) -> std::string_view {
            std::ifstream stream(path, std::ios::binary);
            std::array<std::uint8_t, 12> bytes {};
            stream.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
            const auto count = static_cast<std::size_t>(stream.gcount());
            if (count >= 8 && bytes[0] == 0x89 && bytes[1] == 'P' && bytes[2] == 'N'
                && bytes[3] == 'G' && bytes[4] == 0x0d && bytes[5] == 0x0a && bytes[6] == 0x1a
                && bytes[7] == 0x0a)
            {
                return "image/png";
            }
            if (count >= 4 && bytes[0] == 0x00 && bytes[1] == 0x01 && bytes[2] == 0x00
                && bytes[3] == 0x00)
            {
                return "font/ttf";
            }
            if (count >= 4 && bytes[0] == 'O' && bytes[1] == 'T' && bytes[2] == 'T'
                && bytes[3] == 'O')
            {
                return "font/otf";
            }
            if (count >= 4 && bytes[0] == 't' && bytes[1] == 't' && bytes[2] == 'c'
                && bytes[3] == 'f')
            {
                return "font/collection";
            }
            if (count >= 4 && bytes[0] == 0xff && bytes[1] == 0xd8 && bytes[2] == 0xff) {
                return "image/jpeg";
            }
            if (count >= 12 && bytes[0] == 'R' && bytes[1] == 'I' && bytes[2] == 'F'
                && bytes[3] == 'F' && bytes[8] == 'W' && bytes[9] == 'E' && bytes[10] == 'B'
                && bytes[11] == 'P')
            {
                return "image/webp";
            }
            return {};
        }

        void diagnostic(
            ResourceScanResult& result,
            ScanDiagnosticCode code,
            const std::filesystem::path& path,
            std::string message
        ) {
            result.diagnostics.push_back({
                ScanDiagnosticSeverity::error,
                code,
                path,
                std::move(message),
            });
        }
    } // namespace

    auto ResourceScanResult::valid() const noexcept -> bool {
        return std::ranges::none_of(diagnostics, [](const auto& item) {
            return item.severity == ScanDiagnosticSeverity::error;
        });
    }

    auto detect_resource_media_type(
        const std::filesystem::path& path,
        const std::string_view logical_key,
        const std::vector<ResourceTypeRule>& rules
    ) -> std::string {
        for (const auto& rule: rules) {
            if (glob_matches(rule.glob, logical_key)) {
                return rule.media_type;
            }
        }
        if (const auto signature = signature_type(path); !signature.empty()) {
            return std::string(signature);
        }
        if (const auto extension = extension_type(path); !extension.empty()) {
            return std::string(extension);
        }
        return "application/octet-stream";
    }

    auto scan_resources(const ResourceScanOptions& options) -> ResourceScanResult {
        ResourceScanResult result;
        std::map<std::string, std::filesystem::path> normalized_keys;
        for (const auto& root: options.roots) {
            std::error_code error;
            if (!std::filesystem::is_directory(root.path, error) || error) {
                diagnostic(
                    result,
                    ScanDiagnosticCode::root_unavailable,
                    root.path,
                    "scan root is unavailable"
                );
                continue;
            }
            for (const auto& output: options.output_paths) {
                if (path_is_within(root.path, output)) {
                    diagnostic(
                        result,
                        ScanDiagnosticCode::output_recursion,
                        root.path,
                        "scan root is inside an output path"
                    );
                }
            }

            std::filesystem::recursive_directory_iterator iterator(
                root.path,
                std::filesystem::directory_options::skip_permission_denied,
                error
            );
            const std::filesystem::recursive_directory_iterator end;
            while (iterator != end) {
                if (error) {
                    diagnostic(
                        result,
                        ScanDiagnosticCode::filesystem_error,
                        root.path,
                        "resource tree traversal failed"
                    );
                    error.clear();
                    iterator.increment(error);
                    continue;
                }
                const auto path = iterator->path();
                const auto relative = path.lexically_relative(root.path);
                const auto logical_relative = relative.generic_string();
                if (std::ranges::any_of(
                        options.output_paths,
                        [&](const auto& output) { return path_is_within(path, output); }
                    ))
                {
                    diagnostic(
                        result,
                        ScanDiagnosticCode::output_recursion,
                        path,
                        "output path is inside a scan root"
                    );
                    if (iterator->is_directory(error)) {
                        iterator.disable_recursion_pending();
                    }
                    iterator.increment(error);
                    continue;
                }
                const bool excluded =
                    std::ranges::any_of(options.excludes, [&](const auto& pattern) {
                        return glob_matches(pattern, logical_relative);
                    });
                const bool hidden = !options.include_hidden && is_hidden(relative);
                const bool generated = is_generated(relative);
                const bool symlink = iterator->is_symlink(error);
                if (excluded || hidden || generated || (symlink && !options.allow_symlinks)) {
                    if (iterator->is_directory(error)) {
                        iterator.disable_recursion_pending();
                    }
                    if (symlink && !options.allow_symlinks) {
                        diagnostic(
                            result,
                            ScanDiagnosticCode::symlink_rejected,
                            path,
                            "symbolic links are not allowed"
                        );
                    }
                    iterator.increment(error);
                    continue;
                }
                if (iterator->is_directory(error)) {
                    iterator.increment(error);
                    continue;
                }
                if (!iterator->is_regular_file(error)) {
                    iterator.increment(error);
                    continue;
                }
                const auto key_text = root.key_prefix.empty()
                    ? logical_relative
                    : root.key_prefix + "/" + logical_relative;
                const auto normalized = ascii_lower(key_text);
                if (const auto found = normalized_keys.find(normalized);
                    found != normalized_keys.end() && found->second != path)
                {
                    diagnostic(
                        result,
                        ScanDiagnosticCode::normalized_key_collision,
                        path,
                        "resource key collides after case normalization"
                    );
                    iterator.increment(error);
                    continue;
                }
                normalized_keys.emplace(normalized, path);
                const auto key = ResourceKey::parse(key_text);
                if (!key) {
                    diagnostic(
                        result,
                        ScanDiagnosticCode::invalid_resource_key,
                        path,
                        "path cannot form a canonical resource key"
                    );
                    iterator.increment(error);
                    continue;
                }
                const auto size = iterator->file_size(error);
                if (error) {
                    diagnostic(
                        result,
                        ScanDiagnosticCode::filesystem_error,
                        path,
                        "resource size cannot be read"
                    );
                    error.clear();
                    iterator.increment(error);
                    continue;
                }
                result.resources.push_back({
                    .key = *key,
                    .source_path = std::filesystem::absolute(path).lexically_normal(),
                    .media_type = detect_resource_media_type(path, key_text, options.type_rules),
                    .size = size,
                });
                iterator.increment(error);
            }
        }
        std::ranges::sort(result.resources, {}, [](const auto& item) { return item.key.value(); });
        std::ranges::sort(result.diagnostics, [](const auto& left, const auto& right) {
            return std::tuple(left.path.generic_string(), left.code, left.message)
                < std::tuple(right.path.generic_string(), right.code, right.message);
        });
        return result;
    }
} // namespace nandina::resource
