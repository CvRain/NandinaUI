//
// tests/sdl_visual_tests — SDL 设备的确定性像素回归（基础正确性层）。
//
// 为什么用**软件渲染器**：`SDL_CreateSoftwareRenderer(surface)` 不需要窗口、不需要显示器、也不需要
// GPU 驱动，像素结果只由绘制调用与合成本身决定。于是"半透明应该合成成什么颜色"可以写成**解析预期**
// （不取自任何后端的当前输出、也不需要 golden），跨机器稳定 —— 正是
// docs/references/visual_regression.md 的"基础正确性层"：先把"对不对"钉住，页面级 golden 另属一层。
//
// 覆盖 D7 混合缺陷的三个可见症状：按钮按下态（状态层）、涟漪中间态、对话框遮罩。
// 断言不是"和上次一样"，而是"等于已知颜色按已知 alpha 合成的结果"：设备若没有启用几何混合，
// 半透明层会被画成不透明，下面每一条都会红。
//
// 目标只在找到 SDL3 时注册（见 tests/meson.build）。
//

#include <nandina/foundation/nandina_color_space.hpp>
#include <nandina/reactive/graph.hpp>
#include <nandina/render/backends/sdl_device.hpp>
#include <nandina/scene/overlay_host.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/design_system.hpp>
#include <nandina/theme/theme_manager.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/dialog.hpp>

#include <SDL3/SDL.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
    using namespace nandina;

    /// 8 位直线 alpha 合成：`out = src*a + dst*(1-a)`。
    ///
    /// 与 SDL 软件渲染器一致地按 8 位分量计算（设备也是先经 `to<NanHexRgb>()` 降到 8 位再交给 SDL），
    /// 所以能用解析预期而不是"看着差不多"；整数舍入可能差一级，断言统一给容差。
    struct Rgba {
        int r = 0;
        int g = 0;
        int b = 0;
        int a = 255;
    };

    [[nodiscard]] auto over(const Rgba dst, const Rgba src) -> Rgba {
        const auto alpha = src.a;
        const auto inverse = 255 - alpha;
        const auto mix = [alpha, inverse](const int s, const int d) {
            return (s * alpha + d * inverse + 127) / 255;
        };
        return Rgba {
            .r = mix(src.r, dst.r),
            .g = mix(src.g, dst.g),
            .b = mix(src.b, dst.b),
            .a = std::min(255, alpha + (dst.a * inverse + 127) / 255),
        };
    }

    [[nodiscard]] auto to_rgba(const foundation::NanColor& color) -> Rgba {
        const auto rgb = color.to<foundation::NanHexRgb>();
        return Rgba {.r = rgb.red, .g = rgb.green, .b = rgb.blue, .a = rgb.alpha};
    }

    [[nodiscard]] auto from_hex(const std::uint32_t packed) -> Rgba {
        return Rgba {
            .r = static_cast<int>((packed >> 24U) & 0xFFU),
            .g = static_cast<int>((packed >> 16U) & 0xFFU),
            .b = static_cast<int>((packed >> 8U) & 0xFFU),
            .a = static_cast<int>(packed & 0xFFU),
        };
    }

    [[nodiscard]] auto color_from_hex(const std::uint32_t packed) -> foundation::NanColor {
        return foundation::NanColor::from(foundation::NanHexRgb::from_uint32(packed));
    }

    /// 内存画布 + 软件渲染器：不依赖窗口、显示器与 GPU 驱动。
    class SoftwareCanvas {
    public:
        SoftwareCanvas(const int width, const int height):
            width_(width),
            height_(height),
            surface_(SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32)) {
            if (surface_ == nullptr) {
                throw std::runtime_error(std::string("SDL_CreateSurface: ") + SDL_GetError());
            }
            renderer_ = SDL_CreateSoftwareRenderer(surface_);
            if (renderer_ == nullptr) {
                SDL_DestroySurface(surface_);
                surface_ = nullptr;
                throw std::runtime_error(
                    std::string("SDL_CreateSoftwareRenderer: ") + SDL_GetError()
                );
            }
        }

        ~SoftwareCanvas() {
            if (renderer_ != nullptr) {
                SDL_DestroyRenderer(renderer_);
            }
            if (surface_ != nullptr) {
                SDL_DestroySurface(surface_);
            }
        }

        SoftwareCanvas(const SoftwareCanvas&) = delete;
        auto operator=(const SoftwareCanvas&) -> SoftwareCanvas& = delete;
        SoftwareCanvas(SoftwareCanvas&&) = delete;
        auto operator=(SoftwareCanvas&&) -> SoftwareCanvas& = delete;

        [[nodiscard]] auto renderer() -> SDL_Renderer& {
            return *renderer_;
        }
        [[nodiscard]] auto width() const -> int {
            return width_;
        }
        [[nodiscard]] auto height() const -> int {
            return height_;
        }

        /// 直接读渲染目标：软件渲染器没有 present，读回即当前内容。
        ///
        /// SDL3 的 `SDL_RenderReadPixels` 返回一张新 Surface（不是 SDL2 那种写入调用方缓冲），
        /// 再由 `SDL_ReadSurfacePixel` 取单个像素；用完必须释放 surface。
        [[nodiscard]] auto pixel(const int x, const int y) const -> Rgba {
            const SDL_Rect region {.x = x, .y = y, .w = 1, .h = 1};
            SDL_Surface* shot = SDL_RenderReadPixels(renderer_, &region);
            if (shot == nullptr) {
                throw std::runtime_error(std::string("SDL_RenderReadPixels: ") + SDL_GetError());
            }
            std::uint8_t r = 0;
            std::uint8_t g = 0;
            std::uint8_t b = 0;
            std::uint8_t a = 0;
            const bool ok = SDL_ReadSurfacePixel(shot, 0, 0, &r, &g, &b, &a);
            SDL_DestroySurface(shot);
            if (!ok) {
                throw std::runtime_error(std::string("SDL_ReadSurfacePixel: ") + SDL_GetError());
            }
            return Rgba {
                .r = static_cast<int>(r),
                .g = static_cast<int>(g),
                .b = static_cast<int>(b),
                .a = static_cast<int>(a),
            };
        }

    private:
        int width_ = 0;
        int height_ = 0;
        SDL_Surface* surface_ = nullptr;
        SDL_Renderer* renderer_ = nullptr;
    };

    /// 8 位合成的容差：整数舍入与 SDL 内部处理顺序最多差一两级。
    constexpr int kChannelTolerance = 2;

    void require_pixel(
        const Rgba actual,
        const Rgba expected,
        const std::string& what,
        const int x,
        const int y
    ) {
        INFO(what << " at (" << x << ", " << y << ")");
        REQUIRE(actual.r == Catch::Approx(expected.r).margin(kChannelTolerance));
        REQUIRE(actual.g == Catch::Approx(expected.g).margin(kChannelTolerance));
        REQUIRE(actual.b == Catch::Approx(expected.b).margin(kChannelTolerance));
        REQUIRE(actual.a == Catch::Approx(expected.a).margin(kChannelTolerance));
    }

    [[nodiscard]] auto
    channels_differ(const Rgba a, const Rgba b, const int margin = kChannelTolerance) -> bool {
        return std::abs(a.r - b.r) > margin || std::abs(a.g - b.g) > margin
            || std::abs(a.b - b.b) > margin || std::abs(a.a - b.a) > margin;
    }

    /// 测试自绘的背景色：不依赖宿主 clear 色，也不依赖主题。
    constexpr std::uint32_t kBackground = 0x202833FFU;
    constexpr int kCanvasWidth = 240;
    constexpr int kCanvasHeight = 140;

    /// 一个已知位置与尺寸的按钮。不设文本：避免把调试字体的像素混进中心采样点。
    struct ButtonScene {
        std::shared_ptr<scene::NanControl> root;
        std::shared_ptr<widget::Button> button;
        scene::NanSceneTree tree;

        ButtonScene() {
            root = std::make_shared<scene::NanControl>(foundation::NanSize(
                static_cast<float>(kCanvasWidth),
                static_cast<float>(kCanvasHeight)
            ));
            button = std::make_shared<widget::Button>("");
            // 显式选 filled：它的 pressed 状态层是 on_accent@0.20 的半透明层，
            // 正是"白底白字"那条缺陷的触发条件，不依赖默认配方是否恰好也这样。
            button->set_treatment(theme::ButtonTreatment::filled);
            button->set_position(foundation::NanPoint(40.0F, 40.0F));
            button->set_size(foundation::NanSize(160.0F, 60.0F));
            root->add_child(button);
            tree.set_root(root);
            (void)tree.layout_root(
                foundation::NanSize(
                    static_cast<float>(kCanvasWidth),
                    static_cast<float>(kCanvasHeight)
                )
            );
        }

        [[nodiscard]] auto centre() const -> foundation::NanPoint {
            const auto bounds = button->global_bounds();
            return foundation::NanPoint(
                (bounds.get_left() + bounds.get_right()) * 0.5F,
                (bounds.get_top() + bounds.get_bottom()) * 0.5F
            );
        }

        void press_centre() {
            const auto point = centre();
            tree.dispatch_mouse_move(scene::MouseMoveEvent {point, foundation::NanPoint::zero()});
            tree.dispatch_mouse_button(
                scene::MouseButtonEvent {
                    scene::MouseButtonEvent::Button::left,
                    scene::MouseButtonEvent::Action::press,
                    point,
                }
            );
        }
    };
} // namespace

TEST_CASE("translucent geometry composites over a known background", "[visual][blend][sdl]") {
    // 基础正确性层：不经过任何控件，只验证设备对半透明几何的合成本身。
    SoftwareCanvas canvas {200, 120};
    render::SdlRenderDevice device {canvas.renderer()};

    const auto background = from_hex(kBackground);
    device.clear(color_from_hex(kBackground));

    const auto translucent = from_hex(0xC8783C80U); // 200,120,60, alpha 128
    device.draw_rounded_rect(
        foundation::NanRect(40.0F, 30.0F, 160.0F, 90.0F),
        12.0F,
        color_from_hex(0xC8783C80U)
    );

    require_pixel(
        canvas.pixel(100, 60),
        over(background, translucent),
        "composited centre",
        100,
        60
    );
    // 反向保证：结果不是"半透明色被原样画成不透明"。
    const auto opaque = Rgba {.r = translucent.r, .g = translucent.g, .b = translucent.b, .a = 255};
    REQUIRE(channels_differ(canvas.pixel(100, 60), opaque));
    // 控件之外必须仍是背景本身。
    require_pixel(canvas.pixel(5, 5), background, "untouched background", 5, 5);

    // 四角扇形内部，远离圆弧边界；中心采样无法发现几何扇形漏画或 alpha 丢失。
    for (const auto& point:
         std::array<std::array<int, 2>, 4> {{{46, 36}, {153, 36}, {46, 83}, {153, 83}}})
    {
        require_pixel(
            canvas.pixel(point[0], point[1]),
            over(background, translucent),
            "corner fan interior",
            point[0],
            point[1]
        );
    }
    // 跨过 x=52/148、y=42/78 的拼接线：漏缝或重复混合都不应出现。
    for (const int x: {51, 52, 147, 148}) {
        for (const int y: {35, 84}) {
            require_pixel(canvas.pixel(x, y), over(background, translucent), "vertical join", x, y);
        }
    }
    for (const int x: {45, 154}) {
        for (const int y: {41, 42, 77, 78}) {
            require_pixel(
                canvas.pixel(x, y),
                over(background, translucent),
                "horizontal join",
                x,
                y
            );
        }
    }
    for (const auto& point:
         std::array<std::array<int, 2>, 4> {{{41, 31}, {158, 31}, {41, 88}, {158, 88}}})
    {
        require_pixel(
            canvas.pixel(point[0], point[1]),
            background,
            "outside rounded corner",
            point[0],
            point[1]
        );
    }
}

TEST_CASE(
    "pressed button paints its state layer as a translucent overlay",
    "[visual][blend][sdl]"
) {
    SoftwareCanvas canvas {kCanvasWidth, kCanvasHeight};
    render::SdlRenderDevice device {canvas.renderer()};
    const auto background = from_hex(kBackground);
    device.clear(color_from_hex(kBackground));

    ButtonScene scene;
    scene.press_centre();
    REQUIRE(scene.button->pressed());

    // 涟漪在 progress=0 时不画（painter 直接返回），所以这一帧只有容器 + 状态层。
    scene.tree.draw(device);

    const auto style = scene.button->resolved_style();
    const auto fill = to_rgba(style.container.fill);
    const auto state =
        to_rgba(theme::button_state_layer_color(style, theme::ButtonVisualState::pressed));
    INFO("fill alpha " << fill.a << ", state layer alpha " << state.a);
    REQUIRE(state.a > 0); // 前提：这一档的 pressed 状态层确实是半透明的
    REQUIRE(state.a < 255); // 否则本测试无法区分"混合"与"不透明覆盖"

    const auto point = scene.centre();
    const auto x = static_cast<int>(std::lround(point.get_x()));
    const auto y = static_cast<int>(std::lround(point.get_y()));
    const auto actual = canvas.pixel(x, y);
    require_pixel(actual, over(over(background, fill), state), "pressed centre", x, y);
    // "白底白字"的直接守卫：中心不得等于状态层被不透明画出的结果。
    const auto opaque_state = Rgba {.r = state.r, .g = state.g, .b = state.b, .a = 255};
    REQUIRE(channels_differ(actual, opaque_state));
}

TEST_CASE("ripple mid-state composites over the pressed button", "[visual][blend][sdl]") {
    SoftwareCanvas canvas {kCanvasWidth, kCanvasHeight};
    render::SdlRenderDevice device {canvas.renderer()};
    const auto background = from_hex(kBackground);

    ButtonScene scene;
    scene.press_centre();
    const auto duration = scene.button->resolved_style().ripple.duration;
    REQUIRE(duration > 0.01F);

    // 固定时间推进到涟漪中间态（不是首帧也不是末帧）。
    scene.tree.process(duration * 0.5F);
    const auto progress = scene.button->ripple_progress();
    REQUIRE(progress == Catch::Approx(0.5F).margin(1.0e-3F));

    device.clear_clip();
    device.clear(color_from_hex(kBackground));
    scene.tree.draw(device);

    const auto style = scene.button->resolved_style();
    const auto fill = to_rgba(style.container.fill);
    const auto state =
        to_rgba(theme::button_state_layer_color(style, theme::ButtonVisualState::pressed));
    const auto ripple_base = to_rgba(style.ripple.color);
    // painter 的 alpha = color.alpha * (1 - progress) * parent_opacity（这里 opacity=1）。
    const auto ripple = Rgba {
        .r = ripple_base.r,
        .g = ripple_base.g,
        .b = ripple_base.b,
        .a = static_cast<int>(std::lround(static_cast<float>(ripple_base.a) * (1.0F - progress))),
    };
    INFO("ripple alpha " << ripple.a << " at progress " << progress);
    REQUIRE(ripple.a > 0);

    const auto point = scene.centre();
    const auto x = static_cast<int>(std::lround(point.get_x()));
    const auto y = static_cast<int>(std::lround(point.get_y()));
    require_pixel(
        canvas.pixel(x, y),
        over(over(over(background, fill), state), ripple),
        "ripple mid-state centre",
        x,
        y
    );
}

TEST_CASE("dialog scrim composites over the background", "[visual][blend][sdl]") {
    SoftwareCanvas canvas {800, 600};
    render::SdlRenderDevice device {canvas.renderer()};
    reactive::Graph graph;
    theme::ThemeManager themes;
    const auto host = scene::OverlayHost::create();
    const auto body = std::make_shared<scene::NanControl>(foundation::NanSize(800.0F, 600.0F));
    const auto dialog = widget::Dialog::create();
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    host->set_content(body);
    tree.set_root(host);
    body->add_child(dialog);
    const auto layout = [&] { (void)tree.layout_root(foundation::NanSize(800.0F, 600.0F)); };
    const auto settle = [&] {
        for (int i = 0; i < 8; ++i) {
            dialog->on_process(0.1F);
            auto phase = tree.enter_phase(scene::FramePhase::animation);
            tree.advance_animations(0.1F);
        }
        layout();
    };
    const auto draw = [&] {
        device.clear_clip();
        device.clear(color_from_hex(kBackground));
        tree.draw(device);
    };
    const auto background = from_hex(kBackground);
    layout();
    REQUIRE(host->overlay_count() == 0);
    draw();
    require_pixel(canvas.pixel(10, 10), background, "before opening", 10, 10);

    dialog->open();
    layout();
    settle();
    // 收敛性自检：再推进一步并重绘，采样点像素必须不变。
    // 这样"动画还没结束"会以明确的失败暴露，而不是伪装成合成色不符。
    draw();
    const auto settled_pixel = canvas.pixel(10, 10);
    settle();
    draw();
    require_pixel(canvas.pixel(10, 10), settled_pixel, "scrim settled", 10, 10);
    REQUIRE(dialog->is_open());
    REQUIRE(host->overlay_count() == 1);
    // 与 dialog_tests 的 overlay 层级一致，明确确认采样点在实际面板之外。
    // 先确认层级存在：层级编号变化应当是清晰的断言失败，而不是空指针解引用。
    auto* overlay_layer = host->layer_at(1);
    REQUIRE(overlay_layer != nullptr);
    auto* surface = overlay_layer->layout_root();
    REQUIRE(surface != nullptr);
    REQUIRE(surface->child_count() == 1);
    auto* dismiss = surface->get_child(0)->as_control();
    REQUIRE(dismiss != nullptr);
    REQUIRE(dismiss->child_count() == 1);
    auto* scope = dismiss->get_child(0)->as_control();
    REQUIRE(scope != nullptr);
    REQUIRE(scope->child_count() == 1);
    auto* panel = scope->get_child(0)->as_control();
    REQUIRE(panel != nullptr);
    REQUIRE(panel->global_bounds().is_valid());
    REQUIRE_FALSE(panel->global_bounds().contains_point(foundation::NanPoint(10.0F, 10.0F)));
    const auto scrim = to_rgba(dialog->resolved_style().scrim);
    INFO("scrim alpha " << scrim.a);
    REQUIRE(scrim.a > 0);
    REQUIRE(scrim.a < 255); // 遮罩本来就该是半透明的

    draw();
    const auto expected = over(background, scrim);
    REQUIRE(channels_differ(expected, background));
    require_pixel(canvas.pixel(10, 10), expected, "opened scrim outside panel", 10, 10);
    // 遮罩不该把背景压成不透明的一块。
    REQUIRE(channels_differ(
        canvas.pixel(10, 10),
        Rgba {.r = scrim.r, .g = scrim.g, .b = scrim.b, .a = 255}
    ));
    dialog->close();
    settle();
    REQUIRE_FALSE(dialog->is_open());
    REQUIRE(host->overlay_count() == 0);
    draw();
    require_pixel(canvas.pixel(10, 10), background, "after closing", 10, 10);
}

TEST_CASE("rgba texture upload draws with blend and scale", "[visual][texture][sdl]") {
    // 服务路径的最后一个环节：应用（或默认 CPU 解码器）解码出的 RGBA8 像素
    // 经 create_rgba_texture 上传，再由 draw_texture_region 绘制。
    SoftwareCanvas canvas {64, 64};
    render::SdlRenderDevice device {canvas.renderer()};
    device.clear(color_from_hex(kBackground));

    REQUIRE(device.supports_rgba_texture_upload());
    REQUIRE_FALSE(device.supports_direct_image_loading());

    // 2x2：红、绿、蓝、白（不透明），放大到 16x16 后每个源像素占 8x8。
    const std::array<std::uint8_t, 16> pixels {
        255,
        0,
        0,
        255, // (0,0) red
        0,
        255,
        0,
        255, // (1,0) green
        0,
        0,
        255,
        255, // (0,1) blue
        255,
        255,
        255,
        255, // (1,1) white
    };
    const auto texture = device.create_rgba_texture(2, 2, pixels);
    REQUIRE(texture);
    REQUIRE(device.texture_size(texture).get_width() == 2.0F);
    REQUIRE(device.texture_size(texture).get_height() == 2.0F);

    device.draw_texture_region(
        texture,
        foundation::NanRect::from_ltrb(0.0F, 0.0F, 2.0F, 2.0F),
        foundation::NanRect::from_ltrb(8.0F, 8.0F, 24.0F, 24.0F),
        color_from_hex(0xFFFFFFFFU)
    );
    // 2x2 放大 8 倍且线性过滤：采样点落在纹素中心之间，断言通道主导关系而不是
    // 精确值；这仍能证明 RGBA 字节序与通道映射正确。
    const auto expect_dominant = [&](const int x, const int y, const int channel) {
        const auto actual = canvas.pixel(x, y);
        REQUIRE(actual.a == Catch::Approx(255).margin(kChannelTolerance));
        const int values[3] {actual.r, actual.g, actual.b};
        for (int index = 0; index < 3; ++index) {
            if (index != channel) {
                REQUIRE(values[index] < 60);
            }
        }
        REQUIRE(values[channel] > 190);
    };
    expect_dominant(12, 12, 0); // red
    expect_dominant(20, 12, 1); // green
    expect_dominant(12, 20, 2); // blue
    {
        const auto white = canvas.pixel(20, 20);
        REQUIRE(white.r > 190);
        REQUIRE(white.g > 190);
        REQUIRE(white.b > 190);
    }
    // 纹理之外保持背景。
    require_pixel(canvas.pixel(4, 4), from_hex(kBackground), "outside texture", 4, 4);
    device.destroy_texture(texture);

    // 非法尺寸 / 缓冲区不足：返回空 handle，不崩溃。
    REQUIRE_FALSE(device.create_rgba_texture(0, 2, pixels));
    REQUIRE_FALSE(device.create_rgba_texture(2, 2, std::span(pixels).first(8)));
}
