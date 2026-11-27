//
// app/detail/sdl_platform_window — SDL3 的 `IPlatformWindow` 实现。设计说明见同名头文件。
//

#include "sdl_platform_window.hpp"

#if defined(NANDINA_HAS_SDL_BACKEND)

    #include "../../foundation/nandina_color_space.hpp"
    #include "../../render/backends/sdl_device.hpp"
    #include "../window_metrics.hpp"
    #if defined(__linux__)
        #include "linux_system_preferences.hpp"
    #endif
    #include "windows_system_text_scale.hpp"

    #include <SDL3/SDL_gpu.h>
    #include <SDL3/SDL_render.h>

    #include <algorithm>
    #include <cmath>
    #include <optional>
    #include <string>

namespace nandina::app::detail
{
    namespace
    {
        [[nodiscard]] auto translate_physical_key(const SDL_Scancode code)
            -> foundation::PhysicalKey {
            using Key = foundation::PhysicalKey;
            switch (code) {
                case SDL_SCANCODE_A:
                    return Key::a;
                case SDL_SCANCODE_B:
                    return Key::b;
                case SDL_SCANCODE_C:
                    return Key::c;
                case SDL_SCANCODE_D:
                    return Key::d;
                case SDL_SCANCODE_E:
                    return Key::e;
                case SDL_SCANCODE_F:
                    return Key::f;
                case SDL_SCANCODE_G:
                    return Key::g;
                case SDL_SCANCODE_H:
                    return Key::h;
                case SDL_SCANCODE_I:
                    return Key::i;
                case SDL_SCANCODE_J:
                    return Key::j;
                case SDL_SCANCODE_K:
                    return Key::k;
                case SDL_SCANCODE_L:
                    return Key::l;
                case SDL_SCANCODE_M:
                    return Key::m;
                case SDL_SCANCODE_N:
                    return Key::n;
                case SDL_SCANCODE_O:
                    return Key::o;
                case SDL_SCANCODE_P:
                    return Key::p;
                case SDL_SCANCODE_Q:
                    return Key::q;
                case SDL_SCANCODE_R:
                    return Key::r;
                case SDL_SCANCODE_S:
                    return Key::s;
                case SDL_SCANCODE_T:
                    return Key::t;
                case SDL_SCANCODE_U:
                    return Key::u;
                case SDL_SCANCODE_V:
                    return Key::v;
                case SDL_SCANCODE_W:
                    return Key::w;
                case SDL_SCANCODE_X:
                    return Key::x;
                case SDL_SCANCODE_Y:
                    return Key::y;
                case SDL_SCANCODE_Z:
                    return Key::z;
                case SDL_SCANCODE_0:
                    return Key::num_0;
                case SDL_SCANCODE_1:
                    return Key::num_1;
                case SDL_SCANCODE_2:
                    return Key::num_2;
                case SDL_SCANCODE_3:
                    return Key::num_3;
                case SDL_SCANCODE_4:
                    return Key::num_4;
                case SDL_SCANCODE_5:
                    return Key::num_5;
                case SDL_SCANCODE_6:
                    return Key::num_6;
                case SDL_SCANCODE_7:
                    return Key::num_7;
                case SDL_SCANCODE_8:
                    return Key::num_8;
                case SDL_SCANCODE_9:
                    return Key::num_9;
                case SDL_SCANCODE_SPACE:
                    return Key::space;
                case SDL_SCANCODE_APOSTROPHE:
                    return Key::apostrophe;
                case SDL_SCANCODE_COMMA:
                    return Key::comma;
                case SDL_SCANCODE_MINUS:
                    return Key::minus;
                case SDL_SCANCODE_PERIOD:
                    return Key::period;
                case SDL_SCANCODE_SLASH:
                    return Key::slash;
                case SDL_SCANCODE_SEMICOLON:
                    return Key::semicolon;
                case SDL_SCANCODE_EQUALS:
                    return Key::equal;
                case SDL_SCANCODE_LEFTBRACKET:
                    return Key::left_bracket;
                case SDL_SCANCODE_BACKSLASH:
                    return Key::backslash;
                case SDL_SCANCODE_RIGHTBRACKET:
                    return Key::right_bracket;
                case SDL_SCANCODE_GRAVE:
                    return Key::grave_accent;
                case SDL_SCANCODE_ESCAPE:
                    return Key::escape;
                case SDL_SCANCODE_RETURN:
                    return Key::enter;
                case SDL_SCANCODE_TAB:
                    return Key::tab;
                case SDL_SCANCODE_BACKSPACE:
                    return Key::backspace;
                case SDL_SCANCODE_INSERT:
                    return Key::insert;
                case SDL_SCANCODE_DELETE:
                    return Key::delete_key;
                case SDL_SCANCODE_RIGHT:
                    return Key::right;
                case SDL_SCANCODE_LEFT:
                    return Key::left;
                case SDL_SCANCODE_DOWN:
                    return Key::down;
                case SDL_SCANCODE_UP:
                    return Key::up;
                case SDL_SCANCODE_PAGEUP:
                    return Key::page_up;
                case SDL_SCANCODE_PAGEDOWN:
                    return Key::page_down;
                case SDL_SCANCODE_HOME:
                    return Key::home;
                case SDL_SCANCODE_END:
                    return Key::end;
                case SDL_SCANCODE_F1:
                    return Key::f1;
                case SDL_SCANCODE_F2:
                    return Key::f2;
                case SDL_SCANCODE_F3:
                    return Key::f3;
                case SDL_SCANCODE_F4:
                    return Key::f4;
                case SDL_SCANCODE_F5:
                    return Key::f5;
                case SDL_SCANCODE_F6:
                    return Key::f6;
                case SDL_SCANCODE_F7:
                    return Key::f7;
                case SDL_SCANCODE_F8:
                    return Key::f8;
                case SDL_SCANCODE_F9:
                    return Key::f9;
                case SDL_SCANCODE_F10:
                    return Key::f10;
                case SDL_SCANCODE_F11:
                    return Key::f11;
                case SDL_SCANCODE_F12:
                    return Key::f12;
                case SDL_SCANCODE_F13:
                    return Key::f13;
                case SDL_SCANCODE_F14:
                    return Key::f14;
                case SDL_SCANCODE_F15:
                    return Key::f15;
                case SDL_SCANCODE_F16:
                    return Key::f16;
                case SDL_SCANCODE_F17:
                    return Key::f17;
                case SDL_SCANCODE_F18:
                    return Key::f18;
                case SDL_SCANCODE_F19:
                    return Key::f19;
                case SDL_SCANCODE_F20:
                    return Key::f20;
                case SDL_SCANCODE_F21:
                    return Key::f21;
                case SDL_SCANCODE_F22:
                    return Key::f22;
                case SDL_SCANCODE_F23:
                    return Key::f23;
                case SDL_SCANCODE_F24:
                    return Key::f24;
                case SDL_SCANCODE_APPLICATION:
                    return Key::menu;
                case SDL_SCANCODE_LSHIFT:
                    return Key::left_shift;
                case SDL_SCANCODE_RSHIFT:
                    return Key::right_shift;
                case SDL_SCANCODE_LCTRL:
                    return Key::left_ctrl;
                case SDL_SCANCODE_RCTRL:
                    return Key::right_ctrl;
                case SDL_SCANCODE_LALT:
                    return Key::left_alt;
                case SDL_SCANCODE_RALT:
                    return Key::right_alt;
                case SDL_SCANCODE_LGUI:
                    return Key::left_super;
                case SDL_SCANCODE_RGUI:
                    return Key::right_super;
                case SDL_SCANCODE_CAPSLOCK:
                    return Key::caps_lock;
                case SDL_SCANCODE_NUMLOCKCLEAR:
                    return Key::num_lock;
                case SDL_SCANCODE_SCROLLLOCK:
                    return Key::scroll_lock;
                case SDL_SCANCODE_PRINTSCREEN:
                    return Key::print_screen;
                case SDL_SCANCODE_PAUSE:
                    return Key::pause;
                case SDL_SCANCODE_KP_0:
                    return Key::keypad_0;
                case SDL_SCANCODE_KP_1:
                    return Key::keypad_1;
                case SDL_SCANCODE_KP_2:
                    return Key::keypad_2;
                case SDL_SCANCODE_KP_3:
                    return Key::keypad_3;
                case SDL_SCANCODE_KP_4:
                    return Key::keypad_4;
                case SDL_SCANCODE_KP_5:
                    return Key::keypad_5;
                case SDL_SCANCODE_KP_6:
                    return Key::keypad_6;
                case SDL_SCANCODE_KP_7:
                    return Key::keypad_7;
                case SDL_SCANCODE_KP_8:
                    return Key::keypad_8;
                case SDL_SCANCODE_KP_9:
                    return Key::keypad_9;
                case SDL_SCANCODE_KP_ENTER:
                    return Key::keypad_enter;
                case SDL_SCANCODE_KP_PLUS:
                    return Key::keypad_add;
                case SDL_SCANCODE_KP_MINUS:
                    return Key::keypad_subtract;
                case SDL_SCANCODE_KP_MULTIPLY:
                    return Key::keypad_multiply;
                case SDL_SCANCODE_KP_DIVIDE:
                    return Key::keypad_divide;
                case SDL_SCANCODE_KP_PERIOD:
                    return Key::keypad_decimal;
                case SDL_SCANCODE_KP_EQUALS:
                    return Key::keypad_equal;
                case SDL_SCANCODE_KP_COMMA:
                    return Key::keypad_comma;
                case SDL_SCANCODE_NONUSHASH:
                    return Key::non_us_hash;
                case SDL_SCANCODE_NONUSBACKSLASH:
                    return Key::non_us_backslash;
                case SDL_SCANCODE_MENU:
                    return Key::menu;
                case SDL_SCANCODE_KP_EQUALSAS400:
                    return Key::keypad_equal;
                default:
                    return Key::unknown;
            }
        }

        [[nodiscard]] auto translate_logical_key(const SDL_Keycode code, const SDL_Keymod mods)
            -> foundation::LogicalKey {
            using Key = foundation::LogicalKey;
            const bool num_lock = (mods & SDL_KMOD_NUM) != 0;
            switch (code) {
                case SDLK_ESCAPE:
                    return Key::escape;
                case SDLK_RETURN:
                    return Key::enter;
                case SDLK_TAB:
                    return Key::tab;
                case SDLK_BACKSPACE:
                    return Key::backspace;
                case SDLK_INSERT:
                    return Key::insert;
                case SDLK_DELETE:
                    return Key::delete_key;
                case SDLK_RIGHT:
                    return Key::right;
                case SDLK_LEFT:
                    return Key::left;
                case SDLK_DOWN:
                    return Key::down;
                case SDLK_UP:
                    return Key::up;
                case SDLK_PAGEUP:
                    return Key::page_up;
                case SDLK_PAGEDOWN:
                    return Key::page_down;
                case SDLK_HOME:
                    return Key::home;
                case SDLK_END:
                    return Key::end;
                case SDLK_F1:
                    return Key::f1;
                case SDLK_F2:
                    return Key::f2;
                case SDLK_F3:
                    return Key::f3;
                case SDLK_F4:
                    return Key::f4;
                case SDLK_F5:
                    return Key::f5;
                case SDLK_F6:
                    return Key::f6;
                case SDLK_F7:
                    return Key::f7;
                case SDLK_F8:
                    return Key::f8;
                case SDLK_F9:
                    return Key::f9;
                case SDLK_F10:
                    return Key::f10;
                case SDLK_F11:
                    return Key::f11;
                case SDLK_F12:
                    return Key::f12;
                case SDLK_F13:
                    return Key::f13;
                case SDLK_F14:
                    return Key::f14;
                case SDLK_F15:
                    return Key::f15;
                case SDLK_F16:
                    return Key::f16;
                case SDLK_F17:
                    return Key::f17;
                case SDLK_F18:
                    return Key::f18;
                case SDLK_F19:
                    return Key::f19;
                case SDLK_F20:
                    return Key::f20;
                case SDLK_F21:
                    return Key::f21;
                case SDLK_F22:
                    return Key::f22;
                case SDLK_F23:
                    return Key::f23;
                case SDLK_F24:
                    return Key::f24;
                case SDLK_APPLICATION:
                    return Key::menu;
                case SDLK_LSHIFT:
                    return Key::left_shift;
                case SDLK_RSHIFT:
                    return Key::right_shift;
                case SDLK_LCTRL:
                    return Key::left_ctrl;
                case SDLK_RCTRL:
                    return Key::right_ctrl;
                case SDLK_LALT:
                    return Key::left_alt;
                case SDLK_RALT:
                    return Key::right_alt;
                case SDLK_LGUI:
                    return Key::left_super;
                case SDLK_RGUI:
                    return Key::right_super;
                case SDLK_CAPSLOCK:
                    return Key::caps_lock;
                case SDLK_NUMLOCKCLEAR:
                    return Key::num_lock;
                case SDLK_SCROLLLOCK:
                    return Key::scroll_lock;
                case SDLK_PRINTSCREEN:
                    return Key::print_screen;
                case SDLK_PAUSE:
                    return Key::pause;
                case SDLK_MENU:
                    return Key::menu;
                case SDLK_RETURN2:
                case SDLK_KP_ENTER:
                    return Key::enter;
                case SDLK_KP_PLUS:
                    return static_cast<Key>('+');
                case SDLK_KP_MINUS:
                    return Key::minus;
                case SDLK_KP_MULTIPLY:
                    return static_cast<Key>('*');
                case SDLK_KP_DIVIDE:
                    return Key::slash;
                case SDLK_KP_EQUALS:
                case SDLK_KP_EQUALSAS400:
                    return Key::equal;
                case SDLK_KP_COMMA:
                    return Key::comma;
                case SDLK_KP_0:
                    return num_lock ? Key::num_0 : Key::insert;
                case SDLK_KP_1:
                    return num_lock ? Key::num_1 : Key::end;
                case SDLK_KP_2:
                    return num_lock ? Key::num_2 : Key::down;
                case SDLK_KP_3:
                    return num_lock ? Key::num_3 : Key::page_down;
                case SDLK_KP_4:
                    return num_lock ? Key::num_4 : Key::left;
                case SDLK_KP_5:
                    return num_lock ? Key::num_5 : Key::unknown;
                case SDLK_KP_6:
                    return num_lock ? Key::num_6 : Key::right;
                case SDLK_KP_7:
                    return num_lock ? Key::num_7 : Key::home;
                case SDLK_KP_8:
                    return num_lock ? Key::num_8 : Key::up;
                case SDLK_KP_9:
                    return num_lock ? Key::num_9 : Key::page_up;
                case SDLK_KP_PERIOD:
                    return num_lock ? Key::period : Key::delete_key;
                default:
                    // SDL character keycodes are Unicode, not scancode-derived keycodes.
                    if (code >= 0x20 && code <= 0x10FFFF && !(code >= 0xD800 && code <= 0xDFFF)
                        && !(code >= 0x7F && code <= 0x9F))
                    {
                        return static_cast<Key>(code);
                    }
                    return Key::unknown;
            }
        }

        /// SDL 的三键 -> `scene::MouseButtonEvent::Button`。其余键（X1/X2）返回 nullopt。
        [[nodiscard]] auto translate_button(const std::uint8_t button)
            -> std::optional<scene::MouseButtonEvent::Button> {
            switch (button) {
                case SDL_BUTTON_LEFT:
                    return scene::MouseButtonEvent::Button::left;
                case SDL_BUTTON_MIDDLE:
                    return scene::MouseButtonEvent::Button::middle;
                case SDL_BUTTON_RIGHT:
                    return scene::MouseButtonEvent::Button::right;
                default:
                    return std::nullopt;
            }
        }

        /// 将 SDL 事件或边界状态翻译为框架修饰键快照。
        [[nodiscard]] auto translate_modifiers(const SDL_Keymod mods) -> scene::KeyModifiers {
            return scene::KeyModifiers {
                .shift = (mods & SDL_KMOD_SHIFT) != 0,
                .ctrl = (mods & SDL_KMOD_CTRL) != 0,
                .alt = (mods & (SDL_KMOD_ALT | SDL_KMOD_MODE)) != 0,
                .super = (mods & SDL_KMOD_GUI) != 0,
            };
        }
    } // namespace

    SdlPlatformWindow::~SdlPlatformWindow() {
        destroy();
    }

    auto SdlPlatformWindow::create(const WindowConfig& config) -> bool {
        if (ready_) {
            return true;
        }

        // EditableText paints preedit from SDL_EVENT_TEXT_EDITING. Declare that capability
        // before input contexts are created; native candidate UI remains platform-owned.
        // Environment/host hints can explicitly override this framework default.
        SDL_SetHintWithPriority(SDL_HINT_IME_IMPLEMENTED_UI, "composition", SDL_HINT_DEFAULT);
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            SDL_Log("SdlPlatformWindow: SDL_Init(SDL_INIT_VIDEO) failed: %s", SDL_GetError());
            return false;
        }
        sdl_initialized_ = true;

        SDL_WindowFlags flags = 0;
        if (config.resizable) {
            flags |= SDL_WINDOW_RESIZABLE;
        }
        if (config.high_dpi) {
            flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
        }
        // `decorated` 不在任务的必选字段里，但它是 `WindowConfig` 的既有语义（自定义标题栏
        // 需要无边框），映射到 SDL 是无成本的。
        if (!config.decorated) {
            flags |= SDL_WINDOW_BORDERLESS;
        }

        window_ = SDL_CreateWindow(config.title.c_str(), config.width, config.height, flags);
        if (window_ == nullptr) {
            SDL_Log("SdlPlatformWindow: SDL_CreateWindow failed: %s", SDL_GetError());
            destroy();
            return false;
        }

        // `WindowConfig::width/height` 是逻辑（作者/布局）单位，而 SDL 的窗口尺寸是原生窗口
        // 坐标：Windows 上就是物理像素（175% 时 1 逻辑单位 = 1.75 像素），Retina 上是点。
        // 创建时按内容缩放把原生窗口放大到逻辑尺寸对应的物理尺寸；之后布局、输入与文本都
        // 留在逻辑空间，只有渲染器的 scale 负责到物理像素的最后一步。
        high_dpi_ = config.high_dpi;
        logical_size_ = foundation::NanSize(
            static_cast<float>(config.width),
            static_cast<float>(config.height)
        );
        refresh_content_scale();
        apply_logical_window_size();
        ensure_window_visible();

        // SDL3 默认不投递文本输入（`SDL_StartTextInput` 文档：Text input events are not
        // received by default）。缺这一步时键盘事件照常到达，但 `SDL_EVENT_TEXT_INPUT`
        // 永远为空，TextField/TextArea 只能删除和移动光标，不能键入字符。
        // 文本输入是窗口级开关，与具体焦点目标无关；焦点是否可编辑仍由场景派发决定。
        if (!SDL_StartTextInput(window_)) {
            SDL_Log("SdlPlatformWindow: SDL_StartTextInput failed: %s", SDL_GetError());
        }
        text_input_service_ = std::make_unique<SdlTextInputService>(window_);

        // 渲染器：优先走由平台直接接管 swapchain 的无窗口 GPU 渲染器（`SDL_CreateGPURenderer(gpu,
        // nullptr)` + 显式 `SDL_TEXTUREACCESS_TARGET` 离屏帧目标），避免 SDL 内置
        // `GPU_RenderPresent` 在缩放帧先把上一尺寸 `backbuffer` 线性拉伸到新尺寸 swapchain、提交后
        // 才重建 `backbuffer` 造成的固定组件抽搐；同时允许在连续缩放期临时切到 `MAILBOX` 消除
        // Wayland `wp_fifo_v1` 队列积压。无窗口表面（如 dummy 驱动）上 `ClaimWindow` 失败则退回
        // 默认 2D 渲染器。
        gpu_device_ = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
        if (gpu_device_ != nullptr) {
            if (SDL_ClaimWindowForGPUDevice(gpu_device_, window_)) {
                gpu_swapchain_claimed_ = true;
                renderer_ = SDL_CreateGPURenderer(gpu_device_, nullptr);
                if (renderer_ == nullptr) {
                    SDL_ReleaseWindowFromGPUDevice(gpu_device_, window_);
                    gpu_swapchain_claimed_ = false;
                }
            }
            if (renderer_ == nullptr) {
                SDL_DestroyGPUDevice(gpu_device_);
                gpu_device_ = nullptr;
            }
        }
        if (gpu_swapchain_claimed_ && renderer_ != nullptr) {
            const auto choose_non_vsync_mode = [](SDL_GPUDevice* dev, SDL_Window* win) {
                if (SDL_WindowSupportsGPUPresentMode(dev, win, SDL_GPU_PRESENTMODE_MAILBOX)) {
                    return SDL_GPU_PRESENTMODE_MAILBOX;
                }
                if (SDL_WindowSupportsGPUPresentMode(dev, win, SDL_GPU_PRESENTMODE_IMMEDIATE)) {
                    return SDL_GPU_PRESENTMODE_IMMEDIATE;
                }
                return SDL_GPU_PRESENTMODE_VSYNC;
            };
            vsync_enabled_ = config.vsync;
            if (config.vsync) {
                steady_present_mode_ = SDL_GPU_PRESENTMODE_VSYNC;
                if (SDL_WindowSupportsGPUPresentMode(
                        gpu_device_,
                        window_,
                        SDL_GPU_PRESENTMODE_MAILBOX
                    ))
                {
                    resize_present_mode_ = SDL_GPU_PRESENTMODE_MAILBOX;
                }
                else if (
                    const char* video_driver = SDL_GetCurrentVideoDriver(); video_driver != nullptr
                    && std::string_view(video_driver) == "wayland"
                    && SDL_WindowSupportsGPUPresentMode(gpu_device_,
                                                        window_,
                                                        SDL_GPU_PRESENTMODE_IMMEDIATE)
                )
                {
                    resize_present_mode_ = SDL_GPU_PRESENTMODE_IMMEDIATE;
                }
                else {
                    resize_present_mode_ = SDL_GPU_PRESENTMODE_VSYNC;
                }
            }
            else {
                const auto non_vsync = choose_non_vsync_mode(gpu_device_, window_);
                steady_present_mode_ = non_vsync;
                resize_present_mode_ = non_vsync;
            }
            (void)SDL_SetGPUAllowedFramesInFlight(gpu_device_, 1);
            if (SDL_SetGPUSwapchainParameters(
                    gpu_device_,
                    window_,
                    SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                    steady_present_mode_
                ))
            {
                active_present_mode_ = steady_present_mode_;
            }
            else {
                active_present_mode_ = SDL_GPU_PRESENTMODE_VSYNC;
                SDL_Log(
                    "SdlPlatformWindow: SDL_SetGPUSwapchainParameters(%d) failed: %s",
                    static_cast<int>(steady_present_mode_),
                    SDL_GetError()
                );
            }

            const SDL_GPUTextureFormat swap_format =
                SDL_GetGPUSwapchainTextureFormat(gpu_device_, window_);
            const SDL_PixelFormat mapped_format =
                SDL_GetPixelFormatFromGPUTextureFormat(swap_format);
            frame_target_format_ =
                (mapped_format == SDL_PIXELFORMAT_BGRA32 || mapped_format == SDL_PIXELFORMAT_RGBA32
                 || mapped_format == SDL_PIXELFORMAT_BGRX32
                 || mapped_format == SDL_PIXELFORMAT_RGBX32)
                ? mapped_format
                : SDL_PIXELFORMAT_RGBA32;

            if (!ensure_gpu_frame_target()) {
                SDL_DestroyRenderer(renderer_);
                renderer_ = nullptr;
                SDL_ReleaseWindowFromGPUDevice(gpu_device_, window_);
                gpu_swapchain_claimed_ = false;
                SDL_DestroyGPUDevice(gpu_device_);
                gpu_device_ = nullptr;
            }
        }
        if (renderer_ == nullptr) {
            renderer_ = SDL_CreateRenderer(window_, nullptr);
        }
        if (renderer_ == nullptr) {
            SDL_Log(
                "SdlPlatformWindow: no usable SDL renderer (GPU and default both failed): %s",
                SDL_GetError()
            );
            destroy();
            return false;
        }

        // 渲染器 scale 与文本输入区域换算依赖窗口与 renderer 都存在；创建路径末尾统一应用。
        refresh_content_scale();

        // vsync / target_fps：非 GPU 路径走 SDL_SetRenderVSync；GPU 路径已通过
        // SDL_SetGPUSwapchainParameters 配置，target_fps 由 `present()` 在 vsync 关闭时自己数拍子。
        vsync_enabled_ = config.vsync;
        if (!gpu_swapchain_claimed_) {
            const int vsync = config.vsync ? 1 : 0;
            if (!SDL_SetRenderVSync(renderer_, vsync)) {
                SDL_Log(
                    "SdlPlatformWindow: SDL_SetRenderVSync(%d) failed: %s",
                    vsync,
                    SDL_GetError()
                );
            }
        }
        target_fps_ = config.target_fps > 0 ? static_cast<std::uint64_t>(config.target_fps) : 0;
        pace_frames_ = !config.vsync && target_fps_ > 0;
        last_present_ns_ = 0;
        last_resize_ns_ = 0;
        clear_color_ = config.background.value_or(foundation::NanColor {});
        if (pace_frames_) {
            SDL_Log(
                "SdlPlatformWindow: vsync off, pacing present() to target_fps=%d",
                config.target_fps
            );
        }

        should_close_ = false;
        has_pointer_ = false;
        pointer_ = foundation::NanPoint::zero();
        window_active_ = (SDL_GetWindowFlags(window_) & SDL_WINDOW_INPUT_FOCUS) != 0;
        modifiers_ = translate_modifiers(SDL_GetModState());
        has_frame_tick_ = false;
        last_frame_ns_ = 0;
        ready_ = true;

        const auto* renderer_name = SDL_GetRendererName(renderer_);
        int native_width = 0;
        int native_height = 0;
        int pixel_width = 0;
        int pixel_height = 0;
        (void)SDL_GetWindowSize(window_, &native_width, &native_height);
        (void)SDL_GetWindowSizeInPixels(window_, &pixel_width, &pixel_height);
        SDL_Log(
            "SdlPlatformWindow: ready %dx%d logical (%dx%d native, %dx%d pixels), "
            "content_scale=%.2f renderer=%s",
            config.width,
            config.height,
            native_width,
            native_height,
            pixel_width,
            pixel_height,
            content_scale(),
            renderer_name != nullptr ? renderer_name : "unknown"
        );

        // OS 偏好来源：SDL3 原生系统亮暗外观（事件驱动，Linux/Windows 通用）叠加平台无障碍
        // 文字缩放与减弱动效来源。
        std::unique_ptr<ISystemPreferencesSource> platform_preferences;
    #if defined(_WIN32)
        platform_preferences = std::make_unique<WindowsSystemTextScaleSource>();
    #elif defined(__linux__)
        platform_preferences = std::make_unique<LinuxSystemPreferencesSource>();
    #endif
        system_preferences_ =
            std::make_unique<SdlSystemPreferencesSource>(std::move(platform_preferences));
        return true;
    }

    void SdlPlatformWindow::destroy() {
        system_preferences_.reset();
        // Stop IME/text input before the SDL window goes away.
        text_input_service_.reset();
        if (frame_target_ != nullptr) {
            if (renderer_ != nullptr) {
                (void)SDL_SetRenderTarget(renderer_, nullptr);
            }
            SDL_DestroyTexture(frame_target_);
            frame_target_ = nullptr;
        }
        frame_target_gpu_ = nullptr;
        frame_target_width_ = 0;
        frame_target_height_ = 0;
        frame_target_format_ = SDL_PIXELFORMAT_RGBA32;
        if (renderer_ != nullptr) {
            SDL_DestroyRenderer(renderer_);
            renderer_ = nullptr;
        }
        if (gpu_swapchain_claimed_ && gpu_device_ != nullptr && window_ != nullptr) {
            SDL_ReleaseWindowFromGPUDevice(gpu_device_, window_);
        }
        gpu_swapchain_claimed_ = false;
        if (gpu_device_ != nullptr) {
            SDL_DestroyGPUDevice(gpu_device_);
            gpu_device_ = nullptr;
        }
        if (window_ != nullptr) {
            SDL_DestroyWindow(window_);
            window_ = nullptr;
        }
        // 只关自己开的那次 SDL_Init：`destroy()` 可以重复调用，也不会误伤同进程其他 SDL 用户。
        if (sdl_initialized_) {
            SDL_Quit();
            sdl_initialized_ = false;
        }
        ready_ = false;
        should_close_ = false;
        pace_frames_ = false;
        target_fps_ = 0;
        last_present_ns_ = 0;
        vsync_enabled_ = true;
        steady_present_mode_ = SDL_GPU_PRESENTMODE_VSYNC;
        resize_present_mode_ = SDL_GPU_PRESENTMODE_VSYNC;
        active_present_mode_ = SDL_GPU_PRESENTMODE_VSYNC;
        last_resize_ns_ = 0;
        clear_color_ = {};
        modifiers_ = {};
        held_keys_.fill({});
        window_active_ = false;
        high_dpi_ = false;
        content_scale_ = 1.0F;
        logical_size_ = foundation::NanSize::zero();
    }

    auto SdlPlatformWindow::content_scale() const noexcept -> float {
        return content_scale_ > 0.0F ? content_scale_ : 1.0F;
    }

    void SdlPlatformWindow::refresh_content_scale() {
        float scale = 1.0F;
        if (high_dpi_ && window_ != nullptr) {
            // display_scale = 物理像素 / 原生窗口坐标；pixel_density = framebuffer 像素 /
            // 原生窗口坐标。二者之比才是 OS 内容缩放（Windows 175% -> 1.75，Retina -> 1.0）。
            const float display_scale = SDL_GetWindowDisplayScale(window_);
            const float pixel_density = SDL_GetWindowPixelDensity(window_);
            scale = app::content_scale_from_display(display_scale, pixel_density);
        }
        content_scale_ = scale;
        apply_render_scale();
        if (text_input_service_) {
            text_input_service_->set_window_scale(scale);
        }
    }

    void SdlPlatformWindow::apply_render_scale() {
        if (renderer_ == nullptr) {
            return;
        }
        // Scene/layout/text 坐标是逻辑单位；SDL 渲染器负责逻辑 -> 原生窗口 -> 物理像素。
        // display scale 正好是物理像素 / 逻辑单位（Windows 175% = 1.75，Retina = 2）。
        float scale = 1.0F;
        if (high_dpi_ && window_ != nullptr) {
            const float display_scale = SDL_GetWindowDisplayScale(window_);
            if (std::isfinite(display_scale) && display_scale > 0.0F) {
                scale = display_scale;
            }
        }
        if (!SDL_SetRenderScale(renderer_, scale, scale)) {
            SDL_Log(
                "SdlPlatformWindow: SDL_SetRenderScale(%.2f) failed: %s",
                scale,
                SDL_GetError()
            );
        }
    }

    void SdlPlatformWindow::apply_logical_window_size() {
        if (window_ == nullptr || !high_dpi_) {
            return;
        }
        const auto native = app::window_size_from_logical_size(logical_size_, content_scale_);
        const int width = static_cast<int>(std::lround(native.get_width()));
        const int height = static_cast<int>(std::lround(native.get_height()));
        int current_width = 0;
        int current_height = 0;
        if (!SDL_GetWindowSize(window_, &current_width, &current_height)) {
            return;
        }
        if (current_width == width && current_height == height) {
            return;
        }
        if (!SDL_SetWindowSize(window_, width, height)) {
            SDL_Log(
                "SdlPlatformWindow: SDL_SetWindowSize(%dx%d) failed: %s",
                width,
                height,
                SDL_GetError()
            );
        }
    }

    void SdlPlatformWindow::ensure_window_visible() {
        if (window_ == nullptr) {
            return;
        }
        // 最大化/全屏由 OS 管理，不主动改位置。
        const SDL_WindowFlags flags = SDL_GetWindowFlags(window_);
        if ((flags & (SDL_WINDOW_MAXIMIZED | SDL_WINDOW_FULLSCREEN)) != 0) {
            return;
        }
        const SDL_DisplayID display = SDL_GetDisplayForWindow(window_);
        if (display == 0) {
            return;
        }
        SDL_Rect usable {};
        if (!SDL_GetDisplayUsableBounds(display, &usable)) {
            return;
        }
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        if (!SDL_GetWindowPosition(window_, &x, &y) || !SDL_GetWindowSize(window_, &width, &height))
        {
            return;
        }
        // SDL_SetWindowPosition 的坐标是客户区左上角；把非客户区边框也算进可用区域，避免
        // 标题栏/右边框越界。窗口本身大于可用区域时贴住左上角，优先保证标题栏可见。
        int top_border = 0;
        int left_border = 0;
        int bottom_border = 0;
        int right_border = 0;
        (void)SDL_GetWindowBordersSize(
            window_,
            &top_border,
            &left_border,
            &bottom_border,
            &right_border
        );
        const int frame_width = left_border + right_border;
        const int frame_height = top_border + bottom_border;
        const int min_x = usable.x + left_border;
        const int min_y = usable.y + top_border;
        const int max_x = min_x + std::max(0, usable.w - width - frame_width);
        const int max_y = min_y + std::max(0, usable.h - height - frame_height);
        const int clamped_x = std::clamp(x, min_x, std::max(min_x, max_x));
        const int clamped_y = std::clamp(y, min_y, std::max(min_y, max_y));
        if (clamped_x == x && clamped_y == y) {
            return;
        }
        SDL_Log(
            "SdlPlatformWindow: clamp window position %d,%d -> %d,%d (usable %d,%d %dx%d)",
            x,
            y,
            clamped_x,
            clamped_y,
            usable.x,
            usable.y,
            usable.w,
            usable.h
        );
        if (!SDL_SetWindowPosition(window_, clamped_x, clamped_y)) {
            SDL_Log(
                "SdlPlatformWindow: SDL_SetWindowPosition(%d,%d) failed: %s",
                clamped_x,
                clamped_y,
                SDL_GetError()
            );
        }
    }

    void SdlPlatformWindow::handle_display_scale_changed() {
        const float previous_scale = content_scale();
        refresh_content_scale();
        if (content_scale() != previous_scale) {
            // 作者的窗口请求是逻辑尺寸：系统/跨屏 DPI 变化后把它恢复到同一逻辑尺寸。
            apply_logical_window_size();
            ensure_window_visible();
        }
        // 跨屏/系统缩放变化是混合 DPI 验收的核心证据：每次事件只打一行，不做逐帧日志。
        int native_width = 0;
        int native_height = 0;
        int pixel_width = 0;
        int pixel_height = 0;
        float render_scale_x = 0.0F;
        float render_scale_y = 0.0F;
        (void)SDL_GetWindowSize(window_, &native_width, &native_height);
        (void)SDL_GetWindowSizeInPixels(window_, &pixel_width, &pixel_height);
        if (renderer_ != nullptr) {
            (void)SDL_GetRenderScale(renderer_, &render_scale_x, &render_scale_y);
        }
        const auto logical = screen_size();
        SDL_Log(
            "SdlPlatformWindow: display scale %.2f -> %.2f; logical=%dx%d native=%dx%d "
            "pixels=%dx%d render_scale=%.2f",
            previous_scale,
            content_scale(),
            static_cast<int>(std::lround(logical.get_width())),
            static_cast<int>(std::lround(logical.get_height())),
            native_width,
            native_height,
            pixel_width,
            pixel_height,
            render_scale_x
        );
    }

    void SdlPlatformWindow::remember_logical_size_from_window() {
        int width = 0;
        int height = 0;
        if (window_ == nullptr || !SDL_GetWindowSize(window_, &width, &height) || width <= 0
            || height <= 0)
        {
            return;
        }
        logical_size_ = app::logical_size_from_window_size(
            foundation::NanSize(static_cast<float>(width), static_cast<float>(height)),
            content_scale()
        );
    }

    auto SdlPlatformWindow::is_ready() const -> bool {
        return ready_;
    }

    auto SdlPlatformWindow::should_close() const -> bool {
        // 已销毁（或从未创建成功）的窗口必须报「该关闭了」：宿主的主循环用这个判断是否继续
        // tick，而 tick 会读窗口尺寸。关窗后的 should_close() 语义
        // 必须一致 —— 否则 SDL 路径会在 close() 之后继续 tick，读到 0×0 尺寸并断言失败
        // （window_lifecycle 用例就是这么抓到的）。
        if (!ready_ || window_ == nullptr) {
            return true;
        }
        return should_close_;
    }

    auto SdlPlatformWindow::screen_size() const -> foundation::NanSize {
        int width = 0;
        int height = 0;
        if (window_ == nullptr || !SDL_GetWindowSize(window_, &width, &height) || width <= 0
            || height <= 0)
        {
            return foundation::NanSize::zero();
        }
        // 平台 seam 的 `screen_size` 是逻辑（作者/布局）尺寸；原生窗口坐标在 Windows 上
        // 是物理像素、在 Retina 上是点，所以统一按内容缩放折算。
        return app::logical_size_from_window_size(
            foundation::NanSize(static_cast<float>(width), static_cast<float>(height)),
            content_scale()
        );
    }

    auto SdlPlatformWindow::drawable_size() const -> foundation::NanSize {
        int width = 0;
        int height = 0;
        if (window_ == nullptr || !SDL_GetWindowSizeInPixels(window_, &width, &height)) {
            return foundation::NanSize::zero();
        }
        return foundation::NanSize(static_cast<float>(width), static_cast<float>(height));
    }

    auto SdlPlatformWindow::frame_time() const -> float {
        const std::uint64_t now = SDL_GetTicksNS();
        if (!has_frame_tick_) {
            // 第一帧没有参照点：给一个 60Hz 的合理值，避免宿主第一步就把动画跳到底。
            has_frame_tick_ = true;
            last_frame_ns_ = now;
            return 1.0F / 60.0F;
        }
        const std::uint64_t elapsed_ns = now - last_frame_ns_;
        last_frame_ns_ = now;
        constexpr double kNanosPerSecond = 1'000'000'000.0;
        return static_cast<float>(static_cast<double>(elapsed_ns) / kNanosPerSecond);
    }

    auto SdlPlatformWindow::display_scale() const -> float {
        // 物理像素 / 逻辑单位。1× 模式（high_dpi=false）固定为 1：窗口、输入与渲染都不缩。
        if (window_ == nullptr || !high_dpi_) {
            return 1.0F;
        }
        const float scale = SDL_GetWindowDisplayScale(window_);
        return std::isfinite(scale) && scale > 0.0F ? scale : 1.0F;
    }

    void SdlPlatformWindow::poll_input(FrameInput& out) {
        if (window_ == nullptr) {
            return;
        }

        SDL_Event event {};
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    should_close_ = true;
                    break;

                case SDL_EVENT_SYSTEM_THEME_CHANGED:
                    if (system_preferences_ != nullptr) {
                        system_preferences_->notify_theme_changed();
                    }
                    break;

                case SDL_EVENT_WINDOW_FOCUS_LOST:
                    if (event.window.windowID == SDL_GetWindowID(window_)) {
                        window_active_ = false;
                        modifiers_ = {};
                        held_keys_.fill({});
                    }
                    break;

                case SDL_EVENT_WINDOW_FOCUS_GAINED:
                    if (event.window.windowID == SDL_GetWindowID(window_)) {
                        window_active_ = true;
                        modifiers_ = translate_modifiers(SDL_GetModState());
                    }
                    break;

                case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
                    if (event.window.windowID == SDL_GetWindowID(window_)) {
                        handle_display_scale_changed();
                        last_resize_ns_ = SDL_GetTicksNS();
                    }
                    break;

                case SDL_EVENT_WINDOW_RESIZED:
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                    if (event.window.windowID == SDL_GetWindowID(window_)) {
                        remember_logical_size_from_window();
                        last_resize_ns_ = SDL_GetTicksNS();
                    }
                    break;

                case SDL_EVENT_MOUSE_MOTION: {
                    const float scale = content_scale();
                    pointer_ = foundation::NanPoint(event.motion.x / scale, event.motion.y / scale);
                    has_pointer_ = true;
                    out.events.emplace_back(
                        scene::MouseMoveEvent {
                            pointer_,
                            foundation::NanPoint(
                                event.motion.xrel / scale,
                                event.motion.yrel / scale
                            ),
                        }
                    );
                    break;
                }

                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                case SDL_EVENT_MOUSE_BUTTON_UP: {
                    const auto button = translate_button(event.button.button);
                    if (!button.has_value()) {
                        break;
                    }
                    const float scale = content_scale();
                    pointer_ = foundation::NanPoint(event.button.x / scale, event.button.y / scale);
                    has_pointer_ = true;
                    out.events.emplace_back(
                        scene::MouseButtonEvent {
                            *button,
                            event.type == SDL_EVENT_MOUSE_BUTTON_DOWN
                                ? scene::MouseButtonEvent::Action::press
                                : scene::MouseButtonEvent::Action::release,
                            pointer_,
                            modifiers_,
                        }
                    );
                    break;
                }

                case SDL_EVENT_MOUSE_WHEEL: {
                    const float scale = content_scale();
                    pointer_ = foundation::NanPoint(
                        event.wheel.mouse_x / scale,
                        event.wheel.mouse_y / scale
                    );
                    has_pointer_ = true;
                    // SDL wheel.y and MouseWheelEvent both use positive y for upward scroll.
                    out.events.emplace_back(
                        scene::MouseWheelEvent {
                            pointer_,
                            foundation::NanPoint(event.wheel.x, event.wheel.y),
                            modifiers_,
                        }
                    );
                    break;
                }

                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_KEY_UP: {
                    // SDL_PollEvent pumps the whole queue; use this event's modifier snapshot.
                    modifiers_ = translate_modifiers(event.key.mod);
                    const auto physical = translate_physical_key(event.key.scancode);
                    auto logical = translate_logical_key(event.key.key, event.key.mod);
                    const auto action = event.type == SDL_EVENT_KEY_UP
                        ? scene::KeyEvent::Action::release
                        : event.key.repeat ? scene::KeyEvent::Action::repeat
                                           : scene::KeyEvent::Action::press;
                    if (physical != foundation::PhysicalKey::unknown) {
                        auto& held = held_keys_[static_cast<std::size_t>(physical)];
                        if (held.down) {
                            logical = held.logical;
                        }
                        if (action == scene::KeyEvent::Action::press && !held.down) {
                            held = HeldKey {logical, true};
                        }
                        else if (action == scene::KeyEvent::Action::release) {
                            held = {};
                        }
                    }
                    if (logical == foundation::LogicalKey::unknown
                        && physical == foundation::PhysicalKey::unknown)
                    {
                        ++unknown_keys_;
                    }
                    out.events.emplace_back(
                        scene::KeyEvent {logical, action, modifiers_, physical}
                    );
                    break;
                }

                case SDL_EVENT_TEXT_EDITING: {
                    // SDL positions are UTF-8 character offsets into the editing text; keep
                    // them as the protocol's selection range and let the target clamp.
                    const auto* text = event.edit.text;
                    const auto start =
                        event.edit.start >= 0 ? static_cast<std::size_t>(event.edit.start) : 0U;
                    const auto end = event.edit.length >= 0
                        ? static_cast<std::size_t>(event.edit.start + event.edit.length)
                        : start;
                    out.events.emplace_back(
                        scene::TextEditingEvent {
                            .text = text != nullptr ? std::string(text) : std::string {},
                            .selection_start = start,
                            .selection_end = end,
                        }
                    );
                    break;
                }

                case SDL_EVENT_TEXT_INPUT: {
                    // SDL 给的已经是合成好的 UTF-8，框架的 TextInputEvent 正好要这个。
                    if (event.text.text != nullptr && event.text.text[0] != '\0') {
                        out.events.emplace_back(
                            scene::TextInputEvent {std::string(event.text.text)}
                        );
                    }
                    break;
                }

                default:
                    break;
            }
        }

        // 指针位置与修饰键每帧都要给（不只是变化时）。有 motion 事件就用最近一次位置，
        // 否则退回 `SDL_GetMouseState`（窗口刚建好、鼠标还没动过的那一帧）。
        float queried_x = 0.0F;
        float queried_y = 0.0F;
        static_cast<void>(SDL_GetMouseState(&queried_x, &queried_y));
        out.has_pointer = true;
        out.pointer = has_pointer_ ? pointer_ : foundation::NanPoint(queried_x, queried_y);
        // 帧级状态与事件快照不同：保留 SDL 当前状态，不能反向覆盖排队事件的快照。
        out.modifiers = translate_modifiers(SDL_GetModState());
        out.window_active = window_active_;
    }

    auto SdlPlatformWindow::ensure_gpu_frame_target() -> bool {
        if (!gpu_swapchain_claimed_ || renderer_ == nullptr || window_ == nullptr) {
            return true;
        }
        int pixel_width = 0;
        int pixel_height = 0;
        if (!SDL_GetWindowSizeInPixels(window_, &pixel_width, &pixel_height)) {
            return frame_target_ != nullptr;
        }
        if (pixel_width <= 0 || pixel_height <= 0) {
            if (frame_target_ != nullptr) {
                return true;
            }
            pixel_width = std::max(1, pixel_width);
            pixel_height = std::max(1, pixel_height);
        }
        if (frame_target_ != nullptr && frame_target_gpu_ != nullptr
            && frame_target_width_ == pixel_width && frame_target_height_ == pixel_height)
        {
            if (SDL_GetRenderTarget(renderer_) != frame_target_) {
                (void)SDL_SetRenderTarget(renderer_, frame_target_);
                apply_render_scale();
            }
            return true;
        }
        if (frame_target_width_ > 0 && frame_target_height_ > 0) {
            last_resize_ns_ = SDL_GetTicksNS();
        }
        SDL_Texture* next_target = SDL_CreateTexture(
            renderer_,
            frame_target_format_,
            SDL_TEXTUREACCESS_TARGET,
            pixel_width,
            pixel_height
        );
        if (next_target == nullptr) {
            SDL_Log(
                "SdlPlatformWindow: SDL_CreateTexture(target %dx%d) failed: %s",
                pixel_width,
                pixel_height,
                SDL_GetError()
            );
            return false;
        }
        (void)SDL_SetTextureScaleMode(next_target, SDL_SCALEMODE_NEAREST);
        (void)SDL_SetTextureBlendMode(next_target, SDL_BLENDMODE_NONE);
        auto* next_gpu = static_cast<SDL_GPUTexture*>(SDL_GetPointerProperty(
            SDL_GetTextureProperties(next_target),
            SDL_PROP_TEXTURE_GPU_TEXTURE_POINTER,
            nullptr
        ));
        // 先切换 render_target 再销毁旧纹理：SDL_DestroyTexture 在销毁正绑定目标时会把内部
        // render_target 置空；若绑定新目标失败再销毁 next_target，需把旧 frame_target_ 重新绑回。
        if (next_gpu == nullptr || !SDL_SetRenderTarget(renderer_, next_target)) {
            SDL_Log(
                "SdlPlatformWindow: failed to bind GPU frame target %dx%d: %s",
                pixel_width,
                pixel_height,
                SDL_GetError()
            );
            SDL_DestroyTexture(next_target);
            if (frame_target_ != nullptr) {
                (void)SDL_SetRenderTarget(renderer_, frame_target_);
                apply_render_scale();
            }
            return false;
        }
        if (frame_target_ != nullptr) {
            SDL_DestroyTexture(frame_target_);
        }
        frame_target_ = next_target;
        frame_target_gpu_ = next_gpu;
        frame_target_width_ = pixel_width;
        frame_target_height_ = pixel_height;
        apply_render_scale();
        return true;
    }

    auto SdlPlatformWindow::display_refresh_interval_ns() const noexcept -> std::uint64_t {
        constexpr std::uint64_t kDefaultRefreshNs = 1'000'000'000ULL / 60ULL;
        if (window_ == nullptr) {
            return kDefaultRefreshNs;
        }
        const SDL_DisplayID display = SDL_GetDisplayForWindow(window_);
        if (display == 0) {
            return kDefaultRefreshNs;
        }
        const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display);
        if (mode == nullptr) {
            mode = SDL_GetDesktopDisplayMode(display);
        }
        if (mode == nullptr || !std::isfinite(mode->refresh_rate) || mode->refresh_rate <= 1.0F) {
            return kDefaultRefreshNs;
        }
        const auto interval =
            std::llround(1'000'000'000.0 / static_cast<double>(mode->refresh_rate));
        return interval > 0 ? static_cast<std::uint64_t>(interval) : kDefaultRefreshNs;
    }

    void SdlPlatformWindow::prepare_frame(const foundation::NanColor& clear_color) {
        clear_color_ = clear_color;
        (void)ensure_gpu_frame_target();
    }

    void SdlPlatformWindow::present() {
        if (renderer_ == nullptr) {
            return;
        }
        if (!gpu_swapchain_claimed_) {
            static_cast<void>(SDL_RenderPresent(renderer_));
            if (!pace_frames_) {
                return;
            }
            const std::uint64_t frame_ns = 1'000'000'000ULL / target_fps_;
            const std::uint64_t now = SDL_GetTicksNS();
            if (last_present_ns_ != 0 && now - last_present_ns_ < frame_ns) {
                SDL_DelayNS(frame_ns - (now - last_present_ns_));
            }
            last_present_ns_ = SDL_GetTicksNS();
            return;
        }

        // 连续缩放窗口期内切到 MAILBOX（或 Wayland IMMEDIATE），使新尺寸帧直接替换未上屏的旧帧，
        // 避免 Vulkan FIFO / Wayland wp_fifo_v1 积压数十帧在停手后缓慢滑行；静止后恢复 VSYNC。
        constexpr std::uint64_t kResizeSettleNs = 120'000'000ULL;
        const std::uint64_t before_present_ns = SDL_GetTicksNS();
        const bool active_resize = vsync_enabled_ && resize_present_mode_ != steady_present_mode_
            && last_resize_ns_ != 0 && (before_present_ns - last_resize_ns_ <= kResizeSettleNs);
        const SDL_GPUPresentMode desired_mode =
            active_resize ? resize_present_mode_ : steady_present_mode_;
        if (desired_mode != active_present_mode_) {
            if (SDL_SetGPUSwapchainParameters(
                    gpu_device_,
                    window_,
                    SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                    desired_mode
                ))
            {
                active_present_mode_ = desired_mode;
            }
        }

        // 先把无窗口 GPU renderer 队列提交到 frame_target_gpu_，同时推进 render_command_generation。
        static_cast<void>(SDL_RenderPresent(renderer_));

        bool presented_swapchain = false;
        if (frame_target_gpu_ != nullptr && frame_target_width_ > 0 && frame_target_height_ > 0) {
            if (SDL_GPUCommandBuffer* present_cmd = SDL_AcquireGPUCommandBuffer(gpu_device_);
                present_cmd != nullptr)
            {
                SDL_GPUTexture* swapchain = nullptr;
                Uint32 swap_w = 0;
                Uint32 swap_h = 0;
                const bool acquired = SDL_WaitAndAcquireGPUSwapchainTexture(
                    present_cmd,
                    window_,
                    &swapchain,
                    &swap_w,
                    &swap_h
                );
                if (!acquired) {
                    SDL_Log(
                        "SdlPlatformWindow: SDL_WaitAndAcquireGPUSwapchainTexture failed: %s",
                        SDL_GetError()
                    );
                }
                if (acquired && swapchain != nullptr && swap_w > 0 && swap_h > 0) {
                    presented_swapchain = true;
                    const auto plan = make_present_transfer_plan(
                        static_cast<std::uint32_t>(frame_target_width_),
                        static_cast<std::uint32_t>(frame_target_height_),
                        swap_w,
                        swap_h
                    );
                    if (plan.clear_uncovered_margin) {
                        const auto rgb = clear_color_.to<foundation::NanHexRgb>();
                        SDL_GPUColorTargetInfo clear_target {};
                        clear_target.texture = swapchain;
                        clear_target.clear_color = SDL_FColor {
                            .r = static_cast<float>(rgb.red) / 255.0F,
                            .g = static_cast<float>(rgb.green) / 255.0F,
                            .b = static_cast<float>(rgb.blue) / 255.0F,
                            .a = static_cast<float>(rgb.alpha) / 255.0F,
                        };
                        clear_target.load_op = SDL_GPU_LOADOP_CLEAR;
                        clear_target.store_op = SDL_GPU_STOREOP_STORE;
                        if (auto* pass =
                                SDL_BeginGPURenderPass(present_cmd, &clear_target, 1, nullptr);
                            pass != nullptr)
                        {
                            SDL_EndGPURenderPass(pass);
                        }
                    }
                    if (plan.copy_width > 0 && plan.copy_height > 0) {
                        SDL_GPUBlitInfo blit {};
                        blit.source.texture = frame_target_gpu_;
                        blit.source.x = 0;
                        blit.source.y = 0;
                        blit.source.w = plan.copy_width;
                        blit.source.h = plan.copy_height;
                        blit.destination.texture = swapchain;
                        blit.destination.x = 0;
                        blit.destination.y = 0;
                        blit.destination.w = plan.copy_width;
                        blit.destination.h = plan.copy_height;
                        blit.load_op = plan.clear_uncovered_margin ? SDL_GPU_LOADOP_LOAD
                                                                   : SDL_GPU_LOADOP_DONT_CARE;
                        blit.filter = SDL_GPU_FILTER_NEAREST;
                        SDL_BlitGPUTexture(present_cmd, &blit);
                    }
                }
                static_cast<void>(SDL_SubmitGPUCommandBuffer(present_cmd));
            }
        }

        // vsync 打开时，若处于 MAILBOX/IMMEDIATE 缩放期，或窗口隐藏/最小化导致未获得可上屏的
        // swapchain 图像（SDL_WaitAndAcquireGPUSwapchainTexture 在 Wayland/X11 上 0ms 返回空纹理），
        // 按显示器刷新周期做上限节流，避免离屏渲染空转占满 CPU/GPU。
        const bool pace_display_refresh = vsync_enabled_
            && (active_present_mode_ != SDL_GPU_PRESENTMODE_VSYNC || !presented_swapchain);
        if (!pace_frames_ && !pace_display_refresh) {
            last_present_ns_ = SDL_GetTicksNS();
            return;
        }
        const std::uint64_t frame_ns =
            pace_frames_ ? (1'000'000'000ULL / target_fps_) : display_refresh_interval_ns();
        const std::uint64_t now = SDL_GetTicksNS();
        if (last_present_ns_ != 0 && now - last_present_ns_ < frame_ns) {
            SDL_DelayNS(frame_ns - (now - last_present_ns_));
        }
        last_present_ns_ = SDL_GetTicksNS();
    }

    void SdlPlatformWindow::set_clipboard_text(const std::string_view text) {
        // SDL 要 NUL 结尾，`string_view` 不保证，先落一份 std::string。
        static_cast<void>(SDL_SetClipboardText(std::string(text).c_str()));
    }

    auto SdlPlatformWindow::clipboard_text() const -> std::string {
        char* raw = SDL_GetClipboardText();
        if (raw == nullptr) {
            return {};
        }
        std::string text(raw);
        SDL_free(raw); // SDL 用 SDL_malloc 分配，必须配 SDL_free。
        return text;
    }

    auto SdlPlatformWindow::make_render_device() -> std::unique_ptr<render::IRenderDevice> {
        if (renderer_ == nullptr) {
            return nullptr;
        }
        return std::make_unique<render::SdlRenderDevice>(*renderer_);
    }

    auto SdlPlatformWindow::text_input_service() -> scene::ITextInputService* {
        return text_input_service_.get();
    }

    auto SdlPlatformWindow::system_preferences_source() -> ISystemPreferencesSource* {
        return system_preferences_.get();
    }

    auto SdlPlatformWindow::name() const -> std::string_view {
        return "sdl3";
    }

    auto SdlPlatformWindow::unknown_key_count() const -> std::uint64_t {
        return unknown_keys_;
    }

} // namespace nandina::app::detail

#endif // NANDINA_HAS_SDL_BACKEND
