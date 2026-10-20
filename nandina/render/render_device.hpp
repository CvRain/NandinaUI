//
// Created by cvrain on 2026/7/2.
//
// Abstract render device. All coordinates are world coordinates (screen pixels,
// y down). Backends submit primitives to a concrete renderer (raylib / offscreen
// / recording test double). No raylib types appear in this header.
//

#ifndef NANDINA_EXPERIMENT_RENDER_DEVICE_HPP
#define NANDINA_EXPERIMENT_RENDER_DEVICE_HPP

#include "../foundation/geometry.hpp"
#include "../foundation/nandina_color.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace nandina::render
{
    using foundation::NanColor;
    using foundation::NanPoint;
    using foundation::NanRect;
    using foundation::NanSize;

    struct TextureHandle {
        std::uint64_t value = 0;

        [[nodiscard]] explicit operator bool() const {
            return value != 0;
        }

        auto operator==(const TextureHandle&) const -> bool = default;
    };

    /// 图片加载时的可选预处理（在 raylib Image 上执行，像素坐标）。
    struct ImageLoadOptions {
        std::optional<NanRect> crop = std::nullopt; // 先裁剪
        std::optional<NanSize> resize = std::nullopt; // 再缩放到目标尺寸
        std::optional<NanColor> tint = std::nullopt; // 最后着色（ImageColorTint）
    };

    /// Abstract drawing device. Coordinates are world-space (screen pixels, y down).
    /// Implementations translate primitives into a concrete backend. Traversal-varying
    /// state (transform, clip, opacity) lives in DrawContext, not here — the device is
    /// a stateless "draw this primitive" executor plus frame/clip bracketing.
    class IRenderDevice {
    public:
        virtual ~IRenderDevice() = default;

        // ---- frame boundaries ----
        virtual void begin_frame() = 0;
        virtual void end_frame() = 0;

        /// Clear the whole framebuffer to a solid color (called at frame start).
        /// Default no-op so offscreen / recording devices need not implement it.
        virtual void clear(const NanColor& /*color*/) {}

        // ---- clipping (screen-space axis-aligned rect) ----
        /// Set the current clip rect; called by ClipStack, not by nodes directly.
        virtual void set_clip(const NanRect& screen_rect) = 0;
        virtual void clear_clip() = 0;

        // ---- primitives (world-space axis-aligned) ----
        virtual void draw_rect(const NanRect& rect, const NanColor& color) = 0;
        virtual void
        draw_rect_outline(const NanRect& rect, float thickness, const NanColor& color) = 0;
        /// 圆角矩形描边（圆角 > 0 时用，避免方框边角与圆角填充不贴合）。
        /// 默认回退到方框描边，测试设备可不实现。
        virtual void draw_rounded_rect_outline(
            const NanRect& rect,
            float radius,
            float thickness,
            const NanColor& color
        ) {
            (void)radius;
            draw_rect_outline(rect, thickness, color);
        }
        virtual void
        draw_rounded_rect(const NanRect& rect, float radius, const NanColor& color) = 0;
        virtual void
        draw_line(const NanPoint& a, const NanPoint& b, float thickness, const NanColor& color) = 0;
        virtual void draw_circle(const NanPoint& center, float radius, const NanColor& color) = 0;

        /// Draw a circle clipped by a rounded rectangle. Backends with analytic
        /// primitives should implement the exact intersection; the default is an
        /// unclipped circle for simple recording/fallback devices.
        virtual void draw_circle_clipped_rounded_rect(
            const NanPoint& center,
            float circle_radius,
            const NanRect& clip_rect,
            float clip_radius,
            const NanColor& color
        ) {
            (void)clip_rect;
            (void)clip_radius;
            draw_circle(center, circle_radius, color);
        }

        /// 圆环扇区。center 为圆心；inner/outer 为环的内外半径；start_radians 为起始角、
        /// sweep_radians 为张角（2π 表示整环）。角度以 +Y 为 0、向 +X 增大。
        /// 默认实现退化为按弦先绘制（不保证抗锯齿），后端应覆写。
        virtual void draw_arc(
            const NanPoint& center,
            float inner_radius,
            float outer_radius,
            float start_radians,
            float sweep_radians,
            const NanColor& color
        ) {
            if (!std::isfinite(center.get_x()) || !std::isfinite(center.get_y())
                || !std::isfinite(inner_radius) || !std::isfinite(outer_radius)
                || !std::isfinite(start_radians) || !std::isfinite(sweep_radians)
                || outer_radius <= 0.0F || inner_radius < 0.0F || inner_radius >= outer_radius
                || std::abs(sweep_radians) < 1.0e-4F)
            {
                return;
            }
            const float sweep = std::abs(sweep_radians);
            const float start =
                sweep_radians < 0.0F ? start_radians + sweep_radians : start_radians;
            const float thickness = outer_radius - inner_radius;
            const float mid = (inner_radius + outer_radius) * 0.5F;
            // 角度 0 在 +Y、向 +X 增大：点为 center + r * (sin, cos)。
            const int steps = std::clamp(static_cast<int>(std::ceil(sweep / 0.25F)), 2, 64);
            const float step = sweep / static_cast<float>(steps);
            auto previous = NanPoint(
                center.get_x() + mid * std::sin(start),
                center.get_y() + mid * std::cos(start)
            );
            for (int i = 1; i <= steps; ++i) {
                const float angle = start + step * static_cast<float>(i);
                const auto next = NanPoint(
                    center.get_x() + mid * std::sin(angle),
                    center.get_y() + mid * std::cos(angle)
                );
                draw_line(previous, next, thickness, color);
                previous = next;
            }
        }

        /// 软阴影：圆角矩形 + 软边衰减。rect 为阴影的圆角矩形（已含偏移），
        /// spread 为软边衰减宽度（屏幕空间）。默认 no-op，录制设备按需覆写计数。
        virtual void draw_rounded_rect_shadow(
            const NanRect& rect,
            float radius,
            float spread,
            const NanColor& color
        ) {
            (void)rect;
            (void)radius;
            (void)spread;
            (void)color;
        }

        /// Text uses a minimal "top-left anchor + backend default font" interface;
        /// full text layout is abstracted later.
        virtual void draw_text(
            std::string_view text,
            const NanPoint& pos,
            float font_size,
            const NanColor& color
        ) = 0;

        [[nodiscard]] virtual auto supports_alpha_textures() const -> bool {
            return false;
        }

        [[nodiscard]] virtual auto create_alpha_texture(
            int /*width*/,
            int /*height*/,
            std::span<const std::uint8_t> /*alpha*/
        ) -> TextureHandle {
            return {};
        }

        virtual void update_alpha_texture(
            TextureHandle /*texture*/,
            int /*width*/,
            int /*height*/,
            std::span<const std::uint8_t> /*alpha*/
        ) {}

        /// Upload only `region` of an alpha texture. `alpha` is still the **full**
        /// tightly packed buffer, so a device that cannot address a sub-rectangle can
        /// fall back to the full upload below — correctness is preserved everywhere,
        /// while real backends transfer only what changed.
        ///
        /// 这个入口是为字形图集准备的：一次文本布局会连续写入许多小字形，整张图集
        /// 重传一次要几十毫秒，按脏矩形传则只跟新增字形的面积成正比。
        virtual void update_alpha_texture_region(
            TextureHandle texture,
            int full_width,
            int full_height,
            const foundation::NanRect& /*region*/,
            std::span<const std::uint8_t> alpha
        ) {
            update_alpha_texture(texture, full_width, full_height, alpha);
        }

        virtual void destroy_texture(TextureHandle /*texture*/) {}

        /// Upload tightly packed RGBA8 pixels. This is a render-thread operation;
        /// callers may decode pixels in the background but must upload on the UI thread.
        [[nodiscard]] virtual auto create_rgba_texture(
            int /*width*/,
            int /*height*/,
            std::span<const std::uint8_t> /*rgba*/
        ) -> TextureHandle {
            return {};
        }

        virtual void draw_texture_region(
            TextureHandle /*texture*/,
            const NanRect& /*source*/,
            const NanRect& /*destination*/,
            const NanColor& /*tint*/
        ) {}

        /// 从文件路径加载 RGBA 图片纹理（可带 crop/resize/tint 预处理）；失败返回空 handle。
        /// 默认 no-op（录制/无窗口设备覆写）。
        [[nodiscard]] virtual auto
        load_texture_from_file(std::string_view path, const ImageLoadOptions& options = {})
            -> TextureHandle {
            (void)path;
            (void)options;
            return {};
        }

        /// 从已编码图片字节加载 RGBA 纹理；media_type 用于选择解码器，失败返回空 handle。
        /// 默认 no-op（录制/无窗口设备按需覆写）。
        [[nodiscard]] virtual auto load_texture_from_memory(
            std::span<const std::uint8_t> bytes,
            std::string_view media_type,
            const ImageLoadOptions& options = {}
        ) -> TextureHandle {
            (void)bytes;
            (void)media_type;
            (void)options;
            return {};
        }

        /// 查询纹理自然像素尺寸；无效 handle 返回 {0,0}。默认 no-op。
        [[nodiscard]] virtual auto texture_size(TextureHandle texture) -> NanSize {
            (void)texture;
            return NanSize {};
        }

        // ---- capability queries (let nodes pick a fallback path) ----
        [[nodiscard]] virtual auto supports_rounded_rect() const -> bool {
            return true;
        }

        /// 是否能把**圆角轮廓**当裁剪边界用（`draw_circle_clipped_rounded_rect`）。
        ///
        /// 返回 false 不代表方法不可用：接口默认实现只是画一个**未裁剪**的圆。所以消费者
        /// 不能假定「调了就会被裁」—— 返回 false 时必须自己用 `set_clip()` 配上圆，
        /// 否则内容会溢出到圆角容器之外（见 platform_backend_contract.md §3）。
        [[nodiscard]] virtual auto supports_rounded_clip() const -> bool {
            return true;
        }

        /// 是否真的实现了软阴影（`draw_rounded_rect_shadow`）。默认 true 与
        /// `supports_rounded_rect()` 保持同一约定：能做的设备不必覆写，做不到的必须显式
        /// 返回 false，让消费者退化为硬边阴影而不是什么都不画。
        [[nodiscard]] virtual auto supports_soft_shadows() const -> bool {
            return true;
        }

        /// 是否提供**解析抗锯齿**（SDF 路径）。默认 false；消费者至少是诊断报告与设备测试。
        ///
        /// 返回 false 不代表不能画：CPU 几何路径仍然可用，只是边缘是硬边。
        [[nodiscard]] virtual auto analytic_antialiasing() const -> bool {
            return false;
        }

        /// 是否能**直接**从文件/内存加载图片（`load_texture_from_file` /
        /// `load_texture_from_memory`，解码在设备内部完成）。接口默认实现是 no-op，
        /// 做不到的设备必须如实返回 false，让消费者在尝试之前就知道会失败。
        [[nodiscard]] virtual auto supports_direct_image_loading() const -> bool {
            return false;
        }

        /// 是否能上传调用方在 CPU 侧解码好的 RGBA8 像素（`create_rgba_texture`）。
        /// 这是"应用提供解码器 + 后端负责上传"的服务路径；默认 no-op 返回 false。
        [[nodiscard]] virtual auto supports_rgba_texture_upload() const -> bool {
            return false;
        }
    };

} // namespace nandina::render

#endif // NANDINA_EXPERIMENT_RENDER_DEVICE_HPP
