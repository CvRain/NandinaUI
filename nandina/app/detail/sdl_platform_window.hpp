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
    #include "sdl_system_preferences.hpp"
    #include "sdl_text_input_service.hpp"

    #include <SDL3/SDL.h>

    #include <array>
    #include <cstddef>
    #include <cstdint>
    #include <memory>
    #include <string>
    #include <string_view>

namespace nandina::app::detail
{
    /// 离屏帧目标向窗口交换链提交时的 1:1 无拉伸传输计划。
    ///
    /// SDL 内置的 `GPU_RenderPresent` 会把旧尺寸 `backbuffer` 线性拉伸到新尺寸 swapchain
    /// 再重建 `backbuffer`，造成缩放帧固定尺寸组件先被放大/缩小再弹回（抽搐）。平台自己接管
    /// swapchain 后始终按逐轴最小公共尺寸 `1:1` 传输，未覆盖的右/下边缘用背景色清屏补齐。
    struct PresentTransferPlan {
        std::uint32_t copy_width = 0;
        std::uint32_t copy_height = 0;
        bool clear_uncovered_margin = false;
    };

    [[nodiscard]] constexpr auto make_present_transfer_plan(
        const std::uint32_t target_width,
        const std::uint32_t target_height,
        const std::uint32_t swapchain_width,
        const std::uint32_t swapchain_height
    ) noexcept -> PresentTransferPlan {
        if (swapchain_width == 0U || swapchain_height == 0U) {
            return PresentTransferPlan {};
        }
        if (target_width == 0U || target_height == 0U) {
            return PresentTransferPlan {
                .copy_width = 0U,
                .copy_height = 0U,
                .clear_uncovered_margin = true,
            };
        }
        const std::uint32_t copy_width =
            target_width < swapchain_width ? target_width : swapchain_width;
        const std::uint32_t copy_height =
            target_height < swapchain_height ? target_height : swapchain_height;
        return PresentTransferPlan {
            .copy_width = copy_width,
            .copy_height = copy_height,
            .clear_uncovered_margin =
                swapchain_width > copy_width || swapchain_height > copy_height,
        };
    }

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
        void prepare_frame(const foundation::NanColor& clear_color) override;
        void present() override;

        // ---- 剪贴板 ----
        void set_clipboard_text(std::string_view text) override;
        [[nodiscard]] auto clipboard_text() const -> std::string override;

        // ---- 设备与服务 ----
        [[nodiscard]] auto make_render_device() -> std::unique_ptr<render::IRenderDevice> override;
        /// SDL 平台目前没有图片解码路径（已知缺口）：显式返回 nullptr，消费者按接口约定退化。

        [[nodiscard]] auto name() const -> std::string_view override;

        [[nodiscard]] auto text_input_service() -> scene::ITextInputService* override;

        /// 提供 OS 偏好来源（跨平台 SDL3 系统亮暗外观 + Linux/Windows 平台无障碍设置）。
        [[nodiscard]] auto system_preferences_source() -> ISystemPreferencesSource* override;

        /// Number of dispatched key events with both identities unknown.
        [[nodiscard]] auto unknown_key_count() const -> std::uint64_t;

        /// White-box accessors for SDL platform integration tests (`detail` namespace only).
        [[nodiscard]] auto native_window() const noexcept -> SDL_Window* {
            return window_;
        }
        [[nodiscard]] auto native_renderer() const noexcept -> SDL_Renderer* {
            return renderer_;
        }

    private:
        /// 逻辑（作者/布局）单位到 SDL 原生窗口坐标的内容缩放；`high_dpi=false` 时恒为 1。
        [[nodiscard]] auto content_scale() const noexcept -> float;
        /// 依据当前窗口的 display scale / pixel density 重算内容缩放、渲染 scale 与输入区比例。
        void refresh_content_scale();
        /// 把渲染器 scale 设为物理像素 / 逻辑单位（1× 模式下恒为 1）。
        void apply_render_scale();
        /// 按 `logical_size_ × content_scale()` 恢复原生窗口尺寸（窗口被缩放到其他 DPI 后）。
        void apply_logical_window_size();
        /// 把窗口收进当前显示器的可用区域：内容缩放后的窗口可能超出按原始尺寸放置的位置。
        void ensure_window_visible();
        /// 系统/跨屏缩放变化：重算缩放并恢复作者请求的逻辑窗口尺寸。
        void handle_display_scale_changed();
        /// 用户 resize 后回写逻辑尺寸，供后续缩放变化恢复。
        void remember_logical_size_from_window();
        /// GPU 交换链路径：在绘制前确保离屏目标纹理与当前窗口像素尺寸一致并已绑定。
        [[nodiscard]] auto ensure_gpu_frame_target() -> bool;
        /// 当前窗口所在显示器的刷新周期（纳秒）；无法查询时回退 60Hz。
        [[nodiscard]] auto display_refresh_interval_ns() const noexcept -> std::uint64_t;

        SDL_Window* window_ = nullptr;
        SDL_Renderer* renderer_ = nullptr;
        /// GPU 渲染器背后的设备；本类持有，`destroy()` 负责释放（渲染器不持有它）。
        SDL_GPUDevice* gpu_device_ = nullptr;
        /// GPU 路径下由平台显式持有的离屏帧目标纹理（绘制前按最新像素尺寸对齐）。
        SDL_Texture* frame_target_ = nullptr;
        SDL_GPUTexture* frame_target_gpu_ = nullptr;
        SDL_PixelFormat frame_target_format_ = SDL_PIXELFORMAT_RGBA32;
        int frame_target_width_ = 0;
        int frame_target_height_ = 0;
        bool gpu_swapchain_claimed_ = false;
        bool vsync_enabled_ = true;
        SDL_GPUPresentMode steady_present_mode_ = SDL_GPU_PRESENTMODE_VSYNC;
        SDL_GPUPresentMode resize_present_mode_ = SDL_GPU_PRESENTMODE_VSYNC;
        SDL_GPUPresentMode active_present_mode_ = SDL_GPU_PRESENTMODE_VSYNC;
        std::uint64_t last_resize_ns_ = 0;
        foundation::NanColor clear_color_ {};
        /// `create()` 是否成功调用过 `SDL_Init`，用于让 `destroy()` 不误关别人的 SDL。
        bool sdl_initialized_ = false;
        bool ready_ = false;
        bool should_close_ = false;
        /// `WindowConfig::high_dpi`：false = 固定 1×（不按 OS 内容缩放创建/映射/渲染）。
        bool high_dpi_ = false;
        /// 物理像素 / 逻辑单位（Windows 175% = 1.75；1× 模式 = 1）。
        float content_scale_ = 1.0F;
        /// 作者/布局单位下的窗口尺寸；resize 与缩放变化时维护。
        foundation::NanSize logical_size_ {};

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
        std::unique_ptr<SdlTextInputService> text_input_service_;
        /// OS 偏好来源（SDL3 系统外观 + 平台无障碍文字缩放/减弱动效）。
        std::unique_ptr<SdlSystemPreferencesSource> system_preferences_;
        bool pace_frames_ = false;
        std::uint64_t last_present_ns_ = 0;
    };

} // namespace nandina::app::detail

#endif // NANDINA_HAS_SDL_BACKEND

#endif // NANDINA_EXPERIMENT_APP_DETAIL_SDL_PLATFORM_WINDOW_HPP
