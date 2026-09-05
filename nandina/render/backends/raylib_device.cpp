//
// Created by cvrain on 2026/7/2.
//
// raylib backend for IRenderDevice. raylib is included ONLY here; all
// nandina<->raylib type conversions live in the anonymous namespace below.
//

#include "raylib_device.hpp"
#include "sdf_primitive_geometry.hpp"

#include "../../foundation/nandina_color_space.hpp"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace nandina::render
{
    namespace
    {

        auto to_rl(const NanPoint& p) -> ::Vector2 {
            return {p.get_x(), p.get_y()};
        }

        auto to_rl(const NanColor& c) -> ::Color {
            const auto hex = c.to<foundation::NanHexRgb>();
            return {hex.red, hex.green, hex.blue, hex.alpha};
        }

        auto to_rl(const NanRect& r) -> ::Rectangle {
            return {r.get_left(), r.get_top(), r.get_width(), r.get_height()};
        }

        [[nodiscard]] auto alpha_to_rgba(std::span<const std::uint8_t> alpha)
            -> std::vector<::Color> {
            std::vector<::Color> pixels;
            pixels.reserve(alpha.size());
            for (const auto value: alpha) {
                pixels.push_back(::Color {255, 255, 255, value});
            }
            return pixels;
        }

        [[nodiscard]] auto image_extension(const std::string_view media_type)
            -> std::optional<std::string_view> {
            if (media_type == "image/png") {
                return ".png";
            }
            if (media_type == "image/jpeg") {
                return ".jpg";
            }
            if (media_type == "image/webp") {
                return ".webp";
            }
            if (media_type == "image/bmp") {
                return ".bmp";
            }
            if (media_type == "image/gif") {
                return ".gif";
            }
            if (media_type == "image/qoi") {
                return ".qoi";
            }
            if (media_type == "image/tga") {
                return ".tga";
            }
            return std::nullopt;
        }

    } // namespace

    // ─── 抗锯齿图元：SDF 着色器 ──────────────────────────────────────────────
    // raylib 的 2D 图元（DrawRectangleRounded* / DrawLineEx 等）不做抗锯齿，且本平台
    // 默认帧缓冲的 MSAA 实际未生效（斜线仍是硬阶梯）。改用 SDF 片段着色器做逐像素
    // 软边，统一覆盖圆角填充 / 圆角描边 / 矩形 / 圆 / 线段，圆角半径与线宽任意。
    // 顶点着色器沿用 raylib 约定（mvp / texture0 / colDiffuse），由 raylib 批量自动更新。

    constexpr const char* kAaVertexShader = R"GLSL(
#version 330 core
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;
uniform mat4 mvp;
out vec2 fragTexCoord;
out vec4 fragColor;
void main() {
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)GLSL";

    constexpr const char* kAaFragmentShader = R"GLSL(
#version 330 core
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec4 uShapeRect; // 图元矩形（raylib 屏幕坐标，y 向下）
uniform vec4 uQuadRect;  // 覆盖 quad；包含图元外侧的抗锯齿过渡区
uniform vec2 uRadius;  // x = 圆角半径；y = 线段 / 描边半宽
uniform vec4 uColor;   // 实心颜色（RGBA 0..1）
uniform int uMode;     // 0 = fill; 1 = outline; 2 = segment; 3 = circle clipped by round rect; 4 = shadow
uniform vec2 uA;       // 线段端点 A（模式 2）
uniform vec2 uB;       // 线段端点 B（模式 2）
out vec4 finalColor;

float sdRoundRect(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + vec2(r);
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

float sdSegment(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / max(dot(ba, ba), 1e-6), 0.0, 1.0);
    return length(pa - ba * h);
}

void main() {
    vec2 p = uQuadRect.xy + fragTexCoord * uQuadRect.zw;
    float alpha;
    if (uMode == 2) {
        float d = sdSegment(p, uA, uB);
        float aa = max(fwidth(d), 1e-4);
        alpha = 1.0 - smoothstep(-aa * 0.5, aa * 0.5, d - uRadius.y);
    }
    else {
        vec2 center = uShapeRect.xy + uShapeRect.zw * 0.5;
        float sd = sdRoundRect(p - center, uShapeRect.zw * 0.5, uRadius.x);
        float aa = max(fwidth(sd), 1e-4);
        if (uMode == 3) {
            float circleSd = length(p - uA) - uRadius.y;
            float combined = max(sd, circleSd);
            float combinedAa = max(fwidth(combined), 1e-4);
            alpha = 1.0 - smoothstep(-combinedAa * 0.5, combinedAa * 0.5, combined);
        }
        else if (uMode == 1) {
            alpha = 1.0 - smoothstep(-aa * 0.5, aa * 0.5, abs(sd) - uRadius.y);
        }
        else if (uMode == 4) {
            // shadow：矩形内不透明，向外在 spread（uRadius.y）距离内渐隐。
            alpha = 1.0 - smoothstep(0.0, max(uRadius.y, 1e-3), sd);
        }
        else {
            alpha = 1.0 - smoothstep(-aa * 0.5, aa * 0.5, sd);
        }
    }
    finalColor = vec4(uColor.rgb, uColor.a * alpha);
}
)GLSL";

    /// raylib-backed device. Stateless w.r.t. traversal; only frame/clip bracketing.
    class RaylibRenderDevice final: public IRenderDevice {
    public:
        ~RaylibRenderDevice() override {
            for (const auto& [handle, texture]: textures_) {
                (void)handle;
                UnloadTexture(texture);
            }
            if (aa_shader_.id != 0) {
                UnloadShader(aa_shader_);
            }
            if (aa_white_tex_.id != 0) {
                UnloadTexture(aa_white_tex_);
            }
        }

        void begin_frame() override {
            BeginDrawing();
        }
        void end_frame() override {
            EndDrawing();
        }

        void clear(const NanColor& c) override {
            ClearBackground(to_rl(c));
        }

        void set_clip(const NanRect& r) override {
            // raylib scissor doesn't nest (each call overrides). That's fine: the
            // ClipStack always submits the already-intersected top rect.
            if (!r.is_valid()) {
                // Fully clipped: a zero-area scissor produces no visible pixels.
                BeginScissorMode(0, 0, 0, 0);
                return;
            }
            BeginScissorMode(
                static_cast<int>(r.get_left()),
                static_cast<int>(r.get_top()),
                static_cast<int>(r.get_width()),
                static_cast<int>(r.get_height())
            );
        }

        void clear_clip() override {
            EndScissorMode();
        }

        void draw_rect(const NanRect& r, const NanColor& c) override {
            if (r.get_width() <= 0.0F || r.get_height() <= 0.0F) {
                return;
            }
            draw_aa(r, 0.0F, 0.0F, detail::SdfPrimitiveMode::fill, to_rl(c));
        }

        void draw_rect_outline(const NanRect& r, float thickness, const NanColor& c) override {
            if (r.get_width() <= 0.0F || r.get_height() <= 0.0F || thickness <= 0.0F) {
                return;
            }
            const auto outline = detail::sdf_inner_outline_geometry(r, 0.0F, thickness);
            draw_aa(
                outline.centerline_bounds,
                outline.centerline_radius,
                outline.half_width,
                detail::SdfPrimitiveMode::outline,
                to_rl(c)
            );
        }

        void draw_rounded_rect_outline(
            const NanRect& r,
            float radius,
            float thickness,
            const NanColor& c
        ) override {
            if (r.get_width() <= 0.0F || r.get_height() <= 0.0F || thickness <= 0.0F) {
                return;
            }
            const float corner =
                std::clamp(radius, 0.0F, std::min(r.get_width(), r.get_height()) * 0.5F);
            const auto outline = detail::sdf_inner_outline_geometry(r, corner, thickness);
            draw_aa(
                outline.centerline_bounds,
                outline.centerline_radius,
                outline.half_width,
                detail::SdfPrimitiveMode::outline,
                to_rl(c)
            );
        }

        void draw_rounded_rect(const NanRect& r, float radius, const NanColor& c) override {
            if (r.get_width() <= 0.0F || r.get_height() <= 0.0F) {
                return;
            }
            const float corner =
                std::clamp(radius, 0.0F, std::min(r.get_width(), r.get_height()) * 0.5F);
            draw_aa(r, corner, 0.0F, detail::SdfPrimitiveMode::fill, to_rl(c));
        }

        void draw_line(
            const NanPoint& a,
            const NanPoint& b,
            float thickness,
            const NanColor& c
        ) override {
            if (thickness <= 0.0F) {
                return;
            }
            const float hw = thickness * 0.5F;
            draw_aa(
                detail::sdf_segment_bounds(a, b, hw),
                0.0F,
                hw,
                detail::SdfPrimitiveMode::segment,
                to_rl(c),
                a,
                b
            );
        }

        void draw_circle(const NanPoint& center, float radius, const NanColor& c) override {
            if (radius <= 0.0F) {
                return;
            }
            draw_aa(
                foundation::NanRect::from_xywh(
                    center.get_x() - radius,
                    center.get_y() - radius,
                    radius * 2.0F,
                    radius * 2.0F
                ),
                radius,
                0.0F,
                detail::SdfPrimitiveMode::fill,
                to_rl(c)
            );
        }

        void draw_circle_clipped_rounded_rect(
            const NanPoint& center,
            const float circle_radius,
            const NanRect& clip_rect,
            const float clip_radius,
            const NanColor& color
        ) override {
            if (circle_radius <= 0.0F || clip_rect.get_width() <= 0.0F
                || clip_rect.get_height() <= 0.0F)
            {
                return;
            }
            const float corner = std::clamp(
                clip_radius,
                0.0F,
                std::min(clip_rect.get_width(), clip_rect.get_height()) * 0.5F
            );
            draw_aa(
                clip_rect,
                corner,
                circle_radius,
                detail::SdfPrimitiveMode::clipped_circle,
                to_rl(color),
                center
            );
        }

        void draw_rounded_rect_shadow(
            const NanRect& r,
            const float radius,
            const float spread,
            const NanColor& c
        ) override {
            if (r.get_width() <= 0.0F || r.get_height() <= 0.0F || spread <= 0.0F) {
                return;
            }
            const float corner =
                std::clamp(radius, 0.0F, std::min(r.get_width(), r.get_height()) * 0.5F);
            draw_aa(r, corner, spread, detail::SdfPrimitiveMode::shadow, to_rl(c));
        }

        /// 懒加载 SDF 抗锯齿着色器与 1x1 白纹理。
        void ensure_aa() {
            if (aa_ready_) {
                return;
            }
            aa_ready_ = true;
            // 注意：LoadShader() 接收的是文件路径；这里必须用 LoadShaderFromMemory()。
            aa_shader_ = LoadShaderFromMemory(kAaVertexShader, kAaFragmentShader);
            if (aa_shader_.id == 0) {
                return;
            }
            aa_loc_shape_rect_ = GetShaderLocation(aa_shader_, "uShapeRect");
            aa_loc_quad_rect_ = GetShaderLocation(aa_shader_, "uQuadRect");
            aa_loc_radius_ = GetShaderLocation(aa_shader_, "uRadius");
            aa_loc_color_ = GetShaderLocation(aa_shader_, "uColor");
            aa_loc_mode_ = GetShaderLocation(aa_shader_, "uMode");
            aa_loc_a_ = GetShaderLocation(aa_shader_, "uA");
            aa_loc_b_ = GetShaderLocation(aa_shader_, "uB");
            // 自定义 uniform 找不到 = 回退到了默认着色器（加载失败），标记不可用。
            if (aa_loc_shape_rect_ < 0 || aa_loc_quad_rect_ < 0 || aa_loc_radius_ < 0
                || aa_loc_color_ < 0 || aa_loc_mode_ < 0)
            {
                UnloadShader(aa_shader_);
                aa_shader_.id = 0;
                return;
            }
            const std::uint8_t white[4] = {255, 255, 255, 255};
            Image image {
                .data = const_cast<std::uint8_t*>(white),
                .width = 1,
                .height = 1,
                .mipmaps = 1,
                .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
            };
            aa_white_tex_ = LoadTextureFromImage(image);
        }

        /// 着色器不可用时的回退：沿用 raylib 原生无抗锯齿图元。
        void draw_aa_fallback(
            const NanRect& rect,
            const float radius,
            const float half_width,
            const detail::SdfPrimitiveMode mode,
            const ::Color& color,
            const std::optional<NanPoint>& line_a,
            const std::optional<NanPoint>& line_b
        ) {
            const float shortest = std::min(rect.get_width(), rect.get_height());
            const float corner = std::clamp(radius, 0.0F, shortest * 0.5F);
            const float roundness = shortest > 0.0F ? corner * 2.0F / shortest : 0.0F;
            if (mode == detail::SdfPrimitiveMode::segment) {
                DrawLineEx(to_rl(*line_a), to_rl(*line_b), half_width * 2.0F, color);
            }
            else if (mode == detail::SdfPrimitiveMode::clipped_circle) {
                // Do not alter raylib's scissor state here: DrawContext owns the active clip,
                // and ending a temporary scissor would silently discard an ancestor clip.
                // The native fallback cannot express a rounded intersection, so preserve
                // traversal state and degrade to the unclipped impact circle instead.
                DrawCircleV(to_rl(*line_a), half_width, color);
            }
            else if (mode == detail::SdfPrimitiveMode::outline) {
                DrawRectangleRoundedLinesEx(to_rl(rect), roundness, 16, half_width * 2.0F, color);
            }
            else if (corner > 0.0F) {
                DrawRectangleRounded(to_rl(rect), roundness, 16, color);
            }
            else {
                DrawRectangleRec(to_rl(rect), color);
            }
        }

        /// 用 SDF 着色器绘制一个抗锯齿图元（覆盖矩形 / 圆角矩形 / 圆 / 描边 / 线段）。
        void draw_aa(
            const NanRect& rect,
            const float radius,
            const float half_width,
            const detail::SdfPrimitiveMode mode,
            const ::Color& color,
            const std::optional<NanPoint>& line_a = std::nullopt,
            const std::optional<NanPoint>& line_b = std::nullopt
        ) {
            ensure_aa();
            if (aa_shader_.id == 0 || aa_white_tex_.id == 0) {
                draw_aa_fallback(rect, radius, half_width, mode, color, line_a, line_b);
                return;
            }
            const auto quad = mode == detail::SdfPrimitiveMode::segment ? rect
                : mode == detail::SdfPrimitiveMode::clipped_circle
                ? rect
                : detail::sdf_quad_bounds(rect, mode, half_width);
            const ::Rectangle shape_rl = to_rl(rect);
            const ::Rectangle quad_rl = to_rl(quad);
            const ::Vector4 rgba = ColorNormalize(color);
            const float shape_rect_v[4] = {
                shape_rl.x,
                shape_rl.y,
                shape_rl.width,
                shape_rl.height,
            };
            const float quad_rect_v[4] = {quad_rl.x, quad_rl.y, quad_rl.width, quad_rl.height};
            const float radius_v[2] = {radius, half_width};
            const float color_v[4] = {rgba.x, rgba.y, rgba.z, rgba.w};
            const int mode_value = static_cast<int>(mode);
            BeginShaderMode(aa_shader_);
            SetShaderValue(aa_shader_, aa_loc_shape_rect_, shape_rect_v, SHADER_UNIFORM_VEC4);
            SetShaderValue(aa_shader_, aa_loc_quad_rect_, quad_rect_v, SHADER_UNIFORM_VEC4);
            if (aa_loc_radius_ >= 0) {
                SetShaderValue(aa_shader_, aa_loc_radius_, radius_v, SHADER_UNIFORM_VEC2);
            }
            if (aa_loc_color_ >= 0) {
                SetShaderValue(aa_shader_, aa_loc_color_, color_v, SHADER_UNIFORM_VEC4);
            }
            if (aa_loc_mode_ >= 0) {
                SetShaderValue(aa_shader_, aa_loc_mode_, &mode_value, SHADER_UNIFORM_INT);
            }
            if (aa_loc_a_ >= 0 && line_a) {
                const float a_v[2] = {line_a->get_x(), line_a->get_y()};
                SetShaderValue(aa_shader_, aa_loc_a_, a_v, SHADER_UNIFORM_VEC2);
            }
            if (aa_loc_b_ >= 0 && line_b) {
                const float b_v[2] = {line_b->get_x(), line_b->get_y()};
                SetShaderValue(aa_shader_, aa_loc_b_, b_v, SHADER_UNIFORM_VEC2);
            }
            DrawTexturePro(
                aa_white_tex_,
                {0.0F, 0.0F, 1.0F, 1.0F},
                quad_rl,
                {0.0F, 0.0F},
                0.0F,
                WHITE
            );
            EndShaderMode();
        }

        void draw_text(
            std::string_view text,
            const NanPoint& pos,
            float font_size,
            const NanColor& c
        ) override {
            const std::string owned(text);
            DrawText(
                owned.c_str(),
                static_cast<int>(pos.get_x()),
                static_cast<int>(pos.get_y()),
                static_cast<int>(font_size),
                to_rl(c)
            );
        }

        [[nodiscard]] auto supports_alpha_textures() const -> bool override {
            return true;
        }

        [[nodiscard]] auto
        create_alpha_texture(int width, int height, std::span<const std::uint8_t> alpha)
            -> TextureHandle override {
            if (width <= 0 || height <= 0
                || alpha.size() != static_cast<std::size_t>(width * height))
            {
                return {};
            }

            auto rgba = alpha_to_rgba(alpha);
            Image image {
                .data = rgba.data(),
                .width = width,
                .height = height,
                .mipmaps = 1,
                .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
            };
            const auto texture = LoadTextureFromImage(image);
            if (texture.id == 0) {
                return {};
            }
            // alpha texture 当前用于 FreeType glyph atlas。bitmap 已包含 grayscale AA，
            // 1:1 绘制时使用点采样，避免双线性过滤造成第二次软化与 atlas 邻区渗色。
            SetTextureFilter(texture, TEXTURE_FILTER_POINT);

            const TextureHandle handle {.value = next_texture_handle_++};
            textures_.emplace(handle.value, texture);
            return handle;
        }

        void update_alpha_texture(
            TextureHandle handle,
            int width,
            int height,
            std::span<const std::uint8_t> alpha
        ) override {
            const auto found = textures_.find(handle.value);
            if (found == textures_.end() || found->second.width != width
                || found->second.height != height
                || alpha.size() != static_cast<std::size_t>(width * height))
            {
                return;
            }
            const auto rgba = alpha_to_rgba(alpha);
            UpdateTexture(found->second, rgba.data());
        }

        void destroy_texture(TextureHandle handle) override {
            const auto found = textures_.find(handle.value);
            if (found == textures_.end()) {
                return;
            }
            UnloadTexture(found->second);
            textures_.erase(found);
        }

        void draw_texture_region(
            TextureHandle handle,
            const NanRect& source,
            const NanRect& destination,
            const NanColor& tint
        ) override {
            const auto found = textures_.find(handle.value);
            if (found == textures_.end()) {
                return;
            }
            DrawTexturePro(
                found->second,
                to_rl(source),
                to_rl(destination),
                ::Vector2 {0.0F, 0.0F},
                0.0F,
                to_rl(tint)
            );
        }

        [[nodiscard]] auto create_rgba_texture(
            const int width,
            const int height,
            const std::span<const std::uint8_t> rgba
        ) -> TextureHandle override {
            if (width <= 0 || height <= 0
                || rgba.size()
                    != static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U)
            {
                return {};
            }
            Image image {
                .data = const_cast<std::uint8_t*>(rgba.data()),
                .width = width,
                .height = height,
                .mipmaps = 1,
                .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
            };
            return retain_texture(LoadTextureFromImage(image));
        }

        [[nodiscard]] auto
        load_texture_from_file(std::string_view path, const ImageLoadOptions& options)
            -> TextureHandle override {
            const std::string path_str(path);
            Image image = LoadImage(path_str.c_str());
            return upload_image(image, options);
        }

        [[nodiscard]] auto load_texture_from_memory(
            const std::span<const std::uint8_t> bytes,
            const std::string_view media_type,
            const ImageLoadOptions& options
        ) -> TextureHandle override {
            const auto extension = image_extension(media_type);
            if (!extension || bytes.empty()
                || bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            {
                return {};
            }
            const std::string extension_str(*extension);
            Image image = LoadImageFromMemory(
                extension_str.c_str(),
                bytes.data(),
                static_cast<int>(bytes.size())
            );
            return upload_image(image, options);
        }

        [[nodiscard]] auto texture_size(TextureHandle texture) -> NanSize override {
            const auto found = textures_.find(texture.value);
            if (found == textures_.end()) {
                return NanSize {};
            }
            return NanSize(
                static_cast<float>(found->second.width),
                static_cast<float>(found->second.height)
            );
        }

    private:
        [[nodiscard]] auto upload_image(Image image, const ImageLoadOptions& options)
            -> TextureHandle {
            if (image.data == nullptr) {
                return {};
            }
            // 可选预处理：先裁剪、再缩放、最后着色（raylib Image 均为 CPU 侧操作）。
            if (options.crop) {
                ImageCrop(&image, to_rl(*options.crop));
            }
            if (options.resize) {
                ImageResize(
                    &image,
                    static_cast<int>(options.resize->get_width()),
                    static_cast<int>(options.resize->get_height())
                );
            }
            if (options.tint) {
                ImageColorTint(&image, to_rl(*options.tint));
            }
            const auto texture = LoadTextureFromImage(image);
            UnloadImage(image);
            return retain_texture(texture);
        }

        [[nodiscard]] auto retain_texture(const ::Texture2D texture) -> TextureHandle {
            if (texture.id == 0)
                return {};
            // RGBA 图片用双线性过滤（与 glyph atlas 的点采样区分），缩放更平滑。
            SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
            const TextureHandle handle {.value = next_texture_handle_++};
            textures_.emplace(handle.value, texture);
            return handle;
        }
        std::unordered_map<std::uint64_t, ::Texture2D> textures_;
        std::uint64_t next_texture_handle_ = 1;
        /// SDF 抗锯齿着色器与 1x1 白纹理（懒加载）。
        ::Shader aa_shader_ {};
        ::Texture2D aa_white_tex_ {};
        int aa_loc_shape_rect_ = -1;
        int aa_loc_quad_rect_ = -1;
        int aa_loc_radius_ = -1;
        int aa_loc_color_ = -1;
        int aa_loc_mode_ = -1;
        int aa_loc_a_ = -1;
        int aa_loc_b_ = -1;
        bool aa_ready_ = false;
    };

    auto make_raylib_device() -> std::unique_ptr<IRenderDevice> {
        return std::make_unique<RaylibRenderDevice>();
    }

} // namespace nandina::render
