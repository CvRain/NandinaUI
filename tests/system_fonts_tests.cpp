//
// System font discovery + CJK fallback tests.
//

#include <nandina/text/system_fonts.hpp>

#include <nandina/resource/resource_manager.hpp>
#include <nandina/text/font_loader.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>

using namespace nandina;

namespace
{
    class TempDirectory {
    public:
        TempDirectory() {
            const auto base = std::filesystem::temp_directory_path() / "nandina-system-fonts-tests";
            std::filesystem::create_directories(base);
            path_ = base / std::to_string(counter_++);
            std::filesystem::create_directories(path_);
        }

        ~TempDirectory() {
            std::error_code error;
            std::filesystem::remove_all(path_, error);
        }

        [[nodiscard]] auto path() const -> const std::filesystem::path& {
            return path_;
        }

        void write_font(const std::string& filename) {
            std::ofstream file(path_ / filename, std::ios::binary);
            file << "dummy-font-bytes";
        }

    private:
        static inline std::size_t counter_ = 0;
        std::filesystem::path path_;
    };

#if defined(__linux__)
    auto home_value() -> std::optional<std::string> {
        const auto* value = std::getenv("HOME");
        return value != nullptr ? std::optional<std::string>(value) : std::nullopt;
    }

    class ScopedHome {
    public:
        explicit ScopedHome(const std::filesystem::path& temporary_home_path):
            previous_(home_value()) {
            if (::setenv("HOME", temporary_home_path.c_str(), 1) != 0) {
                throw std::runtime_error("cannot set temporary HOME");
            }
        }

        ~ScopedHome() {
            if (previous_) {
                (void)::setenv("HOME", previous_->c_str(), 1);
            }
            else {
                (void)::unsetenv("HOME");
            }
        }

        ScopedHome(const ScopedHome&) = delete;
        auto operator=(const ScopedHome&) -> ScopedHome& = delete;

    private:
        std::optional<std::string> previous_;
    };
#endif
} // namespace

TEST_CASE("find_cjk_font_in matches known CJK filenames", "[text][system-fonts]") {
    TempDirectory directory;
    directory.write_font("NotoSansCJK-Regular.ttc");
    directory.write_font("unrelated.ttf");

    const auto found = text::find_cjk_font_in({directory.path()});
    REQUIRE(found.has_value());
    REQUIRE(found->filename() == "NotoSansCJK-Regular.ttc");
}

TEST_CASE("find_cjk_font_in prioritizes higher-ranked CJK fonts", "[text][system-fonts]") {
    TempDirectory directory;
    directory.write_font("wqy-zenhei.ttc"); // 低优先级
    directory.write_font("SourceHanSansCN.otf"); // 高优先级

    const auto found = text::find_cjk_font_in({directory.path()});
    REQUIRE(found.has_value());
    REQUIRE(found->filename() == "SourceHanSansCN.otf");
}

TEST_CASE("find_cjk_font_in returns nullopt when absent", "[text][system-fonts]") {
    TempDirectory directory;
    directory.write_font("LiberationSans-Regular.ttf");

    const auto found = text::find_cjk_font_in({directory.path()});
    REQUIRE_FALSE(found.has_value());
}

TEST_CASE("find_system_font_in matches a filename substring, case-insensitively", "[text][system-fonts]") {
    TempDirectory directory;
    directory.write_font("FreeSerifItalic.otf");
    directory.write_font("unrelated.ttf");

    const auto found = text::find_system_font_in({directory.path()}, "freeserif");
    REQUIRE(found.has_value());
    REQUIRE(found->filename() == "FreeSerifItalic.otf");
}

TEST_CASE("find_system_font_in returns nullopt when absent or hint empty", "[text][system-fonts]") {
    TempDirectory directory;
    directory.write_font("LiberationSans-Regular.ttf");

    REQUIRE_FALSE(text::find_system_font_in({directory.path()}, "noto").has_value());
    REQUIRE_FALSE(text::find_system_font_in({directory.path()}, "").has_value());
}

TEST_CASE("system_font_directories returns at least one directory", "[text][system-fonts]") {
    const auto directories = text::system_font_directories();
    REQUIRE_FALSE(directories.empty());
}

TEST_CASE("register_system_cjk_fallback degrades gracefully", "[text][system-fonts]") {
    resource::ResourceManager resources;
    text::FontFamilyRegistry registry;

    const auto result = text::register_system_cjk_fallback(resources, registry);
    REQUIRE(result.has_value());

    // 若本机存在 CJK 字体，应能按新族名解析出字面。
    if (*result) {
        text::FontLoader loader(resources);
        const auto resolved =
            registry.resolve({.family = resource::ResourceKey("families/system-cjk")}, loader);
        REQUIRE(resolved.has_value());
        REQUIRE_FALSE(resolved->faces.empty());
    }
}

#if defined(__linux__)
TEST_CASE("register_system_cjk_fallback loads a discovered symlink", "[text][system-fonts]") {
    TempDirectory directory;
    directory.write_font("font.bin");
    const auto temporary_home_path = directory.path() / "home";
    const auto fonts = temporary_home_path / ".local" / "share" / "fonts";
    std::filesystem::create_directories(fonts);
    const auto font_link = fonts / "NotoSansCJK-Regular.ttc";
    std::filesystem::create_symlink(directory.path() / "font.bin", font_link);

    const auto previous_home = home_value();
    {
        // Discovery searches this directory first; a rank-zero filename also wins
        // when the machine has other real CJK fonts. Only this process HOME changes.
        ScopedHome temporary_home(temporary_home_path);
        REQUIRE(text::find_system_cjk_font() == font_link);
        resource::ResourceManager resources;
        text::FontFamilyRegistry registry;
        const auto registered = text::register_system_cjk_fallback(resources, registry);
        REQUIRE(registered.has_value());
        REQUIRE(*registered);

        // Registration needs resource access, not font parsing. Prove that it loads
        // the linked bytes, rather than merely announcing a discovered filename.
        const auto font = resources.require(resource::ResourceKey("fonts/system-cjk"));
        REQUIRE(font.has_value());
        const auto bytes = (*font)->bytes();
        REQUIRE(std::string(bytes.begin(), bytes.end()) == "dummy-font-bytes");
    }
    REQUIRE(home_value() == previous_home);
}
#endif
