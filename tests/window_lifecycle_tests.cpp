//
// window_lifecycle_tests — 窗口关闭路径的约束。
//
// 背景：在 `on_frame()` 里直接调用 `close()` 会立刻销毁 render device 与原生窗口，
// 而 `tick()` 在 `on_frame()` 返回后仍要 `device_->begin_frame()` / `clear()` / 绘制，
// 于是必然解引用空指针（实测 SIGSEGV 于 nan_window.cpp 的 device_->clear）。
// 因此新增 `request_close()`：只置标志，`tick()` 在整帧（含绘制与帧末提交）结束后
// 才真正 `close()`。
//
// 本文件用一个真实窗口跑若干帧，验证：
//   * 在 on_frame 里 request_close() 能干净退出（退出码 0，不 SIGSEGV）；
//   * 关闭发生在帧末——关闭前窗口仍完成了若干帧绘制。
//

#include <nandina/app/nan_application.hpp>
#include <nandina/app/nan_window.hpp>
#include <nandina/foundation/nandina_color_space.hpp>
#include <nandina/render/draw_context.hpp>
#include <nandina/theme/builtin_themes.hpp>
#include <nandina/widget/label.hpp>
#include <nandina/widget/layout.hpp>

#if defined(NANDINA_HAS_SDL_BACKEND)
    #include <nandina/app/detail/sdl_platform_window.hpp>

    #include <SDL3/SDL.h>
#endif

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <stdexcept>
#include <string_view>

using namespace nandina;

namespace
{
    /// 在第 `close_on_frame` 帧的 on_frame 里请求关闭。
    class ClosingWindow final: public app::NanWindow {
    public:
        ClosingWindow(
            app::NanApplication& application,
            app::WindowConfig config,
            int close_on_frame
        ):
            app::NanWindow(application, std::move(config)),
            close_on_frame_(close_on_frame) {}

        [[nodiscard]] auto frames() const noexcept -> int {
            return frames_;
        }

        [[nodiscard]] auto close_requested_at_frame() const noexcept -> int {
            return requested_at_;
        }

    protected:
        void on_setup() override {
            auto column = widget::Column::create();
            column->add(widget::Label::create(graph(), "lifecycle", theme::default_theme()));
            set_content(column);
        }

        void on_frame(float /*dt*/) override {
            ++frames_;
            if (frames_ >= close_on_frame_ && requested_at_ < 0) {
                requested_at_ = frames_;
                // 这里必须是 request_close()，不能是 close()。
                request_close();
            }
        }

    private:
        int close_on_frame_ = 3;
        int frames_ = 0;
        int requested_at_ = -1;
    };
} // namespace

TEST_CASE("request_close from on_frame shuts the window down cleanly", "[app][window][lifecycle]") {
#ifdef NANDINA_SKIP_WINDOW_TESTS
    SKIP("window tests are disabled because no native display is available");
#else
    #ifdef NANDINA_OPTIONAL_WINDOW_TESTS
    try {
    #endif
        app::NanApplication application(
            app::NanApplicationConfig::for_process("com.nandina.lifecycle_test")
        );
        theme::register_default_theme_families(application.theme_manager());

        ClosingWindow window {
            application,
            app::WindowConfig {
                .title = "lifecycle test",
                .width = 320,
                .height = 240,
                .backend = app::WindowBackend::sdl3,
            },
            3,
        };

        REQUIRE(window.platform_name() == "sdl3");
        const auto exit_code = application.run(window);

        REQUIRE(exit_code == 0);
        // 关闭请求在第 3 帧发出，且之后不再有帧。
        REQUIRE(window.close_requested_at_frame() == 3);
        REQUIRE(window.frames() == 3);
    #ifdef NANDINA_OPTIONAL_WINDOW_TESTS
    }
    catch (const std::runtime_error& error) {
        // Meson auto 模式只能根据环境变量判断显示环境是否可能存在；容器或
        // sandbox 中 socket 可能存在但不可连接，此时仍应把窗口测试视为不可用。
        if (std::string_view(error.what()) == "NanWindow: failed to initialize native window") {
            SKIP("window tests are unavailable because the native display cannot be opened");
        }
        else {
            throw;
        }
    }
    #endif
#endif
}

#if defined(NANDINA_HAS_SDL_BACKEND)
namespace
{
    class FixedBadgeBox final: public scene::NanControl {
    public:
        explicit FixedBadgeBox(foundation::NanColor fill): fill_(fill) {
            set_width(80.0F);
            set_height(40.0F);
        }

        void on_draw(render::DrawContext& ctx) override {
            const auto world = render::world_bounds_from_local(ctx.world_transform(), local_rect());
            ctx.device().draw_rect(world, fill_);
        }

    private:
        foundation::NanColor fill_;
    };

    class PrePresentReadbackDevice final: public render::IRenderDevice {
    public:
        PrePresentReadbackDevice(
            std::unique_ptr<render::IRenderDevice> inner,
            std::move_only_function<void()> on_end_frame
        ):
            inner_(std::move(inner)),
            on_end_frame_(std::move(on_end_frame)) {}

        void begin_frame() override {
            inner_->begin_frame();
        }
        void end_frame() override {
            inner_->end_frame();
            if (on_end_frame_) {
                on_end_frame_();
            }
        }
        void clear(const foundation::NanColor& color) override {
            inner_->clear(color);
        }
        void set_clip(const foundation::NanRect& screen_rect) override {
            inner_->set_clip(screen_rect);
        }
        void clear_clip() override {
            inner_->clear_clip();
        }
        void
        draw_rect(const foundation::NanRect& rect, const foundation::NanColor& color) override {
            inner_->draw_rect(rect, color);
        }
        void draw_rect_outline(
            const foundation::NanRect& rect,
            float thickness,
            const foundation::NanColor& color
        ) override {
            inner_->draw_rect_outline(rect, thickness, color);
        }
        void draw_rounded_rect(
            const foundation::NanRect& rect,
            float radius,
            const foundation::NanColor& color
        ) override {
            inner_->draw_rounded_rect(rect, radius, color);
        }
        void draw_rounded_rect_outline(
            const foundation::NanRect& rect,
            float radius,
            float thickness,
            const foundation::NanColor& color
        ) override {
            inner_->draw_rounded_rect_outline(rect, radius, thickness, color);
        }
        void draw_line(
            const foundation::NanPoint& a,
            const foundation::NanPoint& b,
            float thickness,
            const foundation::NanColor& color
        ) override {
            inner_->draw_line(a, b, thickness, color);
        }
        void draw_circle(
            const foundation::NanPoint& center,
            float radius,
            const foundation::NanColor& color
        ) override {
            inner_->draw_circle(center, radius, color);
        }
        void draw_circle_clipped_rounded_rect(
            const foundation::NanPoint& center,
            float circle_radius,
            const foundation::NanRect& clip_rect,
            float clip_radius,
            const foundation::NanColor& color
        ) override {
            inner_->draw_circle_clipped_rounded_rect(
                center,
                circle_radius,
                clip_rect,
                clip_radius,
                color
            );
        }
        void draw_arc(
            const foundation::NanPoint& center,
            float inner_radius,
            float outer_radius,
            float start_radians,
            float sweep_radians,
            const foundation::NanColor& color
        ) override {
            inner_
                ->draw_arc(center, inner_radius, outer_radius, start_radians, sweep_radians, color);
        }
        void draw_rounded_rect_shadow(
            const foundation::NanRect& rect,
            float radius,
            float spread,
            const foundation::NanColor& color
        ) override {
            inner_->draw_rounded_rect_shadow(rect, radius, spread, color);
        }
        void draw_text(
            std::string_view text,
            const foundation::NanPoint& pos,
            float font_size,
            const foundation::NanColor& color
        ) override {
            inner_->draw_text(text, pos, font_size, color);
        }
        [[nodiscard]] auto supports_alpha_textures() const -> bool override {
            return inner_->supports_alpha_textures();
        }
        [[nodiscard]] auto
        create_alpha_texture(int width, int height, std::span<const std::uint8_t> alpha)
            -> render::TextureHandle override {
            return inner_->create_alpha_texture(width, height, alpha);
        }
        void update_alpha_texture(
            render::TextureHandle texture,
            int width,
            int height,
            std::span<const std::uint8_t> alpha
        ) override {
            inner_->update_alpha_texture(texture, width, height, alpha);
        }
        void update_alpha_texture_region(
            render::TextureHandle texture,
            int full_width,
            int full_height,
            const foundation::NanRect& region,
            std::span<const std::uint8_t> alpha
        ) override {
            inner_->update_alpha_texture_region(texture, full_width, full_height, region, alpha);
        }
        void destroy_texture(render::TextureHandle texture) override {
            inner_->destroy_texture(texture);
        }
        void draw_texture_region(
            render::TextureHandle texture,
            const foundation::NanRect& source,
            const foundation::NanRect& destination,
            const foundation::NanColor& tint
        ) override {
            inner_->draw_texture_region(texture, source, destination, tint);
        }
        [[nodiscard]] auto texture_size(render::TextureHandle texture)
            -> foundation::NanSize override {
            return inner_->texture_size(texture);
        }
        [[nodiscard]] auto supports_rounded_rect() const -> bool override {
            return inner_->supports_rounded_rect();
        }
        [[nodiscard]] auto supports_rounded_clip() const -> bool override {
            return inner_->supports_rounded_clip();
        }
        [[nodiscard]] auto supports_direct_image_loading() const -> bool override {
            return inner_->supports_direct_image_loading();
        }
        [[nodiscard]] auto supports_rgba_texture_upload() const -> bool override {
            return inner_->supports_rgba_texture_upload();
        }
        [[nodiscard]] auto
        create_rgba_texture(int width, int height, std::span<const std::uint8_t> rgba)
            -> render::TextureHandle override {
            return inner_->create_rgba_texture(width, height, rgba);
        }
        [[nodiscard]] auto supports_soft_shadows() const -> bool override {
            return inner_->supports_soft_shadows();
        }
        [[nodiscard]] auto analytic_antialiasing() const -> bool override {
            return inner_->analytic_antialiasing();
        }

    private:
        std::unique_ptr<render::IRenderDevice> inner_;
        std::move_only_function<void()> on_end_frame_;
    };
} // namespace

TEST_CASE(
    "SDL window resize updates the GPU frame target before rendering the resized frame",
    "[app][window][resize-target]"
) {
    #ifdef NANDINA_SKIP_WINDOW_TESTS
    SKIP("window tests are disabled because no native display is available");
    #else
        #ifdef NANDINA_OPTIONAL_WINDOW_TESTS
    try {
        #endif
        app::NanApplication application(
            app::NanApplicationConfig::for_process("com.nandina.resize_target_test")
        );
        const auto bg_color = foundation::NanColor::from(
            foundation::NanHexRgb {.red = 20, .green = 30, .blue = 40, .alpha = 255}
        );
        const auto box_color = foundation::NanColor::from(
            foundation::NanHexRgb {.red = 220, .green = 60, .blue = 40, .alpha = 255}
        );

        app::NanWindow window {
            application,
            app::WindowConfig {
                .title = "resize target test",
                .width = 320,
                .height = 240,
                .target_fps = 0,
                .vsync = false,
                .high_dpi = false,
                .background = bg_color,
                .backend = app::WindowBackend::sdl3,
            },
        };

        auto platform = std::make_unique<app::detail::SdlPlatformWindow>();
        auto* raw_platform = platform.get();
        window.set_platform_window(std::move(platform));

        struct Snapshot {
            int target_w = 0;
            int target_h = 0;
            int window_pixel_w = 0;
            int window_pixel_h = 0;
            Uint8 inside_r = 0;
            Uint8 outside_r = 0;
        };
        std::vector<Snapshot> snapshots;

        window.set_render_device_wrapper([&](std::unique_ptr<render::IRenderDevice> inner) {
            return std::make_unique<PrePresentReadbackDevice>(std::move(inner), [&]() {
                SDL_Window* sdl_win = raw_platform->native_window();
                SDL_Renderer* renderer = raw_platform->native_renderer();
                if (sdl_win == nullptr || renderer == nullptr) {
                    return;
                }

                int pixel_w = 0;
                int pixel_h = 0;
                (void)SDL_GetWindowSizeInPixels(sdl_win, &pixel_w, &pixel_h);
                Snapshot snap {
                    .window_pixel_w = pixel_w,
                    .window_pixel_h = pixel_h,
                };
                if (SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr);
                    surface != nullptr)
                {
                    snap.target_w = surface->w;
                    snap.target_h = surface->h;
                    if (surface->w > 95 && surface->h > 25) {
                        (void)SDL_ReadSurfacePixel(
                            surface,
                            40,
                            20,
                            &snap.inside_r,
                            nullptr,
                            nullptr,
                            nullptr
                        );
                        (void)SDL_ReadSurfacePixel(
                            surface,
                            90,
                            20,
                            &snap.outside_r,
                            nullptr,
                            nullptr,
                            nullptr
                        );
                    }
                    SDL_DestroySurface(surface);
                }
                snapshots.push_back(snap);
            });
        });

        window.open();
        auto root = widget::Column::create();
        root->set_cross_alignment(widget::LayoutAlignment::start);
        root->add(std::make_shared<FixedBadgeBox>(box_color));
        window.set_content(root);

        window.tick();
        REQUIRE(snapshots.size() == 1);
        CHECK(snapshots[0].target_w == snapshots[0].window_pixel_w);
        CHECK(snapshots[0].target_h == snapshots[0].window_pixel_h);
        CHECK(snapshots[0].inside_r > 180);
        CHECK(snapshots[0].outside_r < 60);

        SDL_Window* sdl_win = raw_platform->native_window();
        REQUIRE(sdl_win != nullptr);
        (void)SDL_SetWindowSize(sdl_win, 440, 300);
        (void)SDL_SyncWindow(sdl_win);
        window.tick();

        REQUIRE(snapshots.size() == 2);
        REQUIRE(snapshots[1].window_pixel_w > 0);
        REQUIRE(snapshots[1].window_pixel_h > 0);
        CHECK(snapshots[1].target_w == snapshots[1].window_pixel_w);
        CHECK(snapshots[1].target_h == snapshots[1].window_pixel_h);
        CHECK(snapshots[1].inside_r > 180);
        CHECK(snapshots[1].outside_r < 60);

        root.reset();
        window.close();
        #ifdef NANDINA_OPTIONAL_WINDOW_TESTS
    }
    catch (const std::runtime_error& error) {
        if (std::string_view(error.what()) == "NanWindow: failed to initialize native window") {
            SKIP("window tests are unavailable because the native display cannot be opened");
        }
        else {
            throw;
        }
    }
        #endif
    #endif
}
#endif
