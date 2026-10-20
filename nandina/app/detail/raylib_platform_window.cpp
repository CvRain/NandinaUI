//
// app/detail/raylib_platform_window — 实现。行为与搬迁前一致，见头文件说明。
//

#include "raylib_platform_window.hpp"

#include "../../foundation/utf8.hpp"
#include "../../render/backends/raylib_device.hpp"

#include <raylib.h>

#include <string>

namespace nandina::app::detail
{
    namespace
    {
        /// 把 raylib 的修饰键状态快照成 KeyModifiers（搬迁前在 NanWindow 的匿名命名空间里）。
        [[nodiscard]] auto raylib_modifiers() -> scene::KeyModifiers {
            return scene::KeyModifiers {
                .shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT),
                .ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL),
                .alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT),
                .super = IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER),
            };
        }

        [[nodiscard]] auto map_mouse_button(int rl_button) -> scene::MouseButtonEvent::Button {
            switch (rl_button) {
                case MOUSE_BUTTON_RIGHT:
                    return scene::MouseButtonEvent::Button::right;
                case MOUSE_BUTTON_MIDDLE:
                    return scene::MouseButtonEvent::Button::middle;
                default:
                    return scene::MouseButtonEvent::Button::left;
            }
        }
    } // namespace

    RaylibPlatformWindow::~RaylibPlatformWindow() {
        destroy();
    }

    auto RaylibPlatformWindow::create(const WindowConfig& config) -> bool {
        if (created_) {
            return true;
        }
        unsigned int flags = 0;
        if (config.msaa) {
            flags |= FLAG_MSAA_4X_HINT;
        }
        if (config.vsync) {
            flags |= FLAG_VSYNC_HINT;
        }
        if (config.high_dpi) {
            flags |= FLAG_WINDOW_HIGHDPI;
        }
        if (!config.decorated) {
            flags |= FLAG_WINDOW_UNDECORATED;
        }
        if (config.resizable) {
            flags |= FLAG_WINDOW_RESIZABLE;
        }
        SetConfigFlags(flags);
        InitWindow(config.width, config.height, config.title.c_str());
        if (!IsWindowReady()) {
            return false;
        }
        created_ = true;
        SetExitKey(KEY_NULL);
        SetTargetFPS(config.target_fps);
        return true;
    }

    void RaylibPlatformWindow::destroy() {
        if (!created_) {
            return;
        }
        created_ = false;
        has_pointer_ = false;
        CloseWindow();
    }

    auto RaylibPlatformWindow::is_ready() const -> bool {
        return created_ && IsWindowReady();
    }

    auto RaylibPlatformWindow::should_close() const -> bool {
        return WindowShouldClose();
    }

    auto RaylibPlatformWindow::screen_size() const -> foundation::NanSize {
        return foundation::NanSize(
            static_cast<float>(GetScreenWidth()),
            static_cast<float>(GetScreenHeight())
        );
    }

    auto RaylibPlatformWindow::drawable_size() const -> foundation::NanSize {
        return foundation::NanSize(
            static_cast<float>(GetRenderWidth()),
            static_cast<float>(GetRenderHeight())
        );
    }

    auto RaylibPlatformWindow::frame_time() const -> float {
        return GetFrameTime();
    }

    auto RaylibPlatformWindow::display_scale() const -> float {
        const auto scale = GetWindowScaleDPI();
        return scale.x > 0.0F ? scale.x : 1.0F;
    }

    void RaylibPlatformWindow::poll_input(FrameInput& out) {
        const auto mx = static_cast<float>(GetMouseX());
        const auto my = static_cast<float>(GetMouseY());
        out.has_pointer = true;
        out.pointer = foundation::NanPoint {mx, my};
        out.modifiers = raylib_modifiers();

        // 鼠标移动：只在真的移动时派发（搬迁前的语义）。
        if (!has_pointer_) {
            last_mouse_x_ = mx;
            last_mouse_y_ = my;
            has_pointer_ = true;
        }
        if (mx != last_mouse_x_ || my != last_mouse_y_) {
            out.mouse_moves.emplace_back(
                foundation::NanPoint {mx, my},
                foundation::NanPoint {mx - last_mouse_x_, my - last_mouse_y_}
            );
            last_mouse_x_ = mx;
            last_mouse_y_ = my;
        }

        // 三个指针按键：raylib 的 Pressed/Released 每帧查询。
        for (const int button: {MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE}) {
            if (IsMouseButtonPressed(button)) {
                out.mouse_buttons.emplace_back(
                    map_mouse_button(button),
                    scene::MouseButtonEvent::Action::press,
                    out.pointer,
                    out.modifiers
                );
            }
            if (IsMouseButtonReleased(button)) {
                out.mouse_buttons.emplace_back(
                    map_mouse_button(button),
                    scene::MouseButtonEvent::Action::release,
                    out.pointer,
                    out.modifiers
                );
            }
        }

        if (const float wheel = GetMouseWheelMove(); wheel != 0.0F) {
            out.mouse_wheels
                .emplace_back(out.pointer, foundation::NanPoint {0.0F, wheel}, out.modifiers);
        }

        for (int key = GetKeyPressed(); key != 0; key = GetKeyPressed()) {
            out.keys.emplace_back(key, scene::KeyEvent::Action::press, out.modifiers);
        }

        for (int codepoint = GetCharPressed(); codepoint != 0; codepoint = GetCharPressed()) {
            if (codepoint >= 32) {
                out.text_inputs.emplace_back(
                    foundation::utf8::encode(static_cast<char32_t>(codepoint))
                );
            }
        }
    }

    void RaylibPlatformWindow::present() {
        // raylib 的 EndDrawing() 在设备里（render/backends/raylib_device.cpp 的 end_frame）。
        // 历史边界保持原样，见头文件说明。
    }

    void RaylibPlatformWindow::set_clipboard_text(std::string_view text) {
        const std::string owned(text);
        SetClipboardText(owned.c_str());
    }

    auto RaylibPlatformWindow::clipboard_text() const -> std::string {
        const char* text = GetClipboardText();
        return text != nullptr ? std::string(text) : std::string {};
    }

    auto RaylibPlatformWindow::make_render_device() -> std::unique_ptr<render::IRenderDevice> {
        return render::make_raylib_device();
    }

    auto RaylibPlatformWindow::name() const -> std::string_view {
        return "raylib";
    }
} // namespace nandina::app::detail
