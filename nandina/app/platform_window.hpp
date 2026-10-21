//
// app/platform_window — 窗口 / 输入 / 帧驱动的平台抽象（P4 的核心 seam）。
//
// 为什么需要它：`NanWindow` 原先同时持有窗口 API、输入轮询、帧驱动与设备创建
// （raylib + GLFW 写死在 .cpp 里）。这带来两个后果，都在探测里撞到过：
//
//   1. **设备不可注入** —— raylib 设备的调用序列无法被包装或插桩，所以跨后端对照只能
//      做像素级，做不了逐图元 trace（探测记录 §2.3.2 的 P4 限制）。
//   2. **平台行为与框架行为混在一起** —— 哪些是「宿主必须做的事」（每帧 drain
//      UiDispatcher、每帧 process(dt) 推进动画、谁画应用背景）只能靠读 575 行实现才知道。
//      探测记录 §2.3.4 就是这么挖出两条必备契约的。
//
// 拆分边界：平台层只负责「窗口 + 输入翻译 + 呈现 + 剪贴板 + 用本窗口上下文创建设备」，
// 框架层（场景树、布局、渲染遍历、主题、路由、浮层）完全不感知平台。输入用 `scene::`
// 的事件类型承载，所以平台层不需要认识场景树，宿主也不需要认识平台的事件模型。

#ifndef NANDINA_EXPERIMENT_APP_PLATFORM_WINDOW_HPP
#define NANDINA_EXPERIMENT_APP_PLATFORM_WINDOW_HPP

#include "../foundation/geometry.hpp"
#include "../render/render_device.hpp"
#include "../scene/input_event.hpp"
#include "window_config.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::app
{

    /// 平台层翻译好的一帧输入。
    ///
    /// 容器语义：**追加**而不是替换 —— 宿主每帧先清空自己的缓冲，平台实现只负责把这一帧
    /// 发生的事件翻译进去。指针位置与修饰键单独给出，因为它们每帧都要读（而不只是变化时）。
    struct FrameInput {
        std::vector<scene::MouseMoveEvent> mouse_moves;
        std::vector<scene::MouseButtonEvent> mouse_buttons;
        std::vector<scene::MouseWheelEvent> mouse_wheels;
        std::vector<scene::KeyEvent> keys;
        std::vector<scene::TextInputEvent> text_inputs;

        /// 指针是否有效（无指针设备时 false）。
        bool has_pointer = false;
        /// 屏幕坐标（物理像素，未做 viewport 映射；映射属于宿主职责）。
        foundation::NanPoint pointer {};
        /// 当前修饰键状态（每帧读，不只在变化时）。
        scene::KeyModifiers modifiers {};
    };

    /// 一个平台实现 = 窗口 + 输入 + 呈现 + 剪贴板 + 匹配的绘制设备。
    ///
    /// 内置实现见 `detail/sdl_platform_window.*`。
    class IPlatformWindow {
    public:
        virtual ~IPlatformWindow() = default;

        // ---- 生命周期 ----
        /// 创建窗口。返回 false 表示平台拒绝（例如没有可用显示设备），调用方负责清理。
        [[nodiscard]] virtual auto create(const WindowConfig& config) -> bool = 0;
        virtual void destroy() = 0;
        [[nodiscard]] virtual auto is_ready() const -> bool = 0;
        /// 平台侧的关闭请求（点标题栏关闭、Alt+F4 等）。宿主的 `request_close()` 不经过这里。
        [[nodiscard]] virtual auto should_close() const -> bool = 0;

        // ---- 指标 ----
        /// 逻辑窗口尺寸（布局用）。
        [[nodiscard]] virtual auto screen_size() const -> foundation::NanSize = 0;
        /// framebuffer / 可绘制尺寸（高 DPI 下与 screen_size 不同）。
        [[nodiscard]] virtual auto drawable_size() const -> foundation::NanSize = 0;
        [[nodiscard]] virtual auto frame_time() const -> float = 0;
        /// 显示缩放（DPI）。无法查询时返回 1.0。
        [[nodiscard]] virtual auto display_scale() const -> float = 0;

        // ---- 输入 ----
        /// 把这一帧的平台输入翻译进 `out`（追加语义，见 FrameInput）。
        virtual void poll_input(FrameInput& out) = 0;

        // ---- 呈现 ----
        /// 呈现这一帧；设备不持有 swap。
        virtual void present() = 0;

        // ---- 剪贴板 ----
        virtual void set_clipboard_text(std::string_view text) = 0;
        [[nodiscard]] virtual auto clipboard_text() const -> std::string = 0;

        // ---- 设备与服务 ----
        /// 用本平台的窗口上下文创建绘制设备。
        [[nodiscard]] virtual auto make_render_device()
            -> std::unique_ptr<render::IRenderDevice> = 0;
        /// 平台名（诊断与能力报告用），例如 "sdl3"。
        [[nodiscard]] virtual auto name() const -> std::string_view = 0;
    };

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_PLATFORM_WINDOW_HPP
