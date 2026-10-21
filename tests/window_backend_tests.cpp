#include <nandina/app/nan_application.hpp>
#include <nandina/app/nan_window.hpp>
#include <nandina/app/window_config.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>

namespace
{
    class BackendEnvironment {
    public:
        BackendEnvironment() {
            if (const auto* value = std::getenv("NANDINA_WINDOW_BACKEND")) {
                previous_ = value;
            }
            set("auto");
        }

        ~BackendEnvironment() {
            if (previous_) {
                set(previous_->c_str());
            }
            else {
#ifdef _WIN32
                _putenv_s("NANDINA_WINDOW_BACKEND", "");
#else
                unsetenv("NANDINA_WINDOW_BACKEND");
#endif
            }
        }

        static void set(const char* value) {
#ifdef _WIN32
            _putenv_s("NANDINA_WINDOW_BACKEND", value);
#else
            setenv("NANDINA_WINDOW_BACKEND", value, 1);
#endif
        }

    private:
        std::optional<std::string> previous_;
    };
} // namespace

TEST_CASE("retired and unknown backend names are rejected", "[app][window]") {
    using namespace nandina::app;
    BackendEnvironment environment;
    REQUIRE_FALSE(parse_window_backend("raylib"));
    REQUIRE_FALSE(parse_window_backend("unknown"));
    REQUIRE(parse_window_backend("SDL3") == WindowBackend::sdl3);
    REQUIRE(parse_window_backend("sdl") == WindowBackend::sdl3);
    REQUIRE(parse_window_backend("auto") == WindowBackend::automatic);
    BackendEnvironment::set("raylib");
    REQUIRE_THROWS_AS(default_window_backend(), std::invalid_argument);
    BackendEnvironment::set("unknown");
    REQUIRE_THROWS_AS(default_window_backend(), std::invalid_argument);
}

TEST_CASE("automatic backend and null platform reset follow this build", "[app][window]") {
    using namespace nandina::app;
    BackendEnvironment environment;
    NanApplication application;
    NanWindow window(application, {});
    window.set_platform_window(nullptr);
#ifdef NANDINA_TEST_HAS_SDL
    REQUIRE(default_window_backend() == WindowBackend::sdl3);
    REQUIRE(window.platform_name() == "sdl3");
    REQUIRE(window.platform_backend() == WindowBackend::sdl3);
#else
    REQUIRE(default_window_backend() == WindowBackend::automatic);
    REQUIRE(window.platform_name() == "none");
    REQUIRE_THROWS_WITH(
        window.open(),
        "NanWindow: no window backend; enable SDL3 or inject IPlatformWindow"
    );
    REQUIRE_THROWS_AS(
        NanWindow(application, WindowConfig {.backend = WindowBackend::sdl3}),
        std::invalid_argument
    );
#endif
    REQUIRE_THROWS_AS(
        NanWindow(application, WindowConfig {.backend = static_cast<WindowBackend>(1)}),
        std::invalid_argument
    );
}
