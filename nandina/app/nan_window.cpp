//
// Created by cvrain on 2026/7/4.
//
// raylib-backed implementation of NanWindow. raylib is included ONLY here.
//

#include "nan_window.hpp"

#include "detail/raylib_platform_window.hpp"
#if defined(NANDINA_HAS_SDL_BACKEND)
    #include "detail/sdl_platform_window.hpp"
#endif
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
#include "../scene/overlay_host.hpp"

#include <raylib.h>

#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace nandina::app
{
    namespace
    {

        /// 把 `IPlatformWindow` 的剪贴板接成 `scene::IClipboard`。
        ///
        /// 行为与搬迁前一致：Linux/Wayland 会话下先走项目自己的 Wayland 细节实现
        /// （`app/detail/linux_clipboard`），失败或非 Wayland 时回退到平台实现。
        /// 平台实现由各自后端提供（raylib → GLFW；SDL → SDL 的原生剪贴板）。
        class PlatformClipboard final: public scene::IClipboard {
        public:
            explicit PlatformClipboard(IPlatformWindow& platform): platform_(&platform) {}

            [[nodiscard]] auto read_text() const -> std::optional<std::string> override {
#if defined(__linux__)
                if (detail::is_wayland_session(std::getenv("WAYLAND_DISPLAY"))) {
                    if (auto text = detail::read_wayland_clipboard()) {
                        return text;
                    }
                }
#endif
                auto text = platform_->clipboard_text();
                return text.empty() ? std::nullopt : std::optional<std::string>(std::move(text));
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
                platform_->set_clipboard_text(text);
                return true;
            }

        private:
            IPlatformWindow* platform_ = nullptr;
        };

        /// 按后端创建平台实现。`automatic` 用本构建的默认后端；
        /// 本构建没有 SDL 后端而要求 sdl3 时**不静默回退**，直接抛错（配置错误要早暴露）。
        [[nodiscard]] auto make_platform(WindowBackend backend)
            -> std::unique_ptr<IPlatformWindow> {
            const auto resolved =
                backend == WindowBackend::automatic ? default_window_backend() : backend;
            switch (resolved) {
                case WindowBackend::sdl3:
#if defined(NANDINA_HAS_SDL_BACKEND)
                    return std::make_unique<detail::SdlPlatformWindow>();
#else
                    throw std::invalid_argument(
                        "NanWindow: sdl3 backend requested but this build has no SDL3 support"
                    );
#endif
                case WindowBackend::raylib:
                case WindowBackend::automatic:
                    break;
            }
            return std::make_unique<detail::RaylibPlatformWindow>();
        }
    } // namespace

    NanWindow::NanWindow(NanApplication& app, WindowConfig config):
        app_(app),
        config_(std::move(config)),
        overlay_host_(scene::OverlayHost::create()) {
        tree_.set_theme_manager(app_.theme_manager());
        platform_ = make_platform(config_.backend);
    }

    NanWindow::~NanWindow() {
        // 若 run 未显式 close (异常路径), 兜底关闭窗口。
        if (opened_ && platform_ != nullptr && platform_->is_ready()) {
            close();
        }
    }

    void NanWindow::set_content(std::shared_ptr<scene::NanNode2D> root) {
        if (!root) {
            tree_.set_root(nullptr);
            return;
        }
        auto* control = root->as_control();
        if (control == nullptr) {
            throw std::invalid_argument(
                "NanWindow::set_content: root must be a NanControl so it can live in the "
                "window content layer"
            );
        }
        // The window owns one overlay portal per window. Application content sits in
        // its content layer; floating content presented through `overlay_host()` is
        // drawn above it and escapes parent clipping.
        auto content = std::shared_ptr<scene::NanControl>(std::move(root), control);
        (void)overlay_host_->set_content(std::move(content));
        tree_.set_root(overlay_host_);
    }

    auto NanWindow::overlay_host() -> scene::OverlayHost& {
        return *overlay_host_;
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
            &app_.background_executor(),
            overlay_host_.get()
        );
        // 直接把服务交给 Router：它内部持有指针，页面构造 BuildContext 时即可取到，
        // 不依赖"内容是否已挂载"这种时序（曾因此让 ui.drag_controller() 恒为 nullptr）。
        router_->set_drag_controller(&drag_controller_);
        // 导航在任务阶段提交，调用方（按钮回调）早已返回，失败无法靠异常回报过去。
        // 转到这里由窗口决定怎么呈现；默认实现写 error 日志，不静默。
        router_->set_page_error_handler(
            [this](std::string_view /*route_address*/, std::exception_ptr error) {
                on_error(std::move(error));
            }
        );
        set_content(router_->host());
        return *router_;
    }

    void NanWindow::on_error(std::exception_ptr error) {
        std::string message = "unknown error";
        try {
            std::rethrow_exception(error);
        }
        catch (const std::exception& caught) {
            message = caught.what();
        }
        catch (...) {
            message = "non-standard exception";
        }
        log::error("NanWindow: page error: {}", message);
    }

    auto NanWindow::use_router(Routes routes) -> NanRouter& {
        auto& router = use_router();
        if (const auto configured = router.configure(std::move(routes)); !configured) {
            throw std::invalid_argument("NanWindow::use_router: " + describe(configured.error()));
        }
        return router;
    }

    void NanWindow::set_shell(std::move_only_function<widget::View(ShellContext&)> factory) {
        if (!factory) {
            throw std::invalid_argument("NanWindow::set_shell: factory is empty");
        }
        if (router_ == nullptr || !router_->route_mode()) {
            throw std::logic_error("NanWindow::set_shell: configure Routes first");
        }
        if (shell_installed_) {
            throw std::logic_error("NanWindow::set_shell: shell is already installed");
        }

        // use_router() initially mounts the outlet as the window content. Detach
        // that temporary mount before the shell factory places the same stable
        // outlet inside the shell tree.
        overlay_host_->clear_content();
        shell_scope_ = std::make_unique<reactive::ReactiveScope>(app_.graph());
        ShellContext context {
            app_.graph(),
            *shell_scope_,
            app_.theme_manager(),
            &app_.resources(),
            overlay_host_.get(),
            &drag_controller_,
            router_->navigation(),
            router_->outlet(),
            router_->routes(),
            router_->current_page(),
        };
        auto root = std::invoke(factory, context);
        if (!root) {
            shell_scope_.reset();
            throw std::runtime_error("NanWindow::set_shell: factory returned null root");
        }
        if (root->as_control() == nullptr) {
            shell_scope_.reset();
            throw std::invalid_argument("NanWindow::set_shell: root must be a NanControl");
        }
        if (router_->outlet()->parent() == nullptr) {
            shell_scope_.reset();
            throw std::logic_error("NanWindow::set_shell: shell does not contain RouterOutlet");
        }
        set_content(std::move(root));
        shell_installed_ = true;
    }

    auto NanWindow::graph() -> reactive::Graph& {
        return app_.graph();
    }

    auto NanWindow::theme() const -> const theme::NanTheme& {
        return app_.theme();
    }

    auto NanWindow::default_text_pipeline() const -> const text::TextPipeline* {
        return default_text_pipeline_ ? &*default_text_pipeline_ : nullptr;
    }

    void NanWindow::open() {
        if (opened_) {
            return;
        }

        // 拖拽服务在任何内容 build 之前安装（install 只需 tree_ 与 overlay host 指针）。
        drag_controller_.install(tree_, overlay_host_.get());

        if (platform_ == nullptr) {
            platform_ = std::make_unique<detail::RaylibPlatformWindow>();
        }
        if (!platform_->create(config_)) {
            log::get("app.window")
                .error(
                    "NanWindow: platform \"{}\" failed to initialize native window {}x{} \"{}\"",
                    platform_->name(),
                    config_.width,
                    config_.height,
                    config_.title
                );
            // 平台没给出可用窗口时不要进入清理路径：headless 启动失败可能根本没有 GL 上下文。
            throw std::runtime_error("NanWindow: failed to initialize native window");
        }
        // From this point on the native window owns a live raylib/GL context. Mark
        // it opened before constructing framework resources so an exception during
        // device/font setup can be recovered by the destructor's normal close path.
        opened_ = true;
        clipboard_ = std::make_unique<PlatformClipboard>(*platform_);
        tree_.set_clipboard(*clipboard_);

        device_ = platform_->make_render_device();
        if (device_ != nullptr && device_wrapper_) {
            // trace / 其它观察者：包装后窗口用返回的设备，内层仍是平台设备。
            device_ = device_wrapper_(std::move(device_));
        }
        if (device_ == nullptr) {
            platform_->destroy();
            opened_ = false;
            throw std::runtime_error("NanWindow: platform failed to create a render device");
        }
        texture_cache_ = std::make_unique<render::TextureCache>(
            *device_,
            render::TextureCacheLimits {},
            render::TextureCacheAsyncServices {
                .decoder = app_.image_decoder(),
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
        const auto pipeline = font_pipeline_cache_->get({});
        if (!pipeline) {
            const auto reason = pipeline.error().message;
            // 错误路径的释放顺序与 close() 一致，且有一条硬约束：`CloseWindow()`
            // 必须在 render device 销毁**之前**调用 —— 它内部会走 raylib 的
            // `rlglClose()`，若此时 GL 上下文已被 device 析构带走，会在
            // `rlUnloadRenderBatch` 解引用空指针（实测无 RTTI 配置下 SIGSEGV）。
            tree_.set_root(nullptr);
            tree_.clear_default_text_pipeline();
            tree_.clear_font_context();
            default_font_pipeline_.reset();
            font_pipeline_cache_.reset();
            tree_.clear_texture_cache();
            texture_cache_.reset();
            platform_->destroy();
            device_.reset();
            tree_.clear_clipboard();
            opened_ = false;
            close_pending_ = false;
            log::get("app.window")
                .error("NanWindow: cannot create default text pipeline: {}", reason);
            throw std::runtime_error("NanWindow: cannot create default text pipeline: " + reason);
        }
        default_font_pipeline_ = *pipeline;
        default_text_pipeline_ = default_font_pipeline_->pipeline();

        log::get("app.window")
            .info("NanWindow: opened {}x{} \"{}\"", config_.width, config_.height, config_.title);
        on_setup();
    }

    void NanWindow::set_render_device_wrapper(
        std::move_only_function<
            std::unique_ptr<render::IRenderDevice>(std::unique_ptr<render::IRenderDevice>)> wrapper
    ) {
        if (opened_) {
            throw std::logic_error("NanWindow::set_render_device_wrapper: window already opened");
        }
        device_wrapper_ = std::move(wrapper);
    }

    void NanWindow::set_platform_window(std::unique_ptr<IPlatformWindow> platform) {
        if (opened_) {
            throw std::logic_error("NanWindow::set_platform_window: window already opened");
        }
        platform_ = platform != nullptr ? std::move(platform)
                                        : std::make_unique<detail::RaylibPlatformWindow>();
    }

    auto NanWindow::platform_name() const -> std::string_view {
        return platform_ != nullptr ? platform_->name() : std::string_view {"none"};
    }

    auto NanWindow::platform_backend() const -> WindowBackend {
        const auto resolved = config_.backend == WindowBackend::automatic ? default_window_backend()
                                                                          : config_.backend;
        return resolved == WindowBackend::automatic ? WindowBackend::raylib : resolved;
    }

    auto NanWindow::should_close() const -> bool {
        return close_pending_ || (platform_ != nullptr && platform_->should_close());
    }

    auto NanWindow::close_requested() const noexcept -> bool {
        return close_pending_;
    }

    void NanWindow::request_close() {
        close_pending_ = true;
    }

    void NanWindow::poll_and_dispatch_input() {
        // 宿主只负责「派发」：平台把这一帧的输入翻译成 scene 事件（见 platform_window.hpp）。
        // 这里按类型分发，派发顺序与搬迁前一致：move → buttons → wheel → keys → text。
        frame_input_.mouse_moves.clear();
        frame_input_.mouse_buttons.clear();
        frame_input_.mouse_wheels.clear();
        frame_input_.keys.clear();
        frame_input_.text_inputs.clear();
        frame_input_.has_pointer = false;
        platform_->poll_input(frame_input_);

        const auto to_logical = [this](const foundation::NanPoint screen_pos) {
            return viewport_mapping_ ? viewport_mapping_->screen_to_logical(screen_pos)
                                     : screen_pos;
        };
        const auto pos = to_logical(frame_input_.pointer);

        for (const auto& screen_ev: frame_input_.mouse_moves) {
            const auto logical = to_logical(screen_ev.screen_pos());
            const auto previous_screen = foundation::NanPoint {
                screen_ev.screen_pos().get_x() - screen_ev.delta().get_x(),
                screen_ev.screen_pos().get_y() - screen_ev.delta().get_y(),
            };
            const auto previous = to_logical(previous_screen);
            scene::MouseMoveEvent ev {
                logical,
                foundation::NanPoint {
                    logical.get_x() - previous.get_x(),
                    logical.get_y() - previous.get_y(),
                }
            };
            tree_.dispatch_mouse_move(ev);
        }

        for (const auto& screen_ev: frame_input_.mouse_buttons) {
            scene::MouseButtonEvent ev {
                screen_ev.button(),
                screen_ev.action(),
                pos,
                screen_ev.modifiers(),
            };
            tree_.dispatch_mouse_button(ev);
        }

        for (const auto& screen_ev: frame_input_.mouse_wheels) {
            scene::MouseWheelEvent ev {pos, screen_ev.delta(), screen_ev.modifiers()};
            tree_.dispatch_mouse_wheel(ev);
        }

        for (const auto& screen_ev: frame_input_.keys) {
            scene::KeyEvent ev {
                screen_ev.keycode(),
                screen_ev.action(),
                screen_ev.modifiers(),
            };
            tree_.dispatch_key(ev);
        }

        for (const auto& screen_ev: frame_input_.text_inputs) {
            scene::TextInputEvent ev {std::string(screen_ev.text())};
            tree_.dispatch_text_input(ev);
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
        // 关窗之后不再推进任何阶段：close() 会销毁设备与平台窗口，继续 tick 会读到 0×0
        // 尺寸并访问已释放的资源。宿主的主循环通常靠 should_close() 停下，但这一层也要
        // 自己守住（SDL 平台的 should_close() 已按同一语义返回 true，见其实现）。
        if (!opened_) {
            return;
        }
        const float dt = platform_->frame_time();
        const auto metrics =
            make_window_metrics(platform_->screen_size(), platform_->drawable_size());
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
        // 平台呈现：raylib 平台的 EndDrawing 在设备里（no-op），SDL 平台在这里 present。
        platform_->present();

        {
            auto phase = tree_.enter_phase(scene::FramePhase::dispose);
            tree_.flush_tree_mutations();
            tree_.flush_deferred_deletes();
        }

        // 只有整帧（含绘制与帧末提交）结束后才真正关窗。close() 会立即销毁
        // render device，放在 on_frame() 里调用会让本帧后续的 device_ 使用变成
        // 空指针解引用。
        if (close_pending_) {
            close();
        }
    }

    void NanWindow::close() {
        if (!opened_) {
            return;
        }
        // 释放场景树 (触发 widget 卸载, 回访 graph) 后再释放设备、关窗口。
        //
        // 注意：FontPipeline 析构会对 atlas 纹理调用 device.destroy_texture()，
        // 因此所有持有 FontPipeline 的对象必须在此之前析构。当前实现下仍有
        // 个别控件 Text 晚于 device_ 释放，导致关闭后 ~GlyphAtlasTexture 解引用
        // 已销毁的 device（已知缺陷，见 docs/references/design_tokens.md）。
        on_teardown();
        drag_controller_.cancel();
        if (router_) {
            router_->clear();
        }
        // 关键：应用内容由 overlay_host_ 的内容层持有，而 overlay_host_ 是窗口成员。
        // 若不在这里主动释放，内容树会活到窗口析构，届时 device_ 已销毁，控件里的
        // 文本资源（FontPipeline → GlyphAtlasTexture）会在析构时对已销毁的 device
        // 调用 destroy_texture()，导致关闭后 SIGSEGV。先清内容层，再清场景树。
        overlay_host_->clear_content();
        overlay_host_->clear_overlays();
        tree_.set_root(nullptr);
        shell_scope_.reset();
        shell_installed_ = false;
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
        clipboard_.reset();
        platform_->destroy();
        opened_ = false;
        close_pending_ = false;
        log::get("app.window").info("NanWindow: closed");
    }

} // namespace nandina::app
