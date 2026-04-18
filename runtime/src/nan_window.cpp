module;

// ============================================================
// 全局模块片段：SDL3 + ThorVG + 标准库
// 所有平台相关类型隔离于此，不通过模块接口泄漏给消费方。
// ============================================================
#include <SDL3/SDL.h>
#include <thorvg-1/thorvg.h>

#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

module nandina.runtime.nan_window;

import nandina.log;

namespace nandina::runtime { 

    namespace {
        [[nodiscard]] auto to_pointer_button(Uint8 button) noexcept -> PointerButton {
            switch (button) {
                case SDL_BUTTON_LEFT:
                    return PointerButton::Left;
                case SDL_BUTTON_MIDDLE:
                    return PointerButton::Middle;
                case SDL_BUTTON_RIGHT:
                    return PointerButton::Right;
                case SDL_BUTTON_X1:
                    return PointerButton::X1;
                case SDL_BUTTON_X2:
                    return PointerButton::X2;
                default:
                    return PointerButton::Unknown;
            }
        }

        struct RuntimeBootstrap {
            auto acquire() -> void {
                std::scoped_lock lock{mutex};
                if (ref_count == 0) {
                    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
                        throw std::runtime_error(std::format("SDL_Init failed: {}", SDL_GetError()));
                    }

                    if (tvg::Initializer::init(0u) != tvg::Result::Success) {
                        SDL_Quit();
                        throw std::runtime_error("ThorVG Initializer::init failed");
                    }
                }
                ++ref_count;
            }

            auto release() noexcept -> void {
                std::scoped_lock lock{mutex};
                if (ref_count <= 0) {
                    ref_count = 0;
                    return;
                }

                --ref_count;
                if (ref_count == 0) {
                    tvg::Initializer::term();
                    SDL_Quit();
                }
            }

            std::mutex mutex;
            int ref_count{0};
        };

        auto runtime_bootstrap() -> RuntimeBootstrap & {
            static RuntimeBootstrap bootstrap;
            return bootstrap;
        }

    } // namespace

    // ============================================================
    // PIMPL 内部实现
    // ============================================================
    struct NanWindow::Impl {
        // ── SDL RAII 删除器（隐藏于实现单元）────────────────────
        struct WindowDeleter {
            auto operator()(SDL_Window *w) const noexcept -> void {
                if (w)
                    SDL_DestroyWindow(w);
            }
        };

        struct RendererDeleter {
            auto operator()(SDL_Renderer *r) const noexcept -> void {
                if (r)
                    SDL_DestroyRenderer(r);
            }
        };

        struct TextureDeleter {
            auto operator()(SDL_Texture *t) const noexcept -> void {
                if (t)
                    SDL_DestroyTexture(t);
            }
        };

        // ── SDL 资源 ─────────────────────────────────────────────
        std::unique_ptr<SDL_Window, WindowDeleter> window;
        std::unique_ptr<SDL_Renderer, RendererDeleter> renderer;
        std::unique_ptr<SDL_Texture, TextureDeleter> texture;

        // ── ThorVG 软件渲染管线 ──────────────────────────────────
        // pixel_buffer: ThorVG 的 CPU 端光栅化目标（ARGB8888）
        // canvas:       绑定到 pixel_buffer 的 SwCanvas
        std::vector<std::uint32_t> pixel_buffer;
        std::unique_ptr<tvg::SwCanvas> canvas;

        // ── 属性缓存 ─────────────────────────────────────────────
        std::string title;
        int width{0};
        int height{0};
        bool runtime_acquired{false};
        float dpi_scale{1.0f};
        bool should_close{false};

        // ── 辅助：重建 SDL 纹理 + ThorVG 画布（供 resize 复用）─
        auto rebuild_surface(int new_w, int new_h) -> void {
            if (new_w <= 0 || new_h <= 0) {
                return;
            }

            // 重建 SDL 流式纹理
            auto *raw = SDL_CreateTexture(
                    renderer.get(), SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, new_w, new_h);
            if (!raw) {
                throw std::runtime_error(std::format("SDL_CreateTexture failed: {}", SDL_GetError()));
            }
            texture.reset(raw);

            // 重建像素缓冲区
            pixel_buffer.assign(static_cast<std::size_t>(new_w * new_h), 0u);

            // 重绑 ThorVG 画布到新缓冲区
            const auto result = canvas->target(pixel_buffer.data(),
                                               static_cast<std::uint32_t>(new_w), // stride（以像素计）
                                               static_cast<std::uint32_t>(new_w),
                                               static_cast<std::uint32_t>(new_h),
                                               tvg::ColorSpace::ARGB8888);

            if (result != tvg::Result::Success) {
                throw std::runtime_error("ThorVG SwCanvas::target rebind failed");
            }

            width = new_w;
            height = new_h;
        }
    };

    // ============================================================
    // NanWindow 构造（私有，仅由 Builder::build() 调用）
    // ============================================================
    NanWindow::NanWindow(const Config &config) :
        NanWindow(config.title, config.width, config.height, config.resizable, config.high_dpi) {}

    NanWindow::NanWindow(std::string_view title, int width, int height, bool resizable, bool high_dpi) :
        m_impl(std::make_unique<Impl>()) {
        auto log = nandina::log::get("runtime.nan_window");
        log.debug("Creating window \"{}\" {}x{}", title, width, height);

        if (width <= 0 || height <= 0) {
            throw std::runtime_error("NanWindow requires positive width and height");
        }

        runtime_bootstrap().acquire();
        m_impl->runtime_acquired = true;

        // ── SDL 窗口 + 渲染器 ────────────────────────────────────
        SDL_WindowFlags flags = SDL_WINDOW_HIDDEN;
        if (resizable)
            flags |= SDL_WINDOW_RESIZABLE;
        if (high_dpi)
            flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;

        m_impl->title = std::string(title);
        m_impl->width = width;
        m_impl->height = height;

        SDL_Window *raw_window = nullptr;
        SDL_Renderer *raw_renderer = nullptr;

        if (!SDL_CreateWindowAndRenderer(m_impl->title.c_str(), width, height, flags, &raw_window, &raw_renderer)) {
            runtime_bootstrap().release();
            m_impl->runtime_acquired = false;
            throw std::runtime_error(std::format("SDL_CreateWindowAndRenderer failed: {}", SDL_GetError()));
        }
        m_impl->window.reset(raw_window);
        m_impl->renderer.reset(raw_renderer);

        m_impl->dpi_scale = SDL_GetWindowDisplayScale(raw_window);

        // ── ThorVG SwCanvas 初始化 ───────────────────────────────
        // 先构造画布对象，再通过 rebuild_surface 统一创建纹理和绑定缓冲区
        m_impl->canvas.reset(tvg::SwCanvas::gen());
        if (!m_impl->canvas) {
            runtime_bootstrap().release();
            m_impl->runtime_acquired = false;
            throw std::runtime_error("tvg::SwCanvas::gen() returned nullptr");
        }

        int pixel_w = 0;
        int pixel_h = 0;
        SDL_GetWindowSizeInPixels(raw_window, &pixel_w, &pixel_h);
        m_impl->rebuild_surface(pixel_w, pixel_h);

        // ── 显示窗口（创建完成后再 show，避免白屏闪烁）──────────
        SDL_ShowWindow(raw_window);

        log.info("Window ready: \"{}\" {}x{} (DPI={:.2f})", title, width, height, m_impl->dpi_scale);
    }

    // ============================================================
    // 析构
    // ============================================================
    NanWindow::~NanWindow() {
        if (!m_impl)
            return;
        auto log = nandina::log::get("runtime.nan_window");
        // canvas 先析构（解除对 pixel_buffer 的引用），再释放 SDL 资源
        m_impl->canvas.reset();
        if (m_impl->runtime_acquired) {
            runtime_bootstrap().release();
            m_impl->runtime_acquired = false;
        }
        log.info("Window destroyed: \"{}\"", m_impl->title);
        // unique_ptr 析构按声明逆序：texture → renderer → window
    }

    // ============================================================
    // 移动语义
    // ============================================================
    NanWindow::NanWindow(NanWindow &&) noexcept = default;
    NanWindow &NanWindow::operator=(NanWindow &&) noexcept = default;

    // ============================================================
    // 属性访问
    // ============================================================
    auto NanWindow::title() const noexcept -> std::string_view {
        return m_impl ? m_impl->title : "";
    }

    auto NanWindow::width() const noexcept -> int {
        return m_impl ? m_impl->width : 0;
    }

    auto NanWindow::height() const noexcept -> int {
        return m_impl ? m_impl->height : 0;
    }

    auto NanWindow::dpi_scale() const noexcept -> float {
        return m_impl ? m_impl->dpi_scale : 1.0f;
    }

    // ============================================================
    // 生命周期状态
    // ============================================================
    auto NanWindow::should_close() const noexcept -> bool {
        return !m_impl || m_impl->should_close;
    }

    auto NanWindow::request_close() noexcept -> void {
        if (m_impl)
            m_impl->should_close = true;
    }

    // ============================================================
    // 事件处理（Issue 010 基础通路）
    // ============================================================
    auto NanWindow::poll_events() -> bool {
        if (!m_impl)
            return true;

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {

                case SDL_EVENT_QUIT:
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    m_impl->should_close = true;
                    on_close_requested();
                    break;

                case SDL_EVENT_WINDOW_RESIZED: {
                    auto log = nandina::log::get("runtime.nan_window");
                    const int logical_w = ev.window.data1;
                    const int logical_h = ev.window.data2;
                    int pixel_w = 0;
                    int pixel_h = 0;
                    SDL_GetWindowSizeInPixels(m_impl->window.get(), &pixel_w, &pixel_h);
                    log.debug("Window resized: logical={}x{}, pixel={}x{}", logical_w, logical_h, pixel_w, pixel_h);

                    // 纹理 + ThorVG 画布按新尺寸重建
                    try {
                        m_impl->rebuild_surface(pixel_w, pixel_h);
                    }
                    catch (const std::exception &e) {
                        log.error("rebuild_surface failed on resize: {}", e.what());
                    }

                    m_impl->dpi_scale = SDL_GetWindowDisplayScale(m_impl->window.get());
                    on_resize(logical_w, logical_h);
                    break;
                }

                case SDL_EVENT_MOUSE_MOTION: {
                    on_pointer_move(PointerMoveEvent{
                        .x = static_cast<double>(ev.motion.x),
                        .y = static_cast<double>(ev.motion.y),
                        .delta_x = static_cast<double>(ev.motion.xrel),
                        .delta_y = static_cast<double>(ev.motion.yrel),
                    });
                    break;
                }

                case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                    on_pointer_down(PointerButtonEvent{
                        .button = to_pointer_button(ev.button.button),
                        .x = static_cast<double>(ev.button.x),
                        .y = static_cast<double>(ev.button.y),
                        .is_repeat = false,
                    });
                    break;
                }

                case SDL_EVENT_MOUSE_BUTTON_UP: {
                    on_pointer_up(PointerButtonEvent{
                        .button = to_pointer_button(ev.button.button),
                        .x = static_cast<double>(ev.button.x),
                        .y = static_cast<double>(ev.button.y),
                        .is_repeat = false,
                    });
                    break;
                }

                case SDL_EVENT_MOUSE_WHEEL: {
                    on_pointer_wheel(PointerWheelEvent{
                        .x = static_cast<double>(ev.wheel.x),
                        .y = static_cast<double>(ev.wheel.y),
                    });
                    break;
                }

                case SDL_EVENT_KEY_DOWN: {
                    on_key_down(KeyEvent{
                        .key_code = static_cast<std::int32_t>(ev.key.key),
                        .is_repeat = ev.key.repeat,
                    });
                    break;
                }

                case SDL_EVENT_KEY_UP: {
                    on_key_up(KeyEvent{
                        .key_code = static_cast<std::int32_t>(ev.key.key),
                        .is_repeat = false,
                    });
                    break;
                }

                case SDL_EVENT_TEXT_INPUT:
                    on_text_input(ev.text.text);
                    break;

                default:
                    break;
            }
        }

        return m_impl->should_close;
    }

    // ============================================================
    // 帧渲染（Issue 017 最小渲染闭环）
    // ============================================================
    auto NanWindow::present_frame() -> void {
        if (!m_impl)
            return;
        if (!m_impl->texture || m_impl->width <= 0 || m_impl->height <= 0)
            return;

        auto &canvas = *m_impl->canvas;

        // 1. 清空上一帧所有 ThorVG 绘制指令
        canvas.remove(nullptr);

        // 2. 用户回调：向 canvas 添加本帧 ThorVG 图元
        on_draw(canvas);

        // 3. ThorVG 光栅化到 pixel_buffer（clear=true 每帧刷新背景）
        if (canvas.draw(true) != tvg::Result::Success) {
            return;
        }
        if (canvas.sync() != tvg::Result::Success) {
            return;
        }

        // 4. pixel_buffer → SDL 流式纹理（CPU → GPU 上传）
        if (!SDL_UpdateTexture(m_impl->texture.get(),
                               nullptr,
                               m_impl->pixel_buffer.data(),
                               m_impl->width * static_cast<int>(sizeof(std::uint32_t)))) {
            return;
        }

        // 5. SDL 渲染器：清空 → 贴纹理 → 呈现
        if (!SDL_RenderClear(m_impl->renderer.get())) {
            return;
        }
        if (!SDL_RenderTexture(m_impl->renderer.get(), m_impl->texture.get(), nullptr, nullptr)) {
            return;
        }
        SDL_RenderPresent(m_impl->renderer.get());
    }

    // ============================================================
    // 主循环阻塞接口
    // ============================================================
    auto NanWindow::run() -> void {
        auto last_counter = SDL_GetPerformanceCounter();
        bool ready_called = false;

        while (!poll_events()) {
            if (!ready_called) {
                on_ready();
                ready_called = true;
            }

            const auto current_counter = SDL_GetPerformanceCounter();
            const auto freq = static_cast<double>(SDL_GetPerformanceFrequency());
            const double delta_seconds = freq > 0.0 ? static_cast<double>(current_counter - last_counter) / freq : 0.0;
            last_counter = current_counter;

            on_update(delta_seconds);
            present_frame();
        }
    }

    auto NanWindow::on_ready() -> void {}

    auto NanWindow::on_update(double) -> void {}

    auto NanWindow::on_draw(tvg::SwCanvas &) -> void {}

    auto NanWindow::on_resize(int, int) -> void {}

    auto NanWindow::on_close_requested() -> void {}

    auto NanWindow::on_pointer_move(const PointerMoveEvent &) -> void {}

    auto NanWindow::on_pointer_down(const PointerButtonEvent &) -> void {}

    auto NanWindow::on_pointer_up(const PointerButtonEvent &) -> void {}

    auto NanWindow::on_pointer_wheel(const PointerWheelEvent &) -> void {}

    auto NanWindow::on_key_down(const KeyEvent &) -> void {}

    auto NanWindow::on_key_up(const KeyEvent &) -> void {}

    auto NanWindow::on_text_input(std::string_view) -> void {}

    // ============================================================
    // Builder 实现
    // ============================================================
    auto NanWindow::Builder::set_title(std::string_view title) noexcept -> Builder & {
        m_title = std::string(title);
        return *this;
    }

    auto NanWindow::Builder::set_width(int width) noexcept -> Builder & {
        m_width = width;
        return *this;
    }

    auto NanWindow::Builder::set_height(int height) noexcept -> Builder & {
        m_height = height;
        return *this;
    }

    auto NanWindow::Builder::set_size(int width, int height) noexcept -> Builder & {
        m_width = width;
        m_height = height;
        return *this;
    }

    auto NanWindow::Builder::set_resizable(bool resizable) noexcept -> Builder & {
        m_resizable = resizable;
        return *this;
    }

    auto NanWindow::Builder::set_high_dpi(bool high_dpi) noexcept -> Builder & {
        m_high_dpi = high_dpi;
        return *this;
    }

    auto NanWindow::Builder::get_title() const noexcept -> std::string_view {
        return m_title;
    }

    auto NanWindow::Builder::get_width() const noexcept -> int {
        return m_width;
    }

    auto NanWindow::Builder::get_height() const noexcept -> int {
        return m_height;
    }

    auto NanWindow::Builder::to_config() const -> Config {
        return Config{
            .title = m_title,
            .width = m_width,
            .height = m_height,
            .resizable = m_resizable,
            .high_dpi = m_high_dpi,
        };
    }

    auto NanWindow::Builder::build() -> NanWindow {
        return NanWindow{to_config()};
    }

} // namespace nandina::runtime
