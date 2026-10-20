//
// app/detail/sdl_platform_window — SDL3 的 `IPlatformWindow` 实现。设计说明见同名头文件。
//

#include "sdl_platform_window.hpp"

#if defined(NANDINA_HAS_SDL_BACKEND)

    #include "../../render/backends/sdl_device.hpp"

    #include <SDL3/SDL_gpu.h>
    #include <SDL3/SDL_render.h>

    #include <optional>
    #include <string>

namespace nandina::app::detail
{
    namespace
    {
        // ─── GLFW 键码空间（`widget/key_codes.hpp`）────────────────────────────
        // 框架的键码就是 GLFW 的：raylib 直接透传。SDL 的键码不同，必须翻译。
        // 下面这些值对应 GLFW_KEY_*，与 key_codes.hpp 一一对应。
        constexpr int kGlfwEscape = 256;
        constexpr int kGlfwEnter = 257;
        constexpr int kGlfwTab = 258;
        constexpr int kGlfwBackspace = 259;
        constexpr int kGlfwInsert = 260;
        constexpr int kGlfwDelete = 261;
        constexpr int kGlfwRight = 262;
        constexpr int kGlfwLeft = 263;
        constexpr int kGlfwDown = 264;
        constexpr int kGlfwUp = 265;
        constexpr int kGlfwPageUp = 266;
        constexpr int kGlfwPageDown = 267;
        constexpr int kGlfwHome = 268;
        constexpr int kGlfwEnd = 269;
        constexpr int kGlfwF1 = 290;
        constexpr int kGlfwShift = 340;
        constexpr int kGlfwControl = 341;
        constexpr int kGlfwAlt = 342;
        constexpr int kGlfwSuper = 343;
        constexpr int kGlfwMenu = 348; // GLFW_KEY_MENU，key_codes.hpp 的 `menu`。

        /// 把 `SDL_Keycode` 翻译成 GLFW 键码；未覆盖返回 nullopt（调用方计数并丢弃）。
        ///
        /// ASCII 部分的分歧点只有一个：SDL 的字母是小写（`SDLK_A == 'a' == 97`），而 GLFW
        /// 的字母是大写（`GLFW_KEY_A == 65`）。数字/标点/空格两边数值相同，直接透传；`[\]\``
        /// 这些 GLFW 也定义了，但 `{|}~` 是它们的 Shift 变体、GLFW 没有对应键码，所以
        /// 123..126 走「未映射」分支。
        [[nodiscard]] auto translate_key(const SDL_Keycode key) -> std::optional<int> {
            // SDL 的小写字母 -> GLFW 的大写字母键码。
            if (key >= SDLK_A && key <= SDLK_Z) {
                return static_cast<int>(key) - ('a' - 'A');
            }
            // 空格、数字与标点：两个键码空间数值相同（GLFW_KEY_SPACE == 32 ... GLFW_KEY_GRAVE_ACCENT == 96）。
            if (key >= SDLK_SPACE && key <= SDLK_GRAVE) {
                return static_cast<int>(key);
            }

            switch (key) {
                case SDLK_ESCAPE:
                    return kGlfwEscape;
                case SDLK_RETURN:
                    return kGlfwEnter;
                case SDLK_TAB:
                    return kGlfwTab;
                case SDLK_BACKSPACE:
                    return kGlfwBackspace;
                case SDLK_INSERT:
                    return kGlfwInsert;
                case SDLK_DELETE:
                    return kGlfwDelete;
                case SDLK_RIGHT:
                    return kGlfwRight;
                case SDLK_LEFT:
                    return kGlfwLeft;
                case SDLK_DOWN:
                    return kGlfwDown;
                case SDLK_UP:
                    return kGlfwUp;
                case SDLK_PAGEUP:
                    return kGlfwPageUp;
                case SDLK_PAGEDOWN:
                    return kGlfwPageDown;
                case SDLK_HOME:
                    return kGlfwHome;
                case SDLK_END:
                    return kGlfwEnd;
                // SDLK_F1..SDLK_F12 连续，偏移量等于 GLFW 的偏移量（GLFW_KEY_F1 == 290）。
                case SDLK_F1:
                case SDLK_F2:
                case SDLK_F3:
                case SDLK_F4:
                case SDLK_F5:
                case SDLK_F6:
                case SDLK_F7:
                case SDLK_F8:
                case SDLK_F9:
                case SDLK_F10:
                case SDLK_F11:
                case SDLK_F12:
                    return kGlfwF1 + static_cast<int>(key - SDLK_F1);
                // 四个修饰键：GLFW 不分左右，SDL 分，都归到同一个键码。
                case SDLK_LSHIFT:
                case SDLK_RSHIFT:
                    return kGlfwShift;
                case SDLK_LCTRL:
                case SDLK_RCTRL:
                    return kGlfwControl;
                case SDLK_LALT:
                case SDLK_RALT:
                    return kGlfwAlt;
                case SDLK_LGUI:
                case SDLK_RGUI:
                    return kGlfwSuper;
                case SDLK_APPLICATION:
                    return kGlfwMenu;
                default:
                    return std::nullopt;
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

        /// 每帧读一次当前修饰键状态（与 `NanWindow::current_modifiers()` 的语义一致：状态，
        /// 不是变化）。
        [[nodiscard]] auto current_modifiers() -> scene::KeyModifiers {
            const SDL_Keymod mods = SDL_GetModState();
            return scene::KeyModifiers {
                .shift = (mods & SDL_KMOD_SHIFT) != 0,
                .ctrl = (mods & SDL_KMOD_CTRL) != 0,
                .alt = (mods & SDL_KMOD_ALT) != 0,
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

        // 渲染器：先走 GPU 路径（与 probe/cross_platform/desktop/page_probe.cpp 一致），
        // 失败再退回 SDL 的默认 2D 渲染器。GPU 设备的唯一用途就是喂 GPU 渲染器，所以
        // GPU 渲染器建不起来时立刻释放它，不在窗口生命周期里留一个悬空的设备。
        gpu_device_ = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
        if (gpu_device_ != nullptr) {
            renderer_ = SDL_CreateGPURenderer(gpu_device_, window_);
        }
        if (renderer_ == nullptr) {
            if (gpu_device_ != nullptr) {
                SDL_DestroyGPUDevice(gpu_device_);
                gpu_device_ = nullptr;
            }
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

        // vsync / target_fps 只能尽力而为：vsync 走 SDL 的通用开关（驱动可能不支持，所以查
        // 返回值），target_fps 由 `present()` 在 vsync 关闭时自己数拍子 —— SDL 没有帧率上限 API。
        const int vsync = config.vsync ? 1 : 0;
        if (!SDL_SetRenderVSync(renderer_, vsync)) {
            SDL_Log("SdlPlatformWindow: SDL_SetRenderVSync(%d) failed: %s", vsync, SDL_GetError());
        }
        target_fps_ = config.target_fps > 0 ? static_cast<std::uint64_t>(config.target_fps) : 0;
        pace_frames_ = !config.vsync && target_fps_ > 0;
        last_present_ns_ = 0;
        if (pace_frames_) {
            SDL_Log(
                "SdlPlatformWindow: vsync off, pacing present() to target_fps=%d",
                config.target_fps
            );
        }

        should_close_ = false;
        has_pointer_ = false;
        pointer_ = foundation::NanPoint::zero();
        has_frame_tick_ = false;
        last_frame_ns_ = 0;
        ready_ = true;

        const auto* renderer_name = SDL_GetRendererName(renderer_);
        SDL_Log(
            "SdlPlatformWindow: ready %dx%d renderer=%s",
            config.width,
            config.height,
            renderer_name != nullptr ? renderer_name : "unknown"
        );
        return true;
    }

    void SdlPlatformWindow::destroy() {
        if (renderer_ != nullptr) {
            SDL_DestroyRenderer(renderer_);
            renderer_ = nullptr;
        }
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
    }

    auto SdlPlatformWindow::is_ready() const -> bool {
        return ready_;
    }

    auto SdlPlatformWindow::should_close() const -> bool {
        // 已销毁（或从未创建成功）的窗口必须报「该关闭了」：宿主的主循环用这个判断是否继续
        // tick，而 tick 会读窗口尺寸。raylib 平台的 WindowShouldClose() 在关窗后为真，语义
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
        if (window_ == nullptr || !SDL_GetWindowSize(window_, &width, &height)) {
            return foundation::NanSize::zero();
        }
        return foundation::NanSize(static_cast<float>(width), static_cast<float>(height));
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
        if (window_ == nullptr) {
            return 1.0F;
        }
        const float scale = SDL_GetWindowDisplayScale(window_);
        return scale > 0.0F ? scale : 1.0F;
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

                case SDL_EVENT_MOUSE_MOTION: {
                    pointer_ = foundation::NanPoint(event.motion.x, event.motion.y);
                    has_pointer_ = true;
                    out.mouse_moves.emplace_back(
                        pointer_,
                        foundation::NanPoint(event.motion.xrel, event.motion.yrel)
                    );
                    break;
                }

                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                case SDL_EVENT_MOUSE_BUTTON_UP: {
                    const auto button = translate_button(event.button.button);
                    if (!button.has_value()) {
                        break;
                    }
                    pointer_ = foundation::NanPoint(event.button.x, event.button.y);
                    has_pointer_ = true;
                    out.mouse_buttons.emplace_back(
                        *button,
                        event.type == SDL_EVENT_MOUSE_BUTTON_DOWN
                            ? scene::MouseButtonEvent::Action::press
                            : scene::MouseButtonEvent::Action::release,
                        pointer_,
                        current_modifiers()
                    );
                    break;
                }

                case SDL_EVENT_MOUSE_WHEEL: {
                    pointer_ = foundation::NanPoint(event.wheel.mouse_x, event.wheel.mouse_y);
                    has_pointer_ = true;
                    // 符号约定：SDL3 的 `wheel.y` 正值表示「远离用户」（向上滚），与 GLFW 的
                    // yoffset 以及 `scene::MouseWheelEvent` 的「y > 0 向上」一致 —— 不翻转。
                    out.mouse_wheels.emplace_back(
                        pointer_,
                        foundation::NanPoint(event.wheel.x, event.wheel.y),
                        current_modifiers()
                    );
                    break;
                }

                case SDL_EVENT_KEY_DOWN: {
                    // 只发按下、跳过自动重复：对应 raylib `GetKeyPressed()` 队列的语义。
                    if (event.key.repeat) {
                        break;
                    }
                    const auto key = translate_key(event.key.key);
                    if (!key.has_value()) {
                        // 未映射的键绝不写进事件流；计数让缺口可观测。
                        ++skipped_keys_;
                        break;
                    }
                    out.keys
                        .emplace_back(*key, scene::KeyEvent::Action::press, current_modifiers());
                    break;
                }

                case SDL_EVENT_TEXT_INPUT: {
                    // SDL 给的已经是合成好的 UTF-8，框架的 TextInputEvent 正好要这个。
                    if (event.text.text != nullptr && event.text.text[0] != '\0') {
                        out.text_inputs.emplace_back(std::string(event.text.text));
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
        out.modifiers = current_modifiers();
    }

    void SdlPlatformWindow::present() {
        if (renderer_ != nullptr) {
            static_cast<void>(SDL_RenderPresent(renderer_));
        }
        if (!pace_frames_ || renderer_ == nullptr) {
            return;
        }
        // best-effort 帧率上限：只在 vsync 关闭时生效，且最多补足到下一帧的截止时间。
        const std::uint64_t frame_ns = 1'000'000'000ULL / target_fps_;
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

    auto SdlPlatformWindow::name() const -> std::string_view {
        return "sdl3";
    }

    auto SdlPlatformWindow::skipped_key_count() const -> std::uint64_t {
        return skipped_keys_;
    }

} // namespace nandina::app::detail

#endif // NANDINA_HAS_SDL_BACKEND
