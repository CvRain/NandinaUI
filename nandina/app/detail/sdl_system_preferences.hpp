// app/detail/sdl_system_preferences — SDL3 跨平台系统外观 + 平台子源组合适配器。
//
// 职责：
// - 亮暗外观优先采用 SDL3 原生的 `SDL_GetSystemTheme()` + `SDL_EVENT_SYSTEM_THEME_CHANGED`
//   （Linux 内部经 XDG Desktop Portal 异步驱动，Windows/macOS 同源，稳态零子进程）；
// - 若 SDL 返回 `SDL_SYSTEM_THEME_UNKNOWN`（如 dummy 驱动或无 portal 的精简窗口管理器），
//   回退到下层平台子源的 `appearance`；
// - `text_scale` 与 `reduced_motion` 由下层平台子源提供（Linux 桌面源 / Windows 注册表源）。

#ifndef NANDINA_EXPERIMENT_APP_DETAIL_SDL_SYSTEM_PREFERENCES_HPP
#define NANDINA_EXPERIMENT_APP_DETAIL_SDL_SYSTEM_PREFERENCES_HPP

#if defined(NANDINA_HAS_SDL_BACKEND)

    #include "../system_preferences.hpp"

    #include <SDL3/SDL_video.h>

    #include <functional>
    #include <memory>
    #include <optional>

namespace nandina::app::detail
{
    /// SDL3 `SDL_SystemTheme` → 框架 `ColorAppearance`。`UNKNOWN` 返回 nullopt。
    [[nodiscard]] constexpr auto appearance_from_sdl_theme(const SDL_SystemTheme theme) noexcept
        -> std::optional<theme::ColorAppearance> {
        switch (theme) {
            case SDL_SYSTEM_THEME_LIGHT:
                return theme::ColorAppearance::light;
            case SDL_SYSTEM_THEME_DARK:
                return theme::ColorAppearance::dark;
            case SDL_SYSTEM_THEME_UNKNOWN:
            default:
                return std::nullopt;
        }
    }

    /// 组合 SDL3 事件驱动亮暗外观与平台子源（Linux/Windows 文字缩放与减弱动效）。
    class SdlSystemPreferencesSource final: public ISystemPreferencesSource {
    public:
        using ThemeQueryFn = std::function<SDL_SystemTheme()>;

        explicit SdlSystemPreferencesSource(
            std::unique_ptr<ISystemPreferencesSource> platform_source = nullptr,
            ThemeQueryFn theme_query = {}
        );

        /// 由 `SdlPlatformWindow::poll_input()` 在收到 `SDL_EVENT_SYSTEM_THEME_CHANGED` 时调用。
        void notify_theme_changed() noexcept;

        [[nodiscard]] auto snapshot() const -> SystemPreferencesSnapshot override;
        [[nodiscard]] auto poll_changes() -> std::optional<SystemPreferencesSnapshot> override;

    private:
        [[nodiscard]] static auto merge_snapshots(
            std::optional<theme::ColorAppearance> sdl_appearance,
            const SystemPreferencesSnapshot& platform
        ) noexcept -> SystemPreferencesSnapshot;

        std::unique_ptr<ISystemPreferencesSource> platform_source_;
        ThemeQueryFn theme_query_;

        mutable std::optional<theme::ColorAppearance> last_sdl_appearance_;
        mutable SystemPreferencesSnapshot last_platform_snapshot_ {};
        mutable SystemPreferencesSnapshot last_merged_ {};
        mutable bool has_baseline_ = false;
        mutable bool theme_dirty_ = false;
    };

} // namespace nandina::app::detail

#endif // NANDINA_HAS_SDL_BACKEND

#endif // NANDINA_EXPERIMENT_APP_DETAIL_SDL_SYSTEM_PREFERENCES_HPP
