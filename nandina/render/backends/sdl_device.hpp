//
// render/backends/sdl_device — 用 SDL3 的 2D Renderer 实现 Nandina 的 IRenderDevice。
//
// 这是**生产候选后端**，不是探测专用代码：探测（probe/cross_platform/）只是它最早的宿主。
// 只有在 `-Dsdl_backend=enabled|auto` 且找到 SDL3 >= 3.4.0 时才编进 libnandina
// （见 nandina/meson.build 的 module_sdl_backend）。
//
// 为什么先做「几何路径」而不是把生产的 SDF fragment shader 搬过来：
//
//   1. 探测记录（docs/references/cross_platform_backend_probe.md §4）把「让探测走一次真实的
//      IRenderDevice」列为 P1 的前置条件 —— 只有这样测到的才是框架行为（接口的默认实现、
//      capability 查询、clip 语义），而不是另一个程序的渲染。
//   2. 同一份记录 §2.1 的暂定 Web 路线是「CPU 生成几何/覆盖率 + 独立的 Web 设备」，
//      并要求验证这条路线的可行性。圆角与圆用三角形扇画出来、不依赖自定义 shader，
//      正好就是那条路线的最小验证。
//   3. 自定义 SDF shader 与现有 6 种图元的**等价性**是另一件事（记录 §1 的未验证项），
//      留到本设备能跑通之后再逐图元比对。
//
// 本设备只实现「能用 SDL 2D API 表达」的部分：矩形/描边/圆角/圆/线段/文字/alpha 纹理/裁剪。
// 图像文件加载（load_texture_from_file/memory）与 `draw_rounded_rect_shadow` 仍走接口默认
// 实现（no-op），因为它们要么需要解码器，要么是 SDF 专属效果 —— 这两条差异会打印在能力摘要里。

#ifndef NANDINA_EXPERIMENT_RENDER_BACKENDS_SDL_DEVICE_HPP
#define NANDINA_EXPERIMENT_RENDER_BACKENDS_SDL_DEVICE_HPP

#include "../render_device.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace nandina::render
{
    /// 统计信息：用来把「这个设备被要求画了什么」变成可打印的事实，而不是凭截图猜。
    struct SdlDeviceStats {
        std::uint64_t frames = 0;
        std::uint64_t rects = 0;
        std::uint64_t rect_outlines = 0;
        std::uint64_t rounded_rects = 0;
        std::uint64_t rounded_outlines = 0;
        std::uint64_t circles = 0;
        std::uint64_t arcs = 0;
        std::uint64_t lines = 0;
        std::uint64_t texts = 0;
        std::uint64_t texture_draws = 0;
        std::uint64_t alpha_uploads = 0;
        std::uint64_t rgba_uploads = 0;
        std::uint64_t clips = 0;
        /// 几何路径生成的顶点数：CPU 细分几何的代价可以直接读出来。
        std::uint64_t generated_vertices = 0;
        /// 当前持有的 alpha 纹理字节数（RGBA8）。不是 GPU 总内存，只是本设备创建的贴图。
        std::uint64_t texture_bytes = 0;
    };

    /// SDF 路径的运行期计数：绑定次数、缓存命中/未命中（未命中即创建）、缓存规模。
    /// 生产接线的验收看这些数：稳定页面上 misses 应趋近 0，binds 应等于参数块连续段数。
    struct SdfDiagnostics {
        /// 生命周期累计的非空绑定次数（不含解绑）。
        std::uint64_t binds = 0;
        /// 本帧的非空绑定 / 解绑次数（begin_frame 清零）；契约要求的"每帧状态切换次数"取两者之和。
        std::uint64_t frame_binds = 0;
        std::uint64_t frame_unbinds = 0;
        std::uint64_t hits = 0;
        std::uint64_t misses = 0;
        std::size_t cached = 0;
        std::size_t capacity = 0;
    };

    class SdlRenderDevice final: public render::IRenderDevice {
    public:
        /// `renderer` 由调用方持有；本设备不创建窗口/渲染器，也不 present。
        explicit SdlRenderDevice(SDL_Renderer& renderer);
        ~SdlRenderDevice() override;

        auto stats() const -> const SdlDeviceStats&;

        // ---- frame boundaries ----
        void begin_frame() override;
        void end_frame() override;
        void clear(const foundation::NanColor& color) override;

        // ---- clipping ----
        void set_clip(const foundation::NanRect& screen_rect) override;
        void clear_clip() override;

        // ---- primitives ----
        void draw_rect(const foundation::NanRect& rect, const foundation::NanColor& color) override;
        void draw_rect_outline(
            const foundation::NanRect& rect,
            float thickness,
            const foundation::NanColor& color
        ) override;
        void draw_rounded_rect(
            const foundation::NanRect& rect,
            float radius,
            const foundation::NanColor& color
        ) override;
        /// 圆角描边：四条直边 + 四个同心环扇。**必须覆写** —— 接口的默认实现会退化成
        /// 方框描边，圆角按钮/卡片的边框会在四角露出直角（anchors 页面的按钮就是这么被
        /// 消费者回归扫出来的）。
        void draw_rounded_rect_outline(
            const foundation::NanRect& rect,
            float radius,
            float thickness,
            const foundation::NanColor& color
        ) override;
        /// 圆环扇区：同心环扇一次提交，不退化到接口默认的弦逼近折线（Spinner/ProgressBar 用它）。
        void draw_arc(
            const foundation::NanPoint& center,
            float inner_radius,
            float outer_radius,
            float start_radians,
            float sweep_radians,
            const foundation::NanColor& color
        ) override;
        void draw_line(
            const foundation::NanPoint& a,
            const foundation::NanPoint& b,
            float thickness,
            const foundation::NanColor& color
        ) override;
        void draw_circle(
            const foundation::NanPoint& center,
            float radius,
            const foundation::NanColor& color
        ) override;
        void draw_circle_clipped_rounded_rect(
            const foundation::NanPoint& center,
            float circle_radius,
            const foundation::NanRect& clip_rect,
            float clip_radius,
            const foundation::NanColor& color
        ) override;
        void draw_text(
            std::string_view text,
            const foundation::NanPoint& pos,
            float font_size,
            const foundation::NanColor& color
        ) override;

        // ---- alpha textures（字形图集的入口）----
        [[nodiscard]] auto supports_alpha_textures() const -> bool override;
        [[nodiscard]] auto
        create_alpha_texture(int width, int height, std::span<const std::uint8_t> alpha)
            -> render::TextureHandle override;
        void update_alpha_texture(
            render::TextureHandle texture,
            int width,
            int height,
            std::span<const std::uint8_t> alpha
        ) override;
        void update_alpha_texture_region(
            render::TextureHandle texture,
            int full_width,
            int full_height,
            const foundation::NanRect& region,
            std::span<const std::uint8_t> alpha
        ) override;
        void destroy_texture(render::TextureHandle texture) override;
        void draw_texture_region(
            render::TextureHandle texture,
            const foundation::NanRect& source,
            const foundation::NanRect& destination,
            const foundation::NanColor& tint
        ) override;
        [[nodiscard]] auto texture_size(render::TextureHandle texture)
            -> foundation::NanSize override;

        // ---- capabilities ----
        [[nodiscard]] auto supports_rounded_rect() const -> bool override;
        /// 只能按外接矩形裁剪（圆角处退化为方角），返回 false。
        [[nodiscard]] auto supports_rounded_clip() const -> bool override;
        /// 未实现软阴影，返回 false，消费者退化为硬边阴影。
        [[nodiscard]] auto supports_soft_shadows() const -> bool override;
        /// 不能加载图片（无解码器/loader），返回 false。
        [[nodiscard]] auto supports_direct_image_loading() const -> bool override;
        [[nodiscard]] auto supports_rgba_texture_upload() const -> bool override;
        [[nodiscard]] auto
        create_rgba_texture(int width, int height, std::span<const std::uint8_t> rgba)
            -> render::TextureHandle override;
        /// SDF 路径是否生效（GPU renderer 且 render state 创建成功）。这是 `render_path` 的事实来源。
        [[nodiscard]] auto analytic_antialiasing() const -> bool override;

        /// SDF 路径的计数快照（无 SDF 路径时全零）。
        [[nodiscard]] auto sdf_diagnostics() const -> SdfDiagnostics;
        /// SDF 路径不可用的原因（可用于诊断报告）；可用时为空。
        [[nodiscard]] auto sdf_unavailable_reason() const -> std::string_view;
        /// 实际使用的着色器产物格式（`spirv` / `dxil` / `msl`）；无 SDF 路径时为空。
        [[nodiscard]] auto sdf_shader_format() const -> std::string_view;

    private:
        struct SdfPath;
        /// 当前 render scale（要求各向同性）；不可用时返回 false，调用方回落到 CPU 几何。
        auto sdf_scale(float& scale) const -> bool;
        /// scale 与设备像素原点（含 letterbox 偏移）：参数块按设备像素编码。
        auto sdf_scale_and_origin(float& scale, float& origin_x, float& origin_y) const -> bool;
        /// 提交一次 SDF 图元：cover 为逻辑坐标覆盖 quad，values 为 32 字节参数块。
        auto submit_sdf(
            const foundation::NanRect& cover,
            const std::array<float, 8>& values,
            const foundation::NanColor& color
        ) -> bool;
        /// 线段与圆环扇区的 SDF 编码；失败时调用方回落到 CPU 几何。
        auto draw_sdf_segment(
            const foundation::NanPoint& a,
            const foundation::NanPoint& b,
            float half_width,
            const foundation::NanColor& color
        ) -> bool;
        auto draw_sdf_arc(
            const foundation::NanPoint& center,
            float inner_radius,
            float outer_radius,
            float start_radians,
            float sweep_radians,
            const foundation::NanColor& color
        ) -> bool;
        /// bounds 为图元边界（描边传入中心线边界）；mode 0 = 填充、1 = 中心线描边。
        auto draw_sdf_shape(
            const foundation::NanRect& bounds,
            float radius,
            float half_width,
            int mode,
            const foundation::NanColor& color
        ) -> bool;
        void unbind_sdf();

        void set_draw_color(const foundation::NanColor& color);
        /// 圆形/圆角的三角形扇：返回顶点数（同时计入 stats）。
        auto draw_fan(
            const foundation::NanPoint& center,
            float radius,
            float start_radians,
            float end_radians,
            const foundation::NanColor& color
        ) -> int;
        SDL_Texture* lookup(render::TextureHandle handle) const;

        SDL_Renderer* renderer_ = nullptr;
        SdlDeviceStats stats_ {};
        std::map<std::uint64_t, SDL_Texture*> textures_ {};
        std::uint64_t next_texture_ = 1;
        std::unique_ptr<SdfPath> sdf_;
        std::string sdf_reason_;
    };

} // namespace nandina::render

#endif // NANDINA_EXPERIMENT_RENDER_BACKENDS_SDL_DEVICE_HPP
