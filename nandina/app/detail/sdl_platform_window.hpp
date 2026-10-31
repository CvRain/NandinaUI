//
// app/detail/sdl_platform_window — SDL3 的 `IPlatformWindow` 实现（P4 的目标平台层）。
//
// 职责边界严格按 `platform_window.hpp`：窗口 + 输入翻译 + 呈现 + 剪贴板 + 用本窗口上下文
// 创建设备。它不持有场景树、不驱动帧、不认识主题 —— 那些是宿主（`NanWindow`）的事。
//
// Keyboard identities belong to foundation/input.hpp. SDL scancodes describe physical
// positions; keycodes describe layout-resolved logical keys. Press, repeat and release
// retain the initial logical identity while a physical key is held. Focus loss clears
// held identities without synthesizing releases. Modifier snapshots follow queue order.
//
// 只有在 `-Dsdl_backend=enabled|auto` 且 SDL3 >= 3.4.0 时本文件才参与编译；否则整个头文件
// 与实现都退化为空（`NANDINA_HAS_SDL_BACKEND` 由 nandina/meson.build 定义）。
//

#ifndef NANDINA_EXPERIMENT_APP_DETAIL_SDL_PLATFORM_WINDOW_HPP
#define NANDINA_EXPERIMENT_APP_DETAIL_SDL_PLATFORM_WINDOW_HPP

#if defined(NANDINA_HAS_SDL_BACKEND)

    #include "../platform_window.hpp"

    #include <SDL3/SDL.h>

    #include <array>
    #include <cstddef>
    #include <cstdint>
    #include <memory>
    #include <string>
    #include <string_view>

namespace nandina::app::detail
{
    /// SDL3 平台窗口 / 输入 / 呈现层。
    ///
    /// 生命周期：`create()` 成功后 `is_ready()` 为真；`destroy()` 必须可重复调用（析构也会
    /// 调一次）。输入与剪贴板在 `window_ == nullptr` 时全部安全退化。
    class SdlPlatformWindow final: public IPlatformWindow {
    public:
        SdlPlatformWindow() = default;
        ~SdlPlatformWindow() override;

        SdlPlatformWindow(const SdlPlatformWindow&) = delete;
        auto operator=(const SdlPlatformWindow&) -> SdlPlatformWindow& = delete;
        SdlPlatformWindow(SdlPlatformWindow&&) = delete;
        auto operator=(SdlPlatformWindow&&) -> SdlPlatformWindow& = delete;

        // ---- 生命周期 ----
        [[nodiscard]] auto create(const WindowConfig& config) -> bool override;
        void destroy() override;
        [[nodiscard]] auto is_ready() const -> bool override;
        [[nodiscard]] auto should_close() const -> bool override;

        // ---- 指标 ----
        [[nodiscard]] auto screen_size() const -> foundation::NanSize override;
        [[nodiscard]] auto drawable_size() const -> foundation::NanSize override;
        [[nodiscard]] auto frame_time() const -> float override;
        [[nodiscard]] auto display_scale() const -> float override;

        // ---- 输入 ----
        void poll_input(FrameInput& out) override;

        // ---- 呈现 ----
        void present() override;

        // ---- 剪贴板 ----
        void set_clipboard_text(std::string_view text) override;
        [[nodiscard]] auto clipboard_text() const -> std::string override;

        // ---- 设备与服务 ----
        [[nodiscard]] auto make_render_device() -> std::unique_ptr<render::IRenderDevice> override;
        /// SDL 平台目前没有图片解码路径（已知缺口）：显式返回 nullptr，消费者按接口约定退化。

        [[nodiscard]] auto name() const -> std::string_view override;

        /// Number of dispatched key events with both identities unknown.
        [[nodiscard]] auto unknown_key_count() const -> std::uint64_t;

    private:
        SDL_Window* window_ = nullptr;
        SDL_Renderer* renderer_ = nullptr;
        /// GPU 渲染器背后的设备；本类持有，`destroy()` 负责释放（渲染器不持有它）。
        SDL_GPUDevice* gpu_device_ = nullptr;
        /// `create()` 是否成功调用过 `SDL_Init`，用于让 `destroy()` 不误关别人的 SDL。
        bool sdl_initialized_ = false;
        bool ready_ = false;
        bool should_close_ = false;

        // `frame_time()` 是 const，所以这两个游标必须是 mutable。
        mutable std::uint64_t last_frame_ns_ = 0;
        mutable bool has_frame_tick_ = false;

        /// 最近一次鼠标事件的位置（窗口坐标），仅作帧尾指针状态。
        foundation::NanPoint pointer_ {};
        bool has_pointer_ = false;
        bool window_active_ = false;

        /// 按 SDL 队列顺序推进；不能在每帧开始用全局状态覆盖先前事件的修饰键。
        scene::KeyModifiers modifiers_ {};

        struct HeldKey {
            foundation::LogicalKey logical = foundation::LogicalKey::unknown;
            bool down = false;
        };
        static constexpr std::size_t physical_key_count_ =
            static_cast<std::size_t>(foundation::PhysicalKey::non_us_backslash) + 1;
        std::array<HeldKey, physical_key_count_> held_keys_ {};
        std::uint64_t unknown_keys_ = 0;

        /// vsync 关闭时 `present()` 自己数拍子用的目标帧率（0 = 不节流）。
        std::uint64_t target_fps_ = 0;
        bool pace_frames_ = false;
        std::uint64_t last_present_ns_ = 0;
    };

} // namespace nandina::app::detail

#endif // NANDINA_HAS_SDL_BACKEND

#endif // NANDINA_EXPERIMENT_APP_DETAIL_SDL_PLATFORM_WINDOW_HPP
