module;

// ============================================================
// 全局模块片段：SDL3 + ThorVG + 标准库
// 所有平台相关类型隔离于此，不通过模块接口泄漏给消费方。
// ============================================================
#include <SDL3/SDL.h>
#include <thorvg-1/thorvg.h>

#include <chrono>
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
        using nandina::types::PointerButton;
        using SteadyClock = std::chrono::steady_clock;

        constexpr double slow_resize_threshold_ms = 4.0;
        constexpr double slow_frame_threshold_ms  = 8.0;
        constexpr double slow_stage_threshold_ms  = 4.0;
        constexpr std::uint64_t slow_log_interval_ms = 250;
        constexpr std::uint64_t interactive_resize_window_ms = 120;

        [[nodiscard]] auto elapsed_ms(SteadyClock::time_point start, SteadyClock::time_point end) noexcept -> double {
            return std::chrono::duration<double, std::milli>(end - start).count();
        }

        [[nodiscard]] auto should_emit_slow_log(std::uint64_t& last_tick_ms) noexcept -> bool {
            const auto now_tick_ms = static_cast<std::uint64_t>(SDL_GetTicks());
            if (last_tick_ms == 0 || now_tick_ms < last_tick_ms || now_tick_ms - last_tick_ms >= slow_log_interval_ms) {
                last_tick_ms = now_tick_ms;
                return true;
            }
            return false;
        }

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

        auto runtime_bootstrap() -> RuntimeBootstrap& {
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
            auto operator()(SDL_Window* w) const noexcept -> void {
                if (w)
                    SDL_DestroyWindow(w);
            }
        };

        struct RendererDeleter {
            auto operator()(SDL_Renderer* r) const noexcept -> void {
                if (r)
                    SDL_DestroyRenderer(r);
            }
        };

        struct TextureDeleter {
            auto operator()(SDL_Texture* t) const noexcept -> void {
                if (t)
                    SDL_DestroyTexture(t);
            }
        };

        struct CursorDeleter {
            auto operator()(SDL_Cursor* c) const noexcept -> void {
                if (c)
                    SDL_DestroyCursor(c);
            }
        };

        // ── SDL 资源 ─────────────────────────────────────────────
        std::unique_ptr<SDL_Window, WindowDeleter> window;
        std::unique_ptr<SDL_Renderer, RendererDeleter> renderer;
        std::unique_ptr<SDL_Texture, TextureDeleter> texture;
        std::unique_ptr<SDL_Cursor, CursorDeleter> default_cursor;

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
        std::uint64_t last_slow_resize_log_ms{0};
        std::uint64_t last_slow_frame_log_ms{0};
        std::uint64_t last_resize_event_ms{0};

        auto reset_cursor() const noexcept -> void {
            if (default_cursor) {
                SDL_SetCursor(default_cursor.get());
            } else {
                SDL_SetCursor(nullptr);
            }
        }

        [[nodiscard]] auto has_input_focus() const noexcept -> bool {
            return window && (SDL_GetWindowFlags(window.get()) & SDL_WINDOW_INPUT_FOCUS) != 0;
        }

        auto ensure_input_focus() const noexcept -> void {
            if (!window || has_input_focus()) {
                return;
            }
            SDL_RaiseWindow(window.get());
        }

        // ── 辅助：重建 SDL 纹理 + ThorVG 画布（供 resize 复用）─
        auto rebuild_surface(int new_w, int new_h) -> void {
            if (new_w <= 0 || new_h <= 0) {
                return;
            }

            const auto rebuild_start = SteadyClock::now();

            // 重建 SDL 流式纹理
            auto* raw = SDL_CreateTexture(
                renderer.get(), SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, new_w, new_h);
            if (!raw) {
                throw std::runtime_error(std::format("SDL_CreateTexture failed: {}", SDL_GetError()));
            }
            texture.reset(raw);
            const auto texture_ready = SteadyClock::now();

            // 复用既有容量，避免 resize 期间每次都整块清零整帧缓冲区。
            // ThorVG 在 draw(true) 阶段会刷新当前目标内容，这里只需要保证容量足够。
            pixel_buffer.resize(static_cast<std::size_t>(new_w) * static_cast<std::size_t>(new_h));
            const auto buffer_ready = SteadyClock::now();

            // 重绑 ThorVG 画布到新缓冲区
            const auto result = canvas->target(pixel_buffer.data(),
                static_cast<std::uint32_t>(new_w), // stride（以像素计）
                static_cast<std::uint32_t>(new_w),
                static_cast<std::uint32_t>(new_h),
                tvg::ColorSpace::ARGB8888);

            if (result != tvg::Result::Success) {
                throw std::runtime_error("ThorVG SwCanvas::target rebind failed");
            }
            const auto target_ready = SteadyClock::now();

            width  = new_w;
            height = new_h;

            const auto texture_ms = elapsed_ms(rebuild_start, texture_ready);
            const auto buffer_ms  = elapsed_ms(texture_ready, buffer_ready);
            const auto target_ms  = elapsed_ms(buffer_ready, target_ready);
            const auto total_ms   = elapsed_ms(rebuild_start, target_ready);

            if (total_ms >= slow_resize_threshold_ms && should_emit_slow_log(last_slow_resize_log_ms)) {
                auto log = nandina::log::get("runtime.nan_window");
                log.warn(
                    "Slow resize rebuild: total={:.2f}ms texture={:.2f}ms buffer={:.2f}ms target={:.2f}ms size={}x{}",
                    total_ms,
                    texture_ms,
                    buffer_ms,
                    target_ms,
                    new_w,
                    new_h);
            }
        }
    };

    // ============================================================
    // NanWindow 构造（私有，仅由 Builder::build() 调用）
    // ============================================================
    NanWindow::NanWindow(const Config& config) : NanWindow(config.title, config.width, config.height, config.resizable,
        config.high_dpi) {
    }

    NanWindow::NanWindow(std::string_view title, int width, int height, bool resizable, bool high_dpi) : m_impl(
        std::make_unique<Impl>()) {
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

        m_impl->title  = std::string(title);
        m_impl->width  = width;
        m_impl->height = height;

        // 允许点击聚焦时同时触发鼠标按键事件（修复首次进入窗口无法点击按钮的问题）
        SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

        SDL_Window* raw_window     = nullptr;
        SDL_Renderer* raw_renderer = nullptr;

        if (!SDL_CreateWindowAndRenderer(m_impl->title.c_str(), width, height, flags, &raw_window, &raw_renderer)) {
            runtime_bootstrap().release();
            m_impl->runtime_acquired = false;
            throw std::runtime_error(std::format("SDL_CreateWindowAndRenderer failed: {}", SDL_GetError()));
        }
        m_impl->window.reset(raw_window);
        m_impl->renderer.reset(raw_renderer);

        if (auto* cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT)) {
            m_impl->default_cursor.reset(cursor);
            m_impl->reset_cursor();
        } else {
            log.warn("Failed to create default cursor: {}", SDL_GetError());
        }

        if (!SDL_SetRenderVSync(m_impl->renderer.get(), 1)) {
            log.warn("Failed to enable renderer vsync: {}", SDL_GetError());
        }

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

        int renderer_vsync = 0;
        if (!SDL_GetRenderVSync(m_impl->renderer.get(), &renderer_vsync)) {
            renderer_vsync = 0;
        }

        log.info("Window ready: \"{}\" {}x{} (DPI={:.2f}, renderer={}, vsync={})",
            title,
            width,
            height,
            m_impl->dpi_scale,
            SDL_GetRendererName(m_impl->renderer.get()),
            renderer_vsync);
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
    NanWindow::NanWindow(NanWindow&&) noexcept = default;

    NanWindow& NanWindow::operator=(NanWindow&&) noexcept = default;

    // ============================================================
    // 属性访问
    // ============================================================
    auto NanWindow::title() const noexcept -> std::string_view {
        //return m_impl ? m_impl->title : "";
        if (m_impl == nullptr) {
            return {};
        }
        return m_impl->title;
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

    auto NanWindow::request_close() const noexcept -> void {
        if (m_impl)
            m_impl->should_close = true;
    }

    // ============================================================
    // 事件处理（Issue 010 基础通路）
    // ============================================================
    auto NanWindow::poll_events() -> bool {
        if (!m_impl)
            return true;

        bool has_pending_resize = false;
        int pending_logical_w   = 0;
        int pending_logical_h   = 0;

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                m_impl->should_close = true;
                on_close_requested();
                break;

            case SDL_EVENT_WINDOW_RESIZED: {
                has_pending_resize = true;
                pending_logical_w  = ev.window.data1;
                pending_logical_h  = ev.window.data2;
                break;
            }

            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                m_impl->reset_cursor();
                on_focus_gained();
                break;

            case SDL_EVENT_WINDOW_FOCUS_LOST:
                on_focus_lost();
                break;

            case SDL_EVENT_MOUSE_MOTION: {
                on_pointer_move(PointerMoveEvent{
                    .x = static_cast<double>(ev.motion.x),
                    .y = static_cast<double>(ev.motion.y),
                    .delta_x = static_cast<double>(ev.motion.xrel),
                    .delta_y = static_cast<double>(ev.motion.yrel),
                });
                break;
            }

            case SDL_EVENT_WINDOW_MOUSE_ENTER: {
                float mouse_x = 0.0f;
                float mouse_y = 0.0f;
                SDL_GetMouseState(&mouse_x, &mouse_y);
                m_impl->ensure_input_focus();
                m_impl->reset_cursor();
                on_pointer_enter(PointerMoveEvent{
                    .x = static_cast<double>(mouse_x),
                    .y = static_cast<double>(mouse_y),
                    .delta_x = 0.0,
                    .delta_y = 0.0,
                });
                break;
            }

            case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
                float mouse_x = 0.0f;
                float mouse_y = 0.0f;
                SDL_GetMouseState(&mouse_x, &mouse_y);
                m_impl->reset_cursor();
                on_pointer_leave(PointerMoveEvent{
                    .x = static_cast<double>(mouse_x),
                    .y = static_cast<double>(mouse_y),
                    .delta_x = 0.0,
                    .delta_y = 0.0,
                });
                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                m_impl->ensure_input_focus();
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

        if (has_pending_resize) {
            auto log = nandina::log::get("runtime.nan_window");
            m_impl->last_resize_event_ms = static_cast<std::uint64_t>(SDL_GetTicks());
            int pixel_w = 0;
            int pixel_h = 0;
            SDL_GetWindowSizeInPixels(m_impl->window.get(), &pixel_w, &pixel_h);
            log.debug("Window resized: logical={}x{}, pixel={}x{}", pending_logical_w, pending_logical_h, pixel_w, pixel_h);

            if (pixel_w != m_impl->width || pixel_h != m_impl->height) {
                try {
                    m_impl->rebuild_surface(pixel_w, pixel_h);
                } catch (const std::exception& e) {
                    log.error("rebuild_surface failed on resize: {}", e.what());
                }
            }

            m_impl->dpi_scale = SDL_GetWindowDisplayScale(m_impl->window.get());
            on_resize(pending_logical_w, pending_logical_h);
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

        auto& canvas = *m_impl->canvas;
        const auto frame_start = SteadyClock::now();

        // 1. 清空上一帧所有 ThorVG 绘制指令
        canvas.remove(nullptr);
        const auto remove_done = SteadyClock::now();

        // 2. 用户回调：向 canvas 添加本帧 ThorVG 图元
        on_draw(canvas);
        const auto build_done = SteadyClock::now();

        // 3. ThorVG 光栅化到 pixel_buffer（clear=true 每帧刷新背景）
        if (canvas.draw(true) != tvg::Result::Success) {
            return;
        }
        const auto draw_done = SteadyClock::now();
        if (canvas.sync() != tvg::Result::Success) {
            return;
        }
        const auto sync_done = SteadyClock::now();

        // 4. pixel_buffer → SDL 流式纹理（CPU → GPU 上传）
        if (!SDL_UpdateTexture(m_impl->texture.get(),
            nullptr,
            m_impl->pixel_buffer.data(),
            m_impl->width * static_cast<int>(sizeof(std::uint32_t)))) {
            return;
        }
        const auto upload_done = SteadyClock::now();

        // 5. SDL 渲染器：清空 → 贴纹理 → 呈现
        if (!SDL_RenderClear(m_impl->renderer.get())) {
            return;
        }
        const auto clear_done = SteadyClock::now();
        if (!SDL_RenderTexture(m_impl->renderer.get(), m_impl->texture.get(), nullptr, nullptr)) {
            return;
        }
        const auto texture_done = SteadyClock::now();
        SDL_RenderPresent(m_impl->renderer.get());
        const auto present_done = SteadyClock::now();

        const auto remove_ms  = elapsed_ms(frame_start, remove_done);
        const auto build_ms   = elapsed_ms(remove_done, build_done);
        const auto draw_ms    = elapsed_ms(build_done, draw_done);
        const auto sync_ms    = elapsed_ms(draw_done, sync_done);
        const auto upload_ms  = elapsed_ms(sync_done, upload_done);
        const auto clear_ms   = elapsed_ms(upload_done, clear_done);
        const auto texture_ms = elapsed_ms(clear_done, texture_done);
        const auto present_ms = elapsed_ms(texture_done, present_done);
        const auto total_ms   = elapsed_ms(frame_start, present_done);

        const bool slow_frame = total_ms >= slow_frame_threshold_ms || build_ms >= slow_stage_threshold_ms
            || draw_ms >= slow_stage_threshold_ms || sync_ms >= slow_stage_threshold_ms
            || upload_ms >= slow_stage_threshold_ms || present_ms >= slow_stage_threshold_ms;

        if (slow_frame && should_emit_slow_log(m_impl->last_slow_frame_log_ms)) {
            auto log = nandina::log::get("runtime.nan_window");
            log.warn(
                "Slow present_frame: total={:.2f}ms remove={:.2f}ms build={:.2f}ms draw={:.2f}ms sync={:.2f}ms upload={:.2f}ms clear={:.2f}ms copy={:.2f}ms present={:.2f}ms size={}x{}",
                total_ms,
                remove_ms,
                build_ms,
                draw_ms,
                sync_ms,
                upload_ms,
                clear_ms,
                texture_ms,
                present_ms,
                m_impl->width,
                m_impl->height);
        }
    }

    // ============================================================
    // 主循环阻塞接口
    // ============================================================
    auto NanWindow::run() -> void {
        auto last_counter = SDL_GetPerformanceCounter();
        bool ready_called = false;
        constexpr double target_frame_seconds = 1.0 / 60.0;

        while (!poll_events()) {
            if (!ready_called) {
                on_ready();
                ready_called = true;
            }

            const auto current_counter = SDL_GetPerformanceCounter();
            const auto freq            = static_cast<double>(SDL_GetPerformanceFrequency());
            const double delta_seconds = freq > 0.0 ? static_cast<double>(current_counter - last_counter) / freq : 0.0;
            last_counter               = current_counter;

            on_update(delta_seconds);

            if (should_present_frame()) {
                present_frame();
            }

            const auto frame_end_counter = SDL_GetPerformanceCounter();
            const auto frame_freq        = static_cast<double>(SDL_GetPerformanceFrequency());
            const double frame_seconds   = frame_freq > 0.0
                ? static_cast<double>(frame_end_counter - current_counter) / frame_freq
                : 0.0;

            const auto now_tick_ms = static_cast<std::uint64_t>(SDL_GetTicks());
            const bool recent_resize = m_impl && m_impl->last_resize_event_ms != 0
                && now_tick_ms >= m_impl->last_resize_event_ms
                && now_tick_ms - m_impl->last_resize_event_ms <= interactive_resize_window_ms;

            if (!recent_resize && frame_seconds < target_frame_seconds) {
                const auto delay_ms = static_cast<Uint32>((target_frame_seconds - frame_seconds) * 1000.0);
                if (delay_ms > 0) {
                    SDL_Delay(delay_ms);
                }
            }
        }
    }

    auto NanWindow::on_ready() -> void {
    }

    auto NanWindow::on_update(double) -> void {
    }

    auto NanWindow::on_draw(tvg::SwCanvas&) -> void {
    }

    auto NanWindow::should_present_frame() const noexcept -> bool {
        return true;
    }

    auto NanWindow::on_resize(int, int) -> void {
    }

    auto NanWindow::on_focus_gained() -> void {
    }

    auto NanWindow::on_focus_lost() -> void {
    }

    auto NanWindow::on_close_requested() -> void {
    }

    auto NanWindow::on_pointer_move(const PointerMoveEvent&) -> void {
    }

    auto NanWindow::on_pointer_enter(const PointerMoveEvent&) -> void {
    }

    auto NanWindow::on_pointer_leave(const PointerMoveEvent&) -> void {
    }

    auto NanWindow::on_pointer_down(const PointerButtonEvent&) -> void {
    }

    auto NanWindow::on_pointer_up(const PointerButtonEvent&) -> void {
    }

    auto NanWindow::on_pointer_wheel(const PointerWheelEvent&) -> void {
    }

    auto NanWindow::on_key_down(const KeyEvent&) -> void {
    }

    auto NanWindow::on_key_up(const KeyEvent&) -> void {
    }

    auto NanWindow::on_text_input(std::string_view) -> void {
    }

    // ============================================================
    // Builder 实现
    // ============================================================
    auto NanWindow::Builder::set_title(std::string_view title) noexcept -> Builder& {
        m_title = std::string(title);
        return *this;
    }

    auto NanWindow::Builder::set_width(int width) noexcept -> Builder& {
        m_width = width;
        return *this;
    }

    auto NanWindow::Builder::set_height(int height) noexcept -> Builder& {
        m_height = height;
        return *this;
    }

    auto NanWindow::Builder::set_size(int width, int height) noexcept -> Builder& {
        m_width  = width;
        m_height = height;
        return *this;
    }

    auto NanWindow::Builder::set_resizable(bool resizable) noexcept -> Builder& {
        m_resizable = resizable;
        return *this;
    }

    auto NanWindow::Builder::set_high_dpi(bool high_dpi) noexcept -> Builder& {
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

    auto NanWindow::Builder::build() const -> NanWindow {
        return NanWindow{to_config()};
    }
} // namespace nandina::runtime
