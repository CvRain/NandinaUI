//
// app/window_config — 平台后端的默认值与名字解析。
//

#include "window_config.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace nandina::app
{
    namespace
    {
        [[nodiscard]] auto env_backend_name() -> std::string {
            const auto* raw = std::getenv("NANDINA_WINDOW_BACKEND");
            return raw != nullptr ? std::string(raw) : std::string {};
        }

        [[nodiscard]] auto lowercase(std::string_view text) -> std::string {
            std::string result(text);
            std::transform(
                result.begin(),
                result.end(),
                result.begin(),
                [](const unsigned char ch) { return static_cast<char>(std::tolower(ch)); }
            );
            return result;
        }
    } // namespace

    auto parse_window_backend(const std::string_view name) -> std::optional<WindowBackend> {
        const auto normalized = lowercase(name);
        if (normalized == "sdl3" || normalized == "sdl") {
            return WindowBackend::sdl3;
        }
        if (normalized == "automatic" || normalized == "auto") {
            return WindowBackend::automatic;
        }
        return std::nullopt;
    }

    auto default_window_backend() -> WindowBackend {
        const auto from_env = env_backend_name();
        if (!from_env.empty()) {
            const auto parsed = parse_window_backend(from_env);
            if (!parsed) {
                throw std::invalid_argument(
                    "NANDINA_WINDOW_BACKEND: unsupported backend " + from_env
                );
            }
            if (*parsed != WindowBackend::automatic) {
                return *parsed;
            }
        }
#if defined(NANDINA_HAS_SDL_BACKEND)
        return WindowBackend::sdl3;
#else
        return WindowBackend::automatic;
#endif
    }
} // namespace nandina::app
