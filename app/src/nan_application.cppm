module;

#include <memory>
#include <string>
#include <thorvg-1/thorvg.h>

export module nandina.app.application;

import nandina.log;
import nandina.runtime.nan_window;
import nandina.runtime.nan_widget;

export namespace nandina::app {
    class NanComponent : public runtime::NanWidget {
    public:
        using Ptr = std::unique_ptr<NanComponent>;

        ~NanComponent() override = default;
    };

    struct AppConfig {
        std::string title = "NandinaUI";
        int width = 1280;
        int height = 720;
        bool resizable = true;
        bool high_dpi = true;
    };

    class NanAppWindow {
    public:
        explicit NanAppWindow(const AppConfig &config) : m_config(config) {
        }

        virtual ~NanAppWindow() {
        }

        auto set_root_component(NanComponent::Ptr component) -> void {
            m_root_component = std::move(component);
            if (m_root_component && m_active_runtime_window) {
                m_root_component->set_size(static_cast<float>(m_active_runtime_window->width()),
                                           static_cast<float>(m_active_runtime_window->height()));
            }
        }

        auto run() -> void;

        [[nodiscard]] auto width() const noexcept -> int {
            return m_active_runtime_window ? m_active_runtime_window->width() : m_config.width;
        }

        [[nodiscard]] auto height() const noexcept -> int {
            return m_active_runtime_window ? m_active_runtime_window->height() : m_config.height;
        }

    protected:
        virtual void on_ready() {
        }

        virtual void on_update(double delta_seconds) {
            (void)delta_seconds;
        }

        virtual void on_draw(tvg::SwCanvas &canvas) {
            if (m_root_component) {
                m_root_component->draw(canvas);
            }
        }

    private:
        AppConfig m_config;
        runtime::NanWindow *m_active_runtime_window{nullptr};
        NanComponent::Ptr m_root_component{nullptr};

        class BridgeWindow final : public runtime::NanWindow {
        public:
            BridgeWindow(NanAppWindow &owner,
                         const runtime::NanWindow::Config &config) : runtime::NanWindow(config),
                                                                     m_owner(owner) {
            }

        protected:
            void on_ready() override {
                m_owner.on_ready();
            }

            void on_update(double delta) override {
                m_owner.on_update(delta);
            }

            void on_draw(tvg::SwCanvas &canvas) override {
                m_owner.on_draw(canvas);
            }

            void on_resize(int w, int h) override {
                if (m_owner.m_root_component) {
                    m_owner.m_root_component->set_size(static_cast<float>(w), static_cast<float>(h));
                }
            }

        private:
            NanAppWindow &m_owner;
        };
    };

    class NanApplication {
    public:
        NanApplication() {
            log::init("nandina", log::Level::Debug);
        }

        virtual ~NanApplication() {
            log::shutdown();
        }

        // Take ownership via unique_ptr in implementation but maybe GCC dislikes it in header?
        // Let's keep it but make sure the TU where it is used is clean.
        auto run(std::unique_ptr<NanAppWindow> window) -> void {
            if (!window)
                return;
            m_window = std::move(window);
            m_window->run();
        }

    private:
        std::unique_ptr<NanAppWindow> m_window;
    };

    inline auto NanAppWindow::run() -> void {
        auto logger = log::get("app.window");

        runtime::NanWindow::Config runtime_config{
            .title = m_config.title,
            .width = m_config.width,
            .height = m_config.height,
            .resizable = m_config.resizable,
            .high_dpi = m_config.high_dpi
        };

        BridgeWindow window{*this, runtime_config};
        m_active_runtime_window = &window;

        if (m_root_component) {
            m_root_component->set_size(static_cast<float>(window.width()), static_cast<float>(window.height()));
        }

        logger.info("Window starting: {} ({}x{})", m_config.title, m_config.width, m_config.height);
        window.run();
        logger.info("Window stopped");

        m_active_runtime_window = nullptr;
    }
} // namespace nandina::app