// app/detail/windows_system_text_scale — Windows 无障碍文字缩放的 OS 来源。
//
// 只读 HKCU\Software\Microsoft\Accessibility\TextScaleFactor（REG_DWORD 百分数，
// 100–225）。注册表是 MSYS2/纯 Win32 构建下可直接使用的事实来源；WinRT
// UISettings.TextScaleFactorChanged 事件驱动版本留作后续（需要 COM/C++/WinRT 投影）。
//
// 本头在非 Windows 平台为空；调用方（SDL 平台窗口）只在 `_WIN32` 下构造实例。

#ifndef NANDINA_EXPERIMENT_APP_DETAIL_WINDOWS_SYSTEM_TEXT_SCALE_HPP
#define NANDINA_EXPERIMENT_APP_DETAIL_WINDOWS_SYSTEM_TEXT_SCALE_HPP

#if defined(_WIN32)

    #include "../system_preferences.hpp"

    #include <chrono>
    #include <cstdint>
    #include <optional>

namespace nandina::app::detail
{
    /// 节流读取 Windows 无障碍文字缩放的来源（UI 线程，Windows-only）。
    class WindowsSystemTextScaleSource final: public ISystemPreferencesSource {
    public:
        [[nodiscard]] auto snapshot() const -> SystemPreferencesSnapshot override;
        [[nodiscard]] auto poll_changes() -> std::optional<SystemPreferencesSnapshot> override;

    private:
        static constexpr auto poll_interval_ = std::chrono::milliseconds(500);

        std::optional<float> last_text_scale_;
        std::chrono::steady_clock::time_point next_poll_ {};
        bool has_polled_ = false;
    };

} // namespace nandina::app::detail

#endif // _WIN32

#endif // NANDINA_EXPERIMENT_APP_DETAIL_WINDOWS_SYSTEM_TEXT_SCALE_HPP
