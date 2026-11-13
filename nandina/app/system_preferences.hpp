// app/system_preferences — host-injected OS preferences used by the framework.
//
// 目前只承载无障碍文字缩放（P4.6 Step 3 的 OS 来源）。目标是让"读 OS 设置"留在平台层、
// "应用缩放"留在 app/theme 层：平台窗口提供一个 `ISystemPreferencesSource` 实现，宿主在
// 首帧前同步一次、之后按节流轮询，把变化交给 `ThemeManager::set_text_scaling()`。
//
// 约定：
// - 只读、UI 线程使用；实现不拥有宿主，也不得回调应用；
// - `snapshot()` 允许短暂阻塞（打开窗口时调用一次）；
// - `poll_changes()` 由实现内部节流，未变化时返回 nullopt，避免每帧读注册表/配置文件；
// - `nullopt` 表示"该平台没有可用的 OS 来源或值"，宿主按 1.0 处理，不假装已接入。

#ifndef NANDINA_EXPERIMENT_APP_SYSTEM_PREFERENCES_HPP
#define NANDINA_EXPERIMENT_APP_SYSTEM_PREFERENCES_HPP

#include <cstdint>
#include <optional>

namespace nandina::app
{
    /// 一次 OS 偏好快照。字段为空表示该平台没有来源或没有有效值。
    using SystemPreferencesSnapshot = struct SystemPreferencesSnapshot {
        /// 无障碍文字缩放因子（1.0 = 100%）。OS 来源的 0.5–2.25 范围由具体平台决定。
        std::optional<float> text_scale;
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
