//
// Foundation-layer UTF-8 tests.
//

#include "foundation/nan_logger.hpp"
#include "foundation/json.hpp"
#include "foundation/utf8.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

using namespace nandina;

namespace
{
    struct LogShutdown {
        ~LogShutdown() {
            log::shutdown();
        }
    };

    [[nodiscard]] auto temporary_log_path() -> std::filesystem::path {
        return std::filesystem::temp_directory_path()
            / ("nandina-logger-"
               + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())
               + ".log");
    }

    [[nodiscard]] auto read_file(const std::filesystem::path& path) -> std::string {
        std::ifstream input(path);
        return {
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()
        };
    }
} // namespace

TEST_CASE("Logger hides its backend and shares named state", "[foundation][logger]") {
    log::shutdown();
    const LogShutdown shutdown;
    const auto path = temporary_log_path();

    log::initialize({
        .name = "logger-test",
        .level = log::LogLevel::trace,
        .file = path,
    });
    const auto first = log::get("foundation.test");
    const auto second = log::get("foundation.test");
    REQUIRE(first.name() == "foundation.test");
    REQUIRE(first.enabled(log::LogLevel::debug));

    first.set_level(log::LogLevel::error);
    REQUIRE_FALSE(second.enabled(log::LogLevel::warning));
    second.warn("hidden message");
    second.error("visible {}", 42);
    log::info("root message");
    log::flush();

    const auto contents = read_file(path);
    REQUIRE(contents.find("visible 42") != std::string::npos);
    REQUIRE(contents.find("root message") != std::string::npos);
    REQUIRE(contents.find("hidden message") == std::string::npos);

    log::shutdown();
    REQUIRE_FALSE(first.enabled(log::LogLevel::critical));
    REQUIRE_NOTHROW(first.info("inactive handle"));
    std::error_code error;
    std::filesystem::remove(path, error);
}

TEST_CASE("Explicit Logger configuration replaces the implicit default", "[foundation][logger]") {
    log::shutdown();
    const LogShutdown shutdown;
    const auto logger = log::get("foundation.early");
    const auto path = temporary_log_path();

    log::initialize({
        .name = "configured",
        .level = log::LogLevel::debug,
        .file = path,
    });
    logger.debug("configured existing handle");
    log::flush();

    const auto contents = read_file(path);
    REQUIRE(contents.find("configured existing handle") != std::string::npos);

    log::shutdown();
    std::error_code error;
    std::filesystem::remove(path, error);
}

TEST_CASE("Logger rejects invalid configuration", "[foundation][logger]") {
    log::shutdown();
    const LogShutdown shutdown;
    REQUIRE_THROWS_AS(
        log::initialize({.name = ""}),
        std::invalid_argument
    );
    REQUIRE_THROWS_AS(
        log::initialize({.file = temporary_log_path(), .max_file_size = 0}),
        std::invalid_argument
    );
    REQUIRE_THROWS_AS(log::get(""), std::invalid_argument);
}

TEST_CASE("UTF-8 encodes Unicode scalar values", "[foundation][utf8]") {
    REQUIRE(foundation::utf8::encode(U'A') == "A");
    REQUIRE(foundation::utf8::encode(U'中') == "\xE4\xB8\xAD");
    REQUIRE(foundation::utf8::encode(U'🙂') == "\xF0\x9F\x99\x82");
    REQUIRE(foundation::utf8::encode(static_cast<char32_t>(0xD800)) == "\xEF\xBF\xBD");
}

TEST_CASE("UTF-8 helpers preserve codepoint boundaries", "[foundation][utf8]") {
    const std::string text = "A中🙂";

    REQUIRE(foundation::utf8::codepoint_count(text) == 3);
    REQUIRE(foundation::utf8::next_boundary(text, 0) == 1);
    REQUIRE(foundation::utf8::next_boundary(text, 1) == 4);
    REQUIRE(foundation::utf8::next_boundary(text, 4) == 8);
    REQUIRE(foundation::utf8::previous_boundary(text, 8) == 4);
    REQUIRE(foundation::utf8::previous_boundary(text, 4) == 1);
    REQUIRE(foundation::utf8::clamp_boundary(text, 2) == 1);
    REQUIRE(foundation::utf8::byte_offset_for_codepoints(text, 2) == 4);
}

TEST_CASE("UTF-8 decode preserves source byte ranges", "[foundation][utf8]") {
    const std::string text = "A中🙂";
    const auto decoded = foundation::utf8::decode(text);

    REQUIRE(decoded.size() == 3);
    REQUIRE(decoded[0].value == U'A');
    REQUIRE(decoded[0].byte_offset == 0);
    REQUIRE(decoded[0].byte_length == 1);
    REQUIRE(decoded[1].value == U'中');
    REQUIRE(decoded[1].byte_offset == 1);
    REQUIRE(decoded[1].byte_length == 3);
    REQUIRE(decoded[2].value == U'🙂');
    REQUIRE(decoded[2].byte_offset == 4);
    REQUIRE(decoded[2].byte_length == 4);

    const auto malformed = foundation::utf8::decode("\xFF" "A");
    REQUIRE(malformed.size() == 2);
    REQUIRE(malformed[0].value == foundation::utf8::replacement_character);
    REQUIRE(malformed[0].byte_length == 1);
    REQUIRE(malformed[1].value == U'A');
}

TEST_CASE("UTF-8 grapheme ranges follow UAX 29 boundaries", "[foundation][utf8][grapheme]") {
    const std::string combining = "a\xCC\x81" "b";
    const auto combining_ranges = foundation::utf8::grapheme_ranges(combining);
    REQUIRE(combining_ranges.size() == 2);
    REQUIRE(combining_ranges[0].offset == 0);
    REQUIRE(combining_ranges[0].length == 3);
    REQUIRE(combining_ranges[1].offset == 3);

    const std::string emoji_zwj = "👩‍💻";
    const auto emoji_ranges = foundation::utf8::grapheme_ranges(emoji_zwj);
    REQUIRE(emoji_ranges.size() == 1);
    REQUIRE(emoji_ranges.front().length == emoji_zwj.size());

    const std::string flag = "🇨🇳";
    const auto flag_ranges = foundation::utf8::grapheme_ranges(flag);
    REQUIRE(flag_ranges.size() == 1);
    REQUIRE(flag_ranges.front().length == flag.size());
}

TEST_CASE("JSON wrapper parses documents and maps errors", "[foundation][json]") {
    const auto root = foundation::parse_json(
        R"({"format":1,"package_id":"org.nandina.d2","meta":{"ok":true},"items":[1,2]})"
    );
    REQUIRE(root.has_value());
    REQUIRE(root->is_object());
    REQUIRE((*root)["package_id"] == "org.nandina.d2");
    REQUIRE((*root)["meta"]["ok"].get<bool>());
    REQUIRE((*root)["items"].is_array());

    REQUIRE_FALSE(foundation::parse_json(R"({"a":})").has_value());
    REQUIRE_FALSE(foundation::parse_json(R"({"a":1} trailing)").has_value());
    REQUIRE_FALSE(foundation::parse_json(R"(not json)").has_value());
}
