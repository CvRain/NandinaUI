// app/detail/sdl_system_preferences — SDL3 跨平台系统外观 + 平台子源组合实现。

#include "sdl_system_preferences.hpp"

#if defined(NANDINA_HAS_SDL_BACKEND)

    #include <utility>

namespace nandina::app::detail
{
    SdlSystemPreferencesSource::SdlSystemPreferencesSource(
        std::unique_ptr<ISystemPreferencesSource> platform_source,
        ThemeQueryFn theme_query
    ):
        platform_source_(std::move(platform_source)),
        theme_query_(theme_query ? std::move(theme_query) : [] { return SDL_GetSystemTheme(); }) {}

    void SdlSystemPreferencesSource::notify_theme_changed() noexcept {
        theme_dirty_ = true;
    }

    auto SdlSystemPreferencesSource::merge_snapshots(
        const std::optional<theme::ColorAppearance> sdl_appearance,
        const SystemPreferencesSnapshot& platform
    ) noexcept -> SystemPreferencesSnapshot {
        SystemPreferencesSnapshot merged = platform;
        if (sdl_appearance.has_value()) {
            merged.appearance = sdl_appearance;
        }
        return merged;
    }

    auto SdlSystemPreferencesSource::snapshot() const -> SystemPreferencesSnapshot {
        const auto sdl_appearance = appearance_from_sdl_theme(theme_query_());
        const auto platform = platform_source_ != nullptr ? platform_source_->snapshot()
                                                          : SystemPreferencesSnapshot {};
        const auto merged = merge_snapshots(sdl_appearance, platform);
        last_sdl_appearance_ = sdl_appearance;
        last_platform_snapshot_ = platform;
        last_merged_ = merged;
        has_baseline_ = true;
        theme_dirty_ = false;
        return merged;
    }

    auto SdlSystemPreferencesSource::poll_changes() -> std::optional<SystemPreferencesSnapshot> {
        if (!has_baseline_) {
            const auto sdl_appearance = appearance_from_sdl_theme(theme_query_());
            const auto platform = platform_source_ != nullptr ? platform_source_->snapshot()
                                                              : SystemPreferencesSnapshot {};
            const auto merged = merge_snapshots(sdl_appearance, platform);
            last_sdl_appearance_ = sdl_appearance;
            last_platform_snapshot_ = platform;
            last_merged_ = merged;
            has_baseline_ = true;
            theme_dirty_ = false;
            return merged == SystemPreferencesSnapshot {} ? std::nullopt : std::optional(merged);
        }

        bool dirty = false;
        if (theme_dirty_) {
            theme_dirty_ = false;
            const auto next_sdl = appearance_from_sdl_theme(theme_query_());
            if (next_sdl != last_sdl_appearance_) {
                last_sdl_appearance_ = next_sdl;
                dirty = true;
            }
        }
        if (platform_source_ != nullptr) {
            if (const auto next_platform = platform_source_->poll_changes();
                next_platform.has_value())
            {
                last_platform_snapshot_ = *next_platform;
                dirty = true;
            }
        }
        if (!dirty) {
            return std::nullopt;
        }

        const auto merged = merge_snapshots(last_sdl_appearance_, last_platform_snapshot_);
        if (merged == last_merged_) {
            return std::nullopt;
        }
        last_merged_ = merged;
        return merged;
    }

} // namespace nandina::app::detail

#endif // NANDINA_HAS_SDL_BACKEND
