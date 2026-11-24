// app/system_preferences — host-injected OS preferences used by the framework.
//
// 承载平台层读取的 OS 桌面偏好（无障碍文字缩放、系统亮暗外观、减弱动效）。目标是让
// "读 OS 设置"留在平台层、"仲裁与应用"留在 app/theme 层：平台窗口提供一个
// `ISystemPreferencesSource` 实现，宿主在首帧前同步一次、之后按节流轮询，把变化分别
// 交给 `ThemeManager::set_system_preferences()` 与 `ThemeManager::set_text_scaling()`。
//
// 约定：
// - 只读、UI 线程使用；实现不拥有宿主，也不得回调应用；
// - `snapshot()` 允许短暂阻塞（打开窗口时调用一次）；
// - `poll_changes()` 由实现内部节流，未变化时返回 nullopt，避免每帧读注册表/配置文件；
// - 字段为空（`nullopt`）表示"该平台没有可用的 OS 来源或有效值"，宿主按默认值回退
//   （`text_scale = 1.0`、`appearance = light`、`reduced_motion = false`），不假装已接入。

#ifndef NANDINA_EXPERIMENT_APP_SYSTEM_PREFERENCES_HPP
#define NANDINA_EXPERIMENT_APP_SYSTEM_PREFERENCES_HPP

#include "../theme/appearance.hpp"

#include <cstdint>
#include <optional>

namespace nandina::app
{
    /// 一次 OS 偏好快照。字段为空表示该平台没有来源或没有有效值。
    using SystemPreferencesSnapshot = struct SystemPreferencesSnapshot {
        /// 无障碍文字缩放因子（1.0 = 100%）。各平台有效区间不同，非法/缺失值用 nullopt 表示。
        std::optional<float> text_scale;
        /// 系统亮暗外观（light / dark）。平台未知或无来源时为 nullopt，宿主回退 light。
        std::optional<theme::ColorAppearance> appearance;
        /// 系统减弱动效偏好（true = 减弱或关闭动效）。无来源时为 nullopt，宿主回退 false。
        std::optional<bool> reduced_motion;

        [[nodiscard]] auto operator==(const SystemPreferencesSnapshot&) const -> bool = default;
    };

    /**
     * Windows 辅助功能的文字缩放百分比（100–225）→ 缩放因子。
     *
     * 注册表 `HKCU\Software\Microsoft\Accessibility\TextScaleFactor` 与 WinRT
     * `UISettings.TextScaleFactor` 使用同一口径的百分数；超出 [100, 225] 的值视为
     * 无有效来源（返回 nullopt），由宿主回退 1.0，而不是把损坏的设置放大到窗口里。
     */
    [[nodiscard]] constexpr auto text_scale_from_percent(const std::uint32_t percent) noexcept
        -> std::optional<float> {
        if (percent < 100U || percent > 225U) {
            return std::nullopt;
        }
        return static_cast<float>(percent) / 100.0F;
    }

    /**
     * 桌面连续文字缩放因子（0.5–3.0，例如 GNOME `text-scaling-factor`）→ 缩放因子。
     *
     * 非有限值（NaN/±Inf）或超出 [0.5, 3.0] 的值视为无有效来源（返回 nullopt），
     * 不做静默钳位，由宿主回退 1.0。
     */
    [[nodiscard]] constexpr auto text_scale_from_factor(const double factor) noexcept
        -> std::optional<float> {
        if (!(factor >= 0.5 && factor <= 3.0)) {
            return std::nullopt;
        }
        return static_cast<float>(factor);
    }

    /**
     * 逻辑字体 DPI（以 96 DPI 为 1.0，例如 KDE `forceFontDPI` 或 GTK `gtk-xft-dpi / 1024`）
     * → 文字缩放因子。0（常见“自动/未设置”哨兵）与换算后越出 [0.5, 3.0] 的值返回 nullopt。
     */
    [[nodiscard]] constexpr auto text_scale_from_dpi(const double dpi) noexcept
        -> std::optional<float> {
        return text_scale_from_factor(dpi / 96.0);
    }

    /// 平台提供的 OS 偏好来源。生命周期由平台窗口/应用管理；框架不持有所有权语义。
    class ISystemPreferencesSource {
    public:
        ISystemPreferencesSource() = default;
        virtual ~ISystemPreferencesSource() = default;

        ISystemPreferencesSource(const ISystemPreferencesSource&) = delete;
        auto operator=(const ISystemPreferencesSource&) -> ISystemPreferencesSource& = delete;
        ISystemPreferencesSource(ISystemPreferencesSource&&) = delete;
        auto operator=(ISystemPreferencesSource&&) -> ISystemPreferencesSource& = delete;

        /// 当前快照。允许短暂阻塞；只在打开窗口/宿主显式同步时调用。
        [[nodiscard]] virtual auto snapshot() const -> SystemPreferencesSnapshot = 0;

        /// 返回变化后的快照；未变化返回 nullopt。实现必须自行节流（例如约 500 ms 一次）。
        [[nodiscard]] virtual auto poll_changes() -> std::optional<SystemPreferencesSnapshot> = 0;
    };

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_SYSTEM_PREFERENCES_HPP
