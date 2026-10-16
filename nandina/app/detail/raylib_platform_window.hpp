//
// app/detail/raylib_platform_window — `IPlatformWindow` 的 raylib + GLFW 实现。
//
// 这是把原先写在 `NanWindow` 里的窗口/输入/指标调用**搬出来**的结果，不是新功能：
// 行为必须与搬迁前逐项一致（关闭语义、高 DPI 尺寸、鼠标增量、按键队列、字符队列、
// 修饰键快照）。历史边界保留原样：raylib 的 `EndDrawing()` 在设备里（`begin_frame`/
// `end_frame`），所以这里的 `present()` 是 no-op。

#ifndef NANDINA_EXPERIMENT_APP_DETAIL_RAYLIB_PLATFORM_WINDOW_HPP
#define NANDINA_EXPERIMENT_APP_DETAIL_RAYLIB_PLATFORM_WINDOW_HPP

#include "../platform_window.hpp"

namespace nandina::app::detail
{
    class RaylibPlatformWindow final: public IPlatformWindow {
    public:
        RaylibPlatformWindow() = default;
        ~RaylibPlatformWindow() override;

        RaylibPlatformWindow(const RaylibPlatformWindow&) = delete;
        auto operator=(const RaylibPlatformWindow&) -> RaylibPlatformWindow& = delete;
        RaylibPlatformWindow(RaylibPlatformWindow&&) = delete;
        auto operator=(RaylibPlatformWindow&&) -> RaylibPlatformWindow& = delete;

        [[nodiscard]] auto create(const WindowConfig& config) -> bool override;
        void destroy() override;
        [[nodiscard]] auto is_ready() const -> bool override;
        [[nodiscard]] auto should_close() const -> bool override;

        [[nodiscard]] auto screen_size() const -> foundation::NanSize override;
        [[nodiscard]] auto drawable_size() const -> foundation::NanSize override;
        [[nodiscard]] auto frame_time() const -> float override;
        [[nodiscard]] auto display_scale() const -> float override;

        void poll_input(FrameInput& out) override;
        void present() override;

        void set_clipboard_text(std::string_view text) override;
        [[nodiscard]] auto clipboard_text() const -> std::string override;

        [[nodiscard]] auto make_render_device() -> std::unique_ptr<render::IRenderDevice> override;
        [[nodiscard]] auto make_image_decoder() -> std::shared_ptr<render::IImageDecoder> override;

        [[nodiscard]] auto name() const -> std::string_view override;

    private:
        bool created_ = false;
        bool has_pointer_ = false;
        float last_mouse_x_ = 0.0F;
        float last_mouse_y_ = 0.0F;
    };

} // namespace nandina::app::detail

#endif // NANDINA_EXPERIMENT_APP_DETAIL_RAYLIB_PLATFORM_WINDOW_HPP
