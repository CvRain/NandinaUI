//
// Created by cvrain on 2026/7/4.
//
// raylib-backed implementation of NanWindow. raylib is included ONLY here.
//

#include "nan_window.hpp"
#include "nan_application.hpp"

#if defined(__linux__)
    #include "detail/linux_clipboard.hpp"
#endif

#include "../foundation/nan_logger.hpp"
#include "../foundation/utf8.hpp"
#include "../render/backends/raylib_device.hpp"
#include "../render/draw_context.hpp"
#include "../scene/clipboard.hpp"
#include "../scene/control.hpp"
#include "../scene/input_event.hpp"

#include <raylib.h>

#include <cstdlib>
#include <utility>

namespace nandina::app
{
    namespace
    {

        /// Snapshot raylib modifier-key state into a KeyModifiers.
        auto current_modifiers() -> scene::KeyModifiers {
            return scene::KeyModifiers {
                .shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT),
                .ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL),
                .alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT),
                .super = IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER),
            };
        }

        void dispatch_mouse_button(
            scene::NanSceneTree& tree,
            foundation::NanPoint pos,
            scene::MouseButtonEvent::Button button,
            int rl_button
        ) {
            const auto mods = current_modifiers();
            if (IsMouseButtonPressed(rl_button)) {
                scene::MouseButtonEvent
                    ev {button, scene::MouseButtonEvent::Action::press, pos, mods};
                tree.dispatch_mouse_button(ev);
            }
            if (IsMouseButtonReleased(rl_button)) {
                scene::MouseButtonEvent
                    ev {button, scene::MouseButtonEvent::Action::release, pos, mods};
                tree.dispatch_mouse_button(ev);
            }
        }

        class DesktopClipboard final: public scene::IClipboard {
        public:
            [[nodiscard]] auto read_text() const -> std::optional<std::string> override {
#if defined(__linux__)
                if (detail::is_wayland_session(std::getenv("WAYLAND_DISPLAY"))) {
                    if (auto text = detail::read_wayland_clipboard()) {
                        return text;
                    }
                }
#endif
                const auto* text = GetClipboardText();
                return text != nullptr ? std::optional<std::string>(text) : std::nullopt;
            }

            auto write_text(const std::string_view text) -> bool override {
                if (text.find('\0') != std::string_view::npos) {
                    return false;
                }
#if defined(__linux__)
                if (detail::is_wayland_session(std::getenv("WAYLAND_DISPLAY"))
                    && detail::write_wayland_clipboard(text))
                {
                    return true;
                }
#endif
                const std::string owned(text);
                SetClipboardText(owned.c_str());
                return true;
            }
        };

        DesktopClipboard desktop_clipboard;

    } // namespace

    NanWindow::NanWindow(NanApplication& app, WindowConfig config):
        app_(app),
        config_(std::move(config)) {
        tree_.set_theme_manager(app_.theme_manager());
    }

    NanWindow::~NanWindow() {
        // 若 run 未显式 close (异常路径), 兜底关闭窗口。
        if (opened_ && IsWindowReady()) {
            close();
        }
    }

    void NanWindow::set_content(std::shared_ptr<scene::NanNode2D> root) {
        tree_.set_root(std::move(root));
    }

    auto NanWindow::use_router() -> NanRouter& {
        router_ = std::make_unique<NanRouter>(
            app_.graph(),
            app_.theme_manager(),
            app_.store_base(),
            app_.store_type_key(),
            &app_.resources(),
            &app_.font_loader(),
            &app_.font_families(),
            &app_.dispatcher(),
            &app_.background_executor()
        );
        set_content(router_->host());
        return *router_;
    }

    auto NanWindow::graph() -> reactive::Graph& {
        return app_.graph();
    }

    auto NanWindow::theme() const -> const theme::NanTheme& {
        return app_.theme();
    }

    auto NanWindow::default_text_pipeline() const -> const widget::primitives::TextPipeline* {
        return default_text_pipeline_ ? &*default_text_pipeline_ : nullptr;
    }

    void NanWindow::open() {
        if (opened_) {
            return;
        }

        unsigned int flags = 0;
        if (config_.msaa) {
            flags |= FLAG_MSAA_4X_HINT;
        }
        if (config_.vsync) {
            flags |= FLAG_VSYNC_HINT;
        }
        if (config_.high_dpi) {
            flags |= FLAG_WINDOW_HIGHDPI;
        }
        if (config_.resizable) {
            flags |= FLAG_WINDOW_RESIZABLE;
        }
        if (!config_.decorated) {
            flags |= FLAG_WINDOW_UNDECORATED;
        }
        SetConfigFlags(flags);

        InitWindow(config_.width, config_.height, config_.title.c_str());
        SetTargetFPS(config_.target_fps);
        tree_.set_clipboard(desktop_clipboard);

        device_ = render::make_raylib_device();
        texture_cache_ = std::make_unique<render::TextureCache>(
            *device_,
            render::TextureCacheLimits {},
            render::TextureCacheAsyncServices {
                .decoder = render::make_raylib_image_decoder(),
                .submit_background =
                    [this](std::move_only_function<void()> task) {
                        return app_.background_executor().submit(std::move(task));
                    },
                .post_ui = [this](
                               std::move_only_function<void()> task
                           ) { return app_.dispatcher().post(std::move(task)); },
                .max_uploads_per_frame = 4,
            }
        );
        tree_.set_texture_cache(*texture_cache_);
        font_pipeline_cache_ = std::make_unique<text::FontPipelineCache>(
            *device_,
            app_.font_loader(),
            app_.font_families()
        );
        tree_.set_font_context(*font_pipeline_cache_);
        auto pipeline = font_pipeline_cache_->get({});
        if (!pipeline) {
            tree_.clear_font_context();
            font_pipeline_cache_.reset();
            tree_.clear_texture_cache();
            texture_cache_.reset();
            device_.reset();
            tree_.clear_clipboard();
            CloseWindow();
            throw std::runtime_error(
                "NanWindow: cannot create default text pipeline: " + pipeline.error().message
            );
        }
        default_font_pipeline_ = *pipeline;
        default_text_pipeline_ = default_font_pipeline_->pipeline();
        opened_ = true;

        log::get("app.window")
            .info("NanWindow: opened {}x{} \"{}\"", config_.width, config_.height, config_.title);
        on_setup();
    }

    auto NanWindow::should_close() const -> bool {
        return WindowShouldClose();
    }

    void NanWindow::poll_and_dispatch_input() {
        const auto mx = static_cast<float>(GetMouseX());
        const auto my = static_cast<float>(GetMouseY());
        const foundation::NanPoint screen_pos {mx, my};
        const auto pos =
            viewport_mapping_ ? viewport_mapping_->screen_to_logical(screen_pos) : screen_pos;

        // Mouse move (only when actually moved).
        if (!has_mouse_) {
            last_mouse_x_ = mx;
            last_mouse_y_ = my;
            has_mouse_ = true;
        }
        if (mx != last_mouse_x_ || my != last_mouse_y_) {
            const auto previous = viewport_mapping_
                ? viewport_mapping_->screen_to_logical(
                      foundation::NanPoint {last_mouse_x_, last_mouse_y_}
                  )
                : foundation::NanPoint {last_mouse_x_, last_mouse_y_};
            scene::MouseMoveEvent ev {
                pos,
                foundation::NanPoint {
                    pos.get_x() - previous.get_x(),
                    pos.get_y() - previous.get_y()
                }
            };
            tree_.dispatch_mouse_move(ev);
            last_mouse_x_ = mx;
            last_mouse_y_ = my;
        }

        // Mouse buttons.
        dispatch_mouse_button(tree_, pos, scene::MouseButtonEvent::Button::left, MOUSE_BUTTON_LEFT);
        dispatch_mouse_button(
            tree_,
            pos,
            scene::MouseButtonEvent::Button::right,
            MOUSE_BUTTON_RIGHT
        );
        dispatch_mouse_button(
            tree_,
            pos,
            scene::MouseButtonEvent::Button::middle,
            MOUSE_BUTTON_MIDDLE
        );

        // Mouse wheel.
        if (const float wheel = GetMouseWheelMove(); wheel != 0.0F) {
            scene::MouseWheelEvent ev {
                pos,
                foundation::NanPoint {0.0F, wheel},
                current_modifiers()
            };
            tree_.dispatch_mouse_wheel(ev);
        }

        // Keyboard: drain the pressed-key queue.
        for (int key = GetKeyPressed(); key != 0; key = GetKeyPressed()) {
            scene::KeyEvent ev {key, scene::KeyEvent::Action::press, current_modifiers()};
            tree_.dispatch_key(ev);
        }

        // Text input: drain the char queue (already composed unicode codepoints).
        for (int codepoint = GetCharPressed(); codepoint != 0; codepoint = GetCharPressed()) {
            if (codepoint >= 32) {
                tree_.dispatch_text_input(
                    scene::TextInputEvent {
                        foundation::utf8::encode(static_cast<char32_t>(codepoint))
                    }
                );
            }
        }
    }

    void NanWindow::update_viewport_mapping(const foundation::NanSize screen_size) {
        if (!config_.viewport) {
            viewport_mapping_.reset();
            return;
        }
        viewport_mapping_ = make_viewport_mapping(screen_size, *config_.viewport);
    }

    void NanWindow::tick() {
        const float dt = GetFrameTime();
        const auto metrics = make_window_metrics(
            foundation::NanSize(
                static_cast<float>(GetScreenWidth()),
                static_cast<float>(GetScreenHeight())
            ),
            foundation::NanSize(
                static_cast<float>(GetRenderWidth()),
                static_cast<float>(GetRenderHeight())
            )
        );
        update_viewport_mapping(metrics.screen_size);
        auto deferred_effects = app_.graph().defer_effects();
        if (texture_cache_) {
            texture_cache_->begin_frame();
        }

        {
            auto phase = tree_.enter_phase(scene::FramePhase::input);
            poll_and_dispatch_input();
        }

        {
            auto phase = tree_.enter_phase(scene::FramePhase::tasks);
            (void)app_.dispatcher().drain();
        }

        {
            auto phase = tree_.enter_phase(scene::FramePhase::process);
            tree_.process(dt);
            on_frame(dt);
        }

        {
            auto phase = tree_.enter_phase(scene::FramePhase::tree_commit);
            tree_.flush_tree_mutations();
            tree_.flush_deferred_deletes();
        }

        {
            auto phase = tree_.enter_phase(scene::FramePhase::physics);
            tree_.physics_step(dt);
        }

        {
            auto phase = tree_.enter_phase(scene::FramePhase::reactive);
            deferred_effects.commit();
        }

        {
            auto phase = tree_.enter_phase(scene::FramePhase::animation);
            tree_.advance_animations(dt);
        }

        const auto logical_size =
            viewport_mapping_ ? viewport_mapping_->logical_size : metrics.screen_size;
        (void)tree_.layout_root(logical_size);

        {
            auto phase = tree_.enter_phase(scene::FramePhase::semantics);
            tree_.set_semantics_transform(
                viewport_mapping_ ? viewport_mapping_->transform()
                                  : foundation::NanTransform2D::identity()
            );
            (void)tree_.update_semantics();
        }

        device_->begin_frame();
        // 清屏背景：显式配置优先，否则跟随当前外观的调色板背景（light/dark 实时切换）。
        device_->clear(config_.background.value_or(theme().palette.background));
        {
            render::DrawContext ctx {
                *device_,
                viewport_mapping_ ? viewport_mapping_->transform() : foundation::NanTransform2D {},
                {
                    .logical_to_screen = viewport_mapping_ ? viewport_mapping_->scale : 1.0F,
                    .screen_to_physical = metrics.screen_to_physical,
                }
            };
            tree_.render(ctx);
        }
        device_->end_frame();

        {
            auto phase = tree_.enter_phase(scene::FramePhase::dispose);
            tree_.flush_tree_mutations();
            tree_.flush_deferred_deletes();
        }
    }

    void NanWindow::close() {
        if (!opened_) {
            return;
        }
        // 释放场景树 (触发 widget 卸载, 回访 graph) 后再释放设备、关窗口。
        on_teardown();
        if (router_) {
            router_->clear();
        }
        tree_.set_root(nullptr);
        tree_.clear_default_text_pipeline();
        tree_.clear_font_context();
        tree_.clear_texture_cache();
        default_text_pipeline_.reset();
        default_font_pipeline_.reset();
        font_pipeline_cache_.reset();
        texture_cache_.reset();
        router_.reset();
        device_.reset();
        tree_.clear_clipboard();
        CloseWindow();
        opened_ = false;
        log::get("app.window").info("NanWindow: closed");
    }

} // namespace nandina::app
