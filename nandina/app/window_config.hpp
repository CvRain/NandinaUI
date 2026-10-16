//
// Created by cvrain on 2026/7/4.
//
// app/window_config — NanWindow 的创建配置。
//
// 标准窗口零配置即可; 需要自定义标题栏 / 无边框 / 可调整大小时通过字段声明,
// 具体的自定义标题栏 chrome 由 NanWindow 子类覆写 build_chrome() 提供。
//

#ifndef NANDINA_EXPERIMENT_APP_WINDOW_CONFIG_HPP
#define NANDINA_EXPERIMENT_APP_WINDOW_CONFIG_HPP

#include "../foundation/nandina_color.hpp"
#include "viewport_scaling.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace nandina::app
{

    /// 窗口与输入的平台后端。
    ///
    /// `sdl3` 是 D4 的目标平台层（SDL3 窗口 + 输入 + 帧驱动 + SDL 渲染设备）；
    /// `raylib` 是历史生产路径，保留给离线/录制与对照验收。
    enum class WindowBackend {
        /// 编译期装了 SDL3 就用 SDL3，否则回退 raylib（见 `default_window_backend()`）。
        automatic,
        raylib,
        sdl3,
    };

    /// 本构建的默认平台后端：装了 SDL3 后端就是 `sdl3`，否则 `raylib`。
    /// 运行时可用环境变量 `NANDINA_WINDOW_BACKEND=raylib|sdl3` 覆盖（诊断与对照用）。
    [[nodiscard]] auto default_window_backend() -> WindowBackend;
    /// 按名字解析后端（不区分大小写）；无法识别时返回 nullopt。
    [[nodiscard]] auto parse_window_backend(std::string_view name) -> std::optional<WindowBackend>;

    /// 窗口创建参数。所有字段有合理默认值, 标准桌面窗口可直接 `WindowConfig{}`。
    using WindowConfig = struct WindowConfig {
        std::string title = "Nandina";
        int width = 1100;
        int height = 700;
        int target_fps = 60;

        /// 是否显示系统标题栏 / 边框。false = 无边框 (用于自定义标题栏)。
        bool decorated = true;
        /// 是否允许用户拖拽调整窗口大小。
        bool resizable = true;
        /// 是否启用 MSAA 4x 抗锯齿。
        bool msaa = true;
        /// 是否垂直同步 (通常配合 target_fps)。
        bool vsync = true;
        /// 请求平台后端启用高 DPI 内容倍率，并分别维护窗口与 framebuffer 尺寸。
        bool high_dpi = true;

        /// 每帧清屏背景色。nullopt（默认）= 跟随当前外观的调色板背景
        /// （`palette.background`），随 light/dark 切换实时变化；
        /// 显式设置则固定为该色，不随主题变化。
        std::optional<foundation::NanColor> background;

        /// Optional fixed-design viewport. When unset, layout follows the window size directly.
        std::optional<ViewportScalePolicy> viewport;

        /// 平台后端。默认 = 本构建的默认后端（装了 SDL3 就是 SDL3）。
        WindowBackend backend = WindowBackend::automatic;
    };

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_WINDOW_CONFIG_HPP
