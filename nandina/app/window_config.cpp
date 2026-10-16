//
// app/window_config — 平台后端的默认值与名字解析。
//

#include "window_config.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
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
        if (normalized == "raylib") {
            return WindowBackend::raylib;
        }
        if (normalized == "sdl3" || normalized == "sdl") {
            return WindowBackend::sdl3;
        }
        if (normalized == "automatic" || normalized == "auto") {
            return WindowBackend::automatic;
        }
        return std::nullopt;
    }

    auto default_window_backend() -> WindowBackend {
        // 环境变量优先：覆盖只在「本构建有这个后端」时才生效，见 NanWindow 的解析。
        const auto from_env = env_backend_name();
        if (!from_env.empty()) {
            if (const auto parsed = parse_window_backend(from_env); parsed.has_value()) {
                return *parsed;
            }
        }
#if defined(NANDINA_HAS_SDL_BACKEND)
        return WindowBackend::sdl3;
#else
        return WindowBackend::raylib;
#endif
    }
} // namespace nandina::app
