// app/detail/windows_system_text_scale — Windows 无障碍文字缩放的注册表读取。
#include "windows_system_text_scale.hpp"

#if defined(_WIN32)

    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>

    #include <chrono>
    #include <cstdint>
    #include <optional>

namespace nandina::app::detail
{
    namespace
    {
        constexpr wchar_t kAccessibilityKey[] = L"Software\\Microsoft\\Accessibility";
        constexpr wchar_t kTextScaleValue[] = L"TextScaleFactor";

        /// 读取百分数；键不存在/类型不符/长度不符都返回 nullopt。
        [[nodiscard]] auto read_text_scale_percent() -> std::optional<std::uint32_t> {
            HKEY key = nullptr;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, kAccessibilityKey, 0, KEY_QUERY_VALUE, &key)
                != ERROR_SUCCESS)
            {
                return std::nullopt;
            }
            DWORD type = 0;
            DWORD value = 0;
            DWORD size = sizeof(value);
            const auto status = RegQueryValueExW(
                key,
                kTextScaleValue,
                nullptr,
                &type,
                reinterpret_cast<LPBYTE>(&value),
                &size
            );
            RegCloseKey(key);
            if (status != ERROR_SUCCESS || type != REG_DWORD || size != sizeof(value)) {
                return std::nullopt;
            }
            return value;
        }
    } // namespace

    auto WindowsSystemTextScaleSource::snapshot() const -> SystemPreferencesSnapshot {
        const auto percent = read_text_scale_percent();
        return {
            .text_scale = percent.has_value() ? text_scale_from_percent(*percent) : std::nullopt,
        };
    }

    auto WindowsSystemTextScaleSource::poll_changes() -> std::optional<SystemPreferencesSnapshot> {
        const auto now = std::chrono::steady_clock::now();
        if (has_polled_ && now < next_poll_) {
            return std::nullopt;
        }
        has_polled_ = true;
        next_poll_ = now + poll_interval_;

        auto current = snapshot();
        if (current.text_scale == last_text_scale_) {
            return std::nullopt;
        }
        last_text_scale_ = current.text_scale;
        return current;
    }

} // namespace nandina::app::detail

#endif // _WIN32
