// app/detail/linux_system_preferences — Linux 桌面偏好来源（XDG Portal / gsettings / KDE / GTK）。
//
// 不引入额外硬编译依赖（不依赖 gio/libdbus）：
// - 优先在存在桌面会话时通过 `busctl` 查询 XDG Desktop Portal（一次调用拿到外观/文字缩放/减弱动效）；
// - 按桌面环境顺序回退到 `gsettings` 与纯配置文件读取（KDE `kcmfonts`/`kdeglobals`、GTK `settings.ini`）；
// - 所有子进程调用带严格超时（防止异常 D-Bus 或沙盒挂死），并修复关闭的标准描述符；
// - `poll_changes()` 先节流再对用户配置文件执行 `stat()` 门控，稳态不拉起任何子进程。

#ifndef NANDINA_EXPERIMENT_APP_DETAIL_LINUX_SYSTEM_PREFERENCES_HPP
#define NANDINA_EXPERIMENT_APP_DETAIL_LINUX_SYSTEM_PREFERENCES_HPP

#if defined(__linux__)

    #include "../system_preferences.hpp"

    #include <chrono>
    #include <cstdint>
    #include <filesystem>
    #include <functional>
    #include <initializer_list>
    #include <optional>
    #include <string>
    #include <string_view>
    #include <vector>

namespace nandina::app::detail
{
    /// 单个被监视配置文件的轻量元数据快照，用于 `stat()` 门控避免稳态拉起子进程。
    struct LinuxConfigFileStamp {
        bool exists = false;
        std::uint64_t device = 0;
        std::uint64_t inode = 0;
        std::int64_t size = 0;
        std::int64_t mtime_sec = 0;
        std::int64_t mtime_nsec = 0;

        [[nodiscard]] auto operator==(const LinuxConfigFileStamp&) const -> bool = default;
    };

    /// 带超时的子进程标准输出读取（仅内部使用；超时会强杀并回收子进程，无僵尸进程）。
    [[nodiscard]] auto read_command_output_with_timeout(
        std::initializer_list<std::string_view> command,
        std::chrono::milliseconds timeout
    ) -> std::optional<std::string>;

    /// 解析 `busctl --user --json=short call ... org.freedesktop.portal.Settings ReadAll ...` 输出。
    [[nodiscard]] auto parse_xdg_portal_settings_json(std::string_view json_text)
        -> SystemPreferencesSnapshot;

    /// 解析 `gsettings get org.gnome.desktop.interface text-scaling-factor` 输出。
    [[nodiscard]] auto parse_gsettings_text_scale(std::string_view text) -> std::optional<float>;

    /// 解析 `gsettings get org.gnome.desktop.interface color-scheme` 输出。
    [[nodiscard]] auto parse_gsettings_color_scheme(std::string_view text)
        -> std::optional<theme::ColorAppearance>;

    /// 解析 `gsettings get org.gnome.desktop.interface enable-animations` 输出并映射为 reduced_motion。
    [[nodiscard]] auto parse_gsettings_enable_animations(std::string_view text)
        -> std::optional<bool>;

    /// 解析 KDE `kcmfonts` 的 `[General] forceFontDPI=`（以 96 DPI 为 1.0；0 表示未设置）。
    [[nodiscard]] auto parse_kde_kcmfonts(std::string_view ini_text) -> std::optional<float>;

    /// 解析 KDE `kdeglobals` 的 `[KDE] AnimationDurationFactor=`（0 表示减弱动效，>0 表示开启）。
    [[nodiscard]] auto parse_kde_globals_reduced_motion(std::string_view ini_text)
        -> std::optional<bool>;

    /// 解析 GTK3/GTK4 `settings.ini` 的 `[Settings]`（`gtk-xft-dpi` / `gtk-enable-animations` /
    /// `gtk-application-prefer-dark-theme`）。
    [[nodiscard]] auto parse_gtk_settings_ini(std::string_view ini_text)
        -> SystemPreferencesSnapshot;

    /// 返回当前用户应监视的 Linux 桌面配置文件列表（遵循 `XDG_CONFIG_HOME`，回退 `$HOME/.config`）。
    [[nodiscard]] auto linux_desktop_watch_paths(const std::filesystem::path& config_home)
        -> std::vector<std::filesystem::path>;

    /// 对一组路径执行 `stat()` 生成轻量戳记列表。
    [[nodiscard]] auto capture_linux_watch_stamps(const std::vector<std::filesystem::path>& paths)
        -> std::vector<LinuxConfigFileStamp>;

    /// Linux 桌面偏好来源（UI 线程）。支持注入查询/监视/时钟以进行确定性单测。
    class LinuxSystemPreferencesSource final: public ISystemPreferencesSource {
    public:
        using QueryFn = std::function<SystemPreferencesSnapshot()>;
        using WatchFn = std::function<std::vector<LinuxConfigFileStamp>()>;
        using ClockFn = std::function<std::chrono::steady_clock::time_point()>;

        LinuxSystemPreferencesSource();
        LinuxSystemPreferencesSource(
            QueryFn query,
            WatchFn watch,
            ClockFn clock = {},
            std::chrono::milliseconds poll_interval = std::chrono::milliseconds(500)
        );

        [[nodiscard]] auto snapshot() const -> SystemPreferencesSnapshot override;
        [[nodiscard]] auto poll_changes() -> std::optional<SystemPreferencesSnapshot> override;

    private:
        QueryFn query_;
        WatchFn watch_;
        ClockFn clock_;
        std::chrono::milliseconds poll_interval_ {500};

        mutable std::vector<LinuxConfigFileStamp> last_stamps_;
        mutable SystemPreferencesSnapshot last_snapshot_ {};
        mutable std::chrono::steady_clock::time_point next_poll_ {};
        mutable bool has_baseline_ = false;
    };

} // namespace nandina::app::detail

#endif // __linux__

#endif // NANDINA_EXPERIMENT_APP_DETAIL_LINUX_SYSTEM_PREFERENCES_HPP
