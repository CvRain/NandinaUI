#include <catch2/catch_test_macros.hpp>

#include "app/detail/linux_clipboard.hpp"

#include <cstdlib>
#include <optional>

#include <unistd.h>

using namespace nandina;

namespace
{
    class ClosedStandardDescriptors {
    public:
        ClosedStandardDescriptors(const int first, const int second):
            first_(first),
            second_(second),
            saved_first_(dup(first)),
            saved_second_(dup(second)) {
            REQUIRE(saved_first_ >= 0);
            REQUIRE(saved_second_ >= 0);
            REQUIRE(close(first_) == 0);
            REQUIRE(close(second_) == 0);
        }

        ~ClosedStandardDescriptors() {
            (void)dup2(saved_first_, first_);
            (void)dup2(saved_second_, second_);
            (void)close(saved_first_);
            (void)close(saved_second_);
        }

        ClosedStandardDescriptors(const ClosedStandardDescriptors&) = delete;
        auto operator=(const ClosedStandardDescriptors&) -> ClosedStandardDescriptors& = delete;

    private:
        int first_;
        int second_;
        int saved_first_;
        int saved_second_;
    };
} // namespace

TEST_CASE("Wayland session detection requires a non-empty display", "[app][clipboard][linux]") {
    REQUIRE_FALSE(app::detail::is_wayland_session(nullptr));
    REQUIRE_FALSE(app::detail::is_wayland_session(""));
    REQUIRE(app::detail::is_wayland_session("wayland-0"));
}

TEST_CASE("Clipboard command reader preserves UTF-8 output", "[app][clipboard][linux]") {
    const auto text = app::detail::read_command_output(
        {"/bin/sh", "-c", "printf '\\344\\270\\255\\346\\226\\207'"}
    );

    REQUIRE(text == "中文");
    REQUIRE_FALSE(app::detail::read_command_output({"/bin/sh", "-c", "exit 7"}));
}

TEST_CASE("Clipboard command writer reports process success", "[app][clipboard][linux]") {
    constexpr auto verify_text = "test \"$(cat)\" = '跨应用粘贴'";

    REQUIRE(app::detail::write_command_input({"/bin/sh", "-c", verify_text}, "跨应用粘贴"));
    REQUIRE_FALSE(app::detail::write_command_input({"/bin/sh", "-c", verify_text}, "错误内容"));
    REQUIRE_FALSE(app::detail::write_command_input({"/bin/sh", "-c", "exit 9"}, "跨应用粘贴"));
}

TEST_CASE(
    "Clipboard commands repair standard descriptors closed by GUI launchers",
    "[app][clipboard][linux]"
) {
    std::optional<std::string> read_text;
    {
        ClosedStandardDescriptors descriptors(STDIN_FILENO, STDERR_FILENO);
        read_text = app::detail::read_command_output(
            {"/bin/sh", "-c", "test -r /dev/stdin && test -w /dev/stderr && printf repaired"}
        );
    }
    REQUIRE(read_text == "repaired");

    bool write_succeeded = false;
    {
        ClosedStandardDescriptors descriptors(STDOUT_FILENO, STDERR_FILENO);
        write_succeeded = app::detail::write_command_input(
            {"/bin/sh",
             "-c",
             "test -w /dev/stdout && test -w /dev/stderr && test \"$(cat)\" = repaired"},
            "repaired"
        );
    }
    REQUIRE(write_succeeded);
}

TEST_CASE(
    "Wayland clipboard bridge exchanges text with the active compositor",
    "[app][clipboard][linux][integration]"
) {
    if (std::getenv("NANDINA_WAYLAND_CLIPBOARD_INTEGRATION") == nullptr) {
        SKIP("requires an active Wayland compositor");
    }

    constexpr std::string_view sentinel = "nandina-wayland-integration-中文";
    REQUIRE(app::detail::write_wayland_clipboard(sentinel));
    REQUIRE(app::detail::read_wayland_clipboard() == sentinel);
}
