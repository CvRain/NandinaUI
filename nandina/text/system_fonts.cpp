//
// text/system_fonts - cross-platform system font discovery + CJK fallback.
//

#include "system_fonts.hpp"

#include "../resource/backends/directory_backend.hpp"

#include <cctype>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::text
{
    namespace
    {
        // 已知 CJK 字体文件名关键字（按优先级，小写子串匹配）。
        constexpr std::string_view cjk_hints[] = {
            "notosanscjk", // Noto Sans CJK（Linux 常见，.ttc 集合）
            "sourcehansans", // 思源黑体（adobe-source-han-sans）
            "sarasa", // Sarasa Gothic
            "pingfang", // macOS 苹方
            "hiragino", // macOS 冬青黑体
            "msyh", // Windows 微软雅黑
            "simsun", // Windows 宋体
            "meiryo", // Windows メイリオ
            "yugoth", // Windows Yu Gothic
            "wqy", // 文泉驿（Linux）
            "droidsansfallback", // Android
        };

        [[nodiscard]] auto ascii_lower(std::string_view input) -> std::string {
            std::string result(input);
            for (auto& ch: result) {
                if (ch >= 'A' && ch <= 'Z') {
                    ch = static_cast<char>(ch - 'A' + 'a');
                }
            }
            return result;
        }

        [[nodiscard]] auto env_path(const char* name) -> std::optional<std::filesystem::path> {
            const auto* value = std::getenv(name);
            if (value == nullptr || *value == '\0') {
                return std::nullopt;
            }
            return std::filesystem::path(value);
        }

        [[nodiscard]] auto is_font_file(const std::filesystem::path& path) -> bool {
            const auto extension = ascii_lower(path.extension().string());
            return extension == ".ttf" || extension == ".otf" || extension == ".ttc";
        }

        [[nodiscard]] auto media_type_for(const std::filesystem::path& path) -> std::string {
            const auto extension = ascii_lower(path.extension().string());
            if (extension == ".ttc") {
                return "font/collection";
            }
            if (extension == ".otf") {
                return "font/otf";
            }
            return "font/ttf";
        }
    } // namespace

    auto system_font_directories() -> std::vector<std::filesystem::path> {
        std::vector<std::filesystem::path> directories;
#if defined(_WIN32)
        if (const auto windows = env_path("WINDIR")) {
            directories.push_back(*windows / "Fonts");
        }
        else {
            directories.emplace_back("C:\\Windows\\Fonts");
        }
        if (const auto local = env_path("LOCALAPPDATA")) {
            directories.push_back(*local / "Microsoft" / "Windows" / "Fonts");
        }
#elif defined(__APPLE__)
        if (const auto home = env_path("HOME")) {
            directories.push_back(*home / "Library" / "Fonts");
        }
        directories.emplace_back("/Library/Fonts");
        directories.emplace_back("/System/Library/Fonts");
#else
        if (const auto home = env_path("HOME")) {
            directories.push_back(*home / ".local" / "share" / "fonts");
            directories.push_back(*home / ".fonts");
        }
        directories.emplace_back("/usr/local/share/fonts");
        directories.emplace_back("/usr/share/fonts");
#endif
        return directories;
    }

    auto find_cjk_font_in(const std::vector<std::filesystem::path>& directories)
        -> std::optional<std::filesystem::path> {
        std::optional<std::filesystem::path> best;
        std::size_t best_rank = std::size(cjk_hints);
        for (const auto& directory: directories) {
            std::error_code error;
            std::filesystem::recursive_directory_iterator iterator(
                directory,
                std::filesystem::directory_options::skip_permission_denied,
                error
            );
            if (error) {
                continue;
            }
            const std::filesystem::recursive_directory_iterator end;
            while (iterator != end) {
                const auto& entry = *iterator;
                if (entry.is_regular_file(error) && !error && is_font_file(entry.path())) {
                    const auto name = ascii_lower(entry.path().filename().string());
                    for (std::size_t rank = 0; rank < std::size(cjk_hints); ++rank) {
                        if (name.find(cjk_hints[rank]) != std::string::npos && rank < best_rank) {
                            best = entry.path();
                            best_rank = rank;
                            break;
                        }
                    }
                }
                iterator.increment(error);
                if (error) {
                    error.clear();
                }
            }
        }
        return best;
    }

    auto find_system_font_in(
        const std::vector<std::filesystem::path>& directories,
        std::string_view file_hint
    ) -> std::optional<std::filesystem::path> {
        const auto needle = ascii_lower(file_hint);
        if (needle.empty()) {
            return std::nullopt;
        }
        for (const auto& directory: directories) {
            std::error_code error;
            std::filesystem::recursive_directory_iterator iterator(
                directory,
                std::filesystem::directory_options::skip_permission_denied,
                error
            );
            if (error) {
                continue;
            }
            const std::filesystem::recursive_directory_iterator end;
            while (iterator != end) {
                const auto& entry = *iterator;
                if (entry.is_regular_file(error) && !error && is_font_file(entry.path())
                    && ascii_lower(entry.path().filename().string()).find(needle) != std::string::npos)
                {
                    return entry.path();
                }
                iterator.increment(error);
                if (error) {
                    error.clear();
                }
            }
        }
        return std::nullopt;
    }

    auto find_system_cjk_font() -> std::optional<std::filesystem::path> {
        return find_cjk_font_in(system_font_directories());
    }

    auto find_system_font(std::string_view file_hint) -> std::optional<std::filesystem::path> {
        return find_system_font_in(system_font_directories(), file_hint);
    }

    auto
    register_system_cjk_fallback(resource::ResourceManager& resources, FontFamilyRegistry& registry)
        -> FontResult<bool> {
        const auto font_path = find_system_cjk_font();
        if (!font_path) {
            return false;
        }

        const resource::ResourceKey font_key("fonts/system-cjk");
        auto backend = resource::DirectoryBackend::open({
            .name = "system-fonts",
            .root = font_path->parent_path(),
            .resources = {{
                .id = resource::ResourceId::random(),
                .key = font_key,
                .relative_path = font_path->filename(),
                .media_type = media_type_for(*font_path),
            }},
        });
        if (!backend) {
            return std::unexpected(
                FontError {
                    .code = FontErrorCode::resource_failure,
                    .operation = "font.system.cjk",
                    .message = backend.error().message,
                    .cause = backend.error(),
                }
            );
        }
        (void)resources.mount(std::move(*backend), 1000);

        return register_optional_font_fallback(
            registry,
            resources,
            resource::ResourceKey("families/system-cjk"),
            font_key
        );
    }

} // namespace nandina::text
