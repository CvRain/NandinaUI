//
// System font discovery + CJK fallback tests.
//

#include "text/system_fonts.hpp"

#include "resource/resource_manager.hpp"
#include "text/font_loader.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
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
