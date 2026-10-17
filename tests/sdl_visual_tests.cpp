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
#include <nandina/render/backends/sdl_device.hpp>
#include <nandina/scene/scene_tree.hpp>
#include <nandina/theme/design_system.hpp>
#include <nandina/widget/button.hpp>
#include <nandina/widget/dialog.hpp>

#include <SDL3/SDL.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
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
    SoftwareCanvas canvas {200, 120};
    render::SdlRenderDevice device {canvas.renderer()};

    const auto dialog = widget::Dialog::create();
    const auto scrim = to_rgba(dialog->resolved_style().scrim);
    INFO("scrim alpha " << scrim.a);
    REQUIRE(scrim.a > 0);
    REQUIRE(scrim.a < 255); // 遮罩本来就该是半透明的

    const auto background = from_hex(kBackground);
    device.clear(color_from_hex(kBackground));
    device.draw_rect(
        foundation::NanRect(0.0F, 0.0F, 200.0F, 120.0F),
        dialog->resolved_style().scrim
    );

    const auto expected = over(background, scrim);
    require_pixel(canvas.pixel(100, 60), expected, "scrim centre", 100, 60);
    // 遮罩不该把背景压成不透明的一块。
    REQUIRE(channels_differ(
        canvas.pixel(100, 60),
        Rgba {.r = scrim.r, .g = scrim.g, .b = scrim.b, .a = 255}
    ));
}
