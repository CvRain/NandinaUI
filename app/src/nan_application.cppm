module;

// ============================================================
// 全局模块片段
// ThorVG 在接口层可见——on_draw 回调参数类型涉及 tvg::SwCanvas。
// 消费者（继承 NanApplication 并重写 on_draw 的用户代码）
// 需在其 TU 的 GMF 中 include ThorVG 才能使用 canvas 参数。
// ============================================================
#include <thorvg-1/thorvg.h>

#include <stdexcept>
#include <string>

export module nandina.app.application;

import nandina.log;
import nandina.runtime.nan_window;

// ============================================================
// 导出接口
// ============================================================
export namespace nandina {

    // ──────────────────────────────────────────────────────────
    // AppConfig — 窗口与应用参数，由 configure() 返回
    //
    // 用法：在 configure() 中用指定初始化器覆盖所需字段：
    //   auto configure() -> AppConfig override {
    //       return { .title = "My App", .width = 1920, .height = 1080 };
    //   }
    // ──────────────────────────────────────────────────────────
    struct AppConfig {
        std::string title     = "NandinaUI";
        int         width     = 1280;
        int         height    = 720;
        bool        resizable = true;
        bool        high_dpi  = true;
    };

    // ──────────────────────────────────────────────────────────
    // NanApplication — Godot 风格应用基类（M1）
    //
    // 开发者继承 NanApplication 并重写生命周期函数：
    //   - on_ready() / on_update() / on_draw()
    //   - on_resize() / on_close_requested() / on_shutdown()
    //   - on_pointer_*() / on_key_*() / on_text_input()
    //
    // run() 内部会构造桥接窗口（继承 NanWindow），并将窗口
    // 生命周期事件转发到当前应用实例，实现脚本式体验。
    class NanApplication {
    public:
        NanApplication() = default;
        NanApplication(const NanApplication &) = delete;
        NanApplication &operator=(const NanApplication &) = delete;
        NanApplication(NanApplication &&) = delete;
        NanApplication &operator=(NanApplication &&) = delete;
        virtual ~NanApplication() = default;

        auto run() -> void;
        auto request_quit() noexcept -> void;

        [[nodiscard]] auto is_running() const noexcept -> bool {
            return m_running;
        }

        [[nodiscard]] auto window_width() const noexcept -> int;
        [[nodiscard]] auto window_height() const noexcept -> int;
        [[nodiscard]] auto window_dpi_scale() const noexcept -> float;

    protected:
        [[nodiscard]] virtual auto configure() -> AppConfig;
        virtual auto on_ready() -> void;
        virtual auto on_update(double delta_seconds) -> void;
        virtual auto on_draw(tvg::SwCanvas &canvas) -> void;
        virtual auto on_resize(int new_width, int new_height) -> void;
        virtual auto on_close_requested() -> void;
        virtual auto on_pointer_move(const nandina::runtime::PointerMoveEvent &event) -> void;
        virtual auto on_pointer_down(const nandina::runtime::PointerButtonEvent &event) -> void;
        virtual auto on_pointer_up(const nandina::runtime::PointerButtonEvent &event) -> void;
        virtual auto on_pointer_wheel(const nandina::runtime::PointerWheelEvent &event) -> void;
        virtual auto on_key_down(const nandina::runtime::KeyEvent &event) -> void;
        virtual auto on_key_up(const nandina::runtime::KeyEvent &event) -> void;
        virtual auto on_text_input(std::string_view text) -> void;
        virtual auto on_shutdown() -> void;

        [[nodiscard]] auto window() noexcept -> nandina::runtime::NanWindow * {
            return m_active_window;
        }

        [[nodiscard]] auto window() const noexcept -> const nandina::runtime::NanWindow * {
            return m_active_window;
        }

    private:
        nandina::runtime::NanWindow *m_active_window{nullptr};
        bool m_running{false};
    };

    auto NanApplication::run() -> void {
        if (m_running) {
            throw std::logic_error("NanApplication::run() cannot be called while already running");
        }

        auto log = nandina::log::get("app.application");
        const auto app_config = configure();

        auto window_config = nandina::runtime::NanWindow::Config{
            .title = app_config.title,
            .width = app_config.width,
            .height = app_config.height,
            .resizable = app_config.resizable,
            .high_dpi = app_config.high_dpi,
        };

        class ApplicationWindow final : public nandina::runtime::NanWindow {
        public:
            ApplicationWindow(NanApplication &app, const Config &config) :
                NanWindow(config),
                m_app(app) {}

        protected:
            auto on_ready() -> void override {
                m_app.on_ready();
            }

            auto on_update(double delta_seconds) -> void override {
                m_app.on_update(delta_seconds);
            }

            auto on_draw(tvg::SwCanvas &canvas) -> void override {
                m_app.on_draw(canvas);
            }

            auto on_resize(int new_width, int new_height) -> void override {
                m_app.on_resize(new_width, new_height);
            }

            auto on_close_requested() -> void override {
                m_app.on_close_requested();
            }

            auto on_pointer_move(const nandina::runtime::PointerMoveEvent &event) -> void override {
                m_app.on_pointer_move(event);
            }

            auto on_pointer_down(const nandina::runtime::PointerButtonEvent &event) -> void override {
                m_app.on_pointer_down(event);
            }

            auto on_pointer_up(const nandina::runtime::PointerButtonEvent &event) -> void override {
                m_app.on_pointer_up(event);
            }

            auto on_pointer_wheel(const nandina::runtime::PointerWheelEvent &event) -> void override {
                m_app.on_pointer_wheel(event);
            }

            auto on_key_down(const nandina::runtime::KeyEvent &event) -> void override {
                m_app.on_key_down(event);
            }

            auto on_key_up(const nandina::runtime::KeyEvent &event) -> void override {
                m_app.on_key_up(event);
            }

            auto on_text_input(std::string_view text) -> void override {
                m_app.on_text_input(text);
            }

        private:
            NanApplication &m_app;
        };

        ApplicationWindow window{*this, window_config};
        m_active_window = &window;
        m_running = true;

        log.info("Application starting: {} ({}x{})", app_config.title, app_config.width, app_config.height);

        try {
            window.run();
        } catch (...) {
            m_running = false;
            m_active_window = nullptr;
            on_shutdown();
            throw;
        }

        m_running = false;
        m_active_window = nullptr;
        on_shutdown();
        log.info("Application stopped");
    }

    auto NanApplication::request_quit() noexcept -> void {
        if (m_active_window) {
            m_active_window->request_close();
        }
    }

    auto NanApplication::window_width() const noexcept -> int {
        return m_active_window ? m_active_window->width() : 0;
    }

    auto NanApplication::window_height() const noexcept -> int {
        return m_active_window ? m_active_window->height() : 0;
    }

    auto NanApplication::window_dpi_scale() const noexcept -> float {
        return m_active_window ? m_active_window->dpi_scale() : 1.0f;
    }

    auto NanApplication::configure() -> AppConfig {
        return {};
    }

    auto NanApplication::on_ready() -> void {}

    auto NanApplication::on_update(double) -> void {}

    auto NanApplication::on_draw(tvg::SwCanvas &) -> void {}

    auto NanApplication::on_resize(int, int) -> void {}

    auto NanApplication::on_close_requested() -> void {}

    auto NanApplication::on_pointer_move(const nandina::runtime::PointerMoveEvent &) -> void {}

    auto NanApplication::on_pointer_down(const nandina::runtime::PointerButtonEvent &) -> void {}

    auto NanApplication::on_pointer_up(const nandina::runtime::PointerButtonEvent &) -> void {}

    auto NanApplication::on_pointer_wheel(const nandina::runtime::PointerWheelEvent &) -> void {}

    auto NanApplication::on_key_down(const nandina::runtime::KeyEvent &) -> void {}

    auto NanApplication::on_key_up(const nandina::runtime::KeyEvent &) -> void {}

    auto NanApplication::on_text_input(std::string_view) -> void {}

    auto NanApplication::on_shutdown() -> void {}

} // namespace nandina