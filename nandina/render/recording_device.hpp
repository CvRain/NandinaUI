//
// render/recording_device — 逐图元调用序列的录制装饰器（P4 的 trace 设备）。
//
// 为什么是**装饰器**而不是又一个后端：契约要的是「同一份组件代码在两个后端下的逐图元
// 等价性」。包装任意 `IRenderDevice` 就能把「用户代码调了什么」录下来，与内层设备无关，
// 历史上 raylib 设备与 SDL 设备由此能在**同一套调用序列格式**下对比 —— 而像素对照只能告诉我们
// 「看起来一样」，说不出「是不是用同样的图元画的」（例如圆角被画成方框时像素差异可能很小）。
//
// 因为它只是转发，包装不会改变渲染结果；代价是每次调用多一次虚函数跳转与一次字符串格式化，
// 所以只在取证运行里使用（探测的 `NANDINA_PROBE_TRACE`、bench 的 `NANDINA_BENCH_TRACE`）。

#ifndef NANDINA_EXPERIMENT_RENDER_RECORDING_DEVICE_HPP
#define NANDINA_EXPERIMENT_RENDER_RECORDING_DEVICE_HPP

#include "render_device.hpp"

#include <cstdint>
#include <iomanip>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace nandina::render
{
    class RecordingDevice final: public IRenderDevice {
    public:
        explicit RecordingDevice(std::unique_ptr<IRenderDevice> inner): inner_(std::move(inner)) {}

        /// 录制下来的调用序列（一行一次调用，参数用定点小数，便于跨后端逐字符对比）。
        [[nodiscard]] auto calls() const -> const std::vector<std::string>& {
            return calls_;
        }
        [[nodiscard]] auto call_count() const -> std::size_t {
            return calls_.size();
        }
        /// 按调用名统计次数（用于「两个后端的图元构成是否一致」这种粗对照）。
        [[nodiscard]] auto histogram() const -> std::vector<std::pair<std::string, std::size_t>>;

        void dump(std::ostream& out) const {
            for (const auto& line: calls_) {
                out << line << '\n';
            }
        }

        [[nodiscard]] auto inner() -> IRenderDevice& {
            return *inner_;
        }

        // ---- frame boundaries ----
        void begin_frame() override {
            record("begin_frame");
            inner_->begin_frame();
        }
        void end_frame() override {
            record("end_frame");
            inner_->end_frame();
        }
        void clear(const foundation::NanColor& color) override {
            record("clear", color);
            inner_->clear(color);
        }

        // ---- clipping ----
        void set_clip(const foundation::NanRect& screen_rect) override {
            record("set_clip", screen_rect);
            inner_->set_clip(screen_rect);
        }
        void clear_clip() override {
            record("clear_clip");
            inner_->clear_clip();
        }

        // ---- primitives ----
        void
        draw_rect(const foundation::NanRect& rect, const foundation::NanColor& color) override {
            record("draw_rect", rect, color);
            inner_->draw_rect(rect, color);
        }
        void draw_rect_outline(
            const foundation::NanRect& rect,
            const float thickness,
            const foundation::NanColor& color
        ) override {
            record("draw_rect_outline", rect, thickness, color);
            inner_->draw_rect_outline(rect, thickness, color);
        }
        void draw_rounded_rect(
            const foundation::NanRect& rect,
            const float radius,
            const foundation::NanColor& color
        ) override {
            record("draw_rounded_rect", rect, radius, color);
            inner_->draw_rounded_rect(rect, radius, color);
        }
        void draw_rounded_rect_outline(
            const foundation::NanRect& rect,
            const float radius,
            const float thickness,
            const foundation::NanColor& color
        ) override {
            record("draw_rounded_rect_outline", rect, radius, thickness, color);
            inner_->draw_rounded_rect_outline(rect, radius, thickness, color);
        }
        void draw_line(
            const foundation::NanPoint& a,
            const foundation::NanPoint& b,
            const float thickness,
            const foundation::NanColor& color
        ) override {
            record("draw_line", a, b, thickness, color);
            inner_->draw_line(a, b, thickness, color);
        }
        void draw_circle(
            const foundation::NanPoint& center,
            const float radius,
            const foundation::NanColor& color
        ) override {
            record("draw_circle", center, radius, color);
            inner_->draw_circle(center, radius, color);
        }
        void draw_circle_clipped_rounded_rect(
            const foundation::NanPoint& center,
            const float circle_radius,
            const foundation::NanRect& clip_rect,
            const float clip_radius,
            const foundation::NanColor& color
        ) override {
            record(
                "draw_circle_clipped_rounded_rect",
                center,
                circle_radius,
                clip_rect,
                clip_radius,
                color
            );
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
            const float inner_radius,
            const float outer_radius,
            const float start_radians,
            const float sweep_radians,
            const foundation::NanColor& color
        ) override {
            record(
                "draw_arc",
                center,
                inner_radius,
                outer_radius,
                start_radians,
                sweep_radians,
                color
            );
            inner_
                ->draw_arc(center, inner_radius, outer_radius, start_radians, sweep_radians, color);
        }
        void draw_rounded_rect_shadow(
            const foundation::NanRect& rect,
            const float radius,
            const float spread,
            const foundation::NanColor& color
        ) override {
            record("draw_rounded_rect_shadow", rect, radius, spread, color);
            inner_->draw_rounded_rect_shadow(rect, radius, spread, color);
        }
        void draw_text(
            const std::string_view text,
            const foundation::NanPoint& pos,
            const float font_size,
            const foundation::NanColor& color
        ) override {
            // 文本内容本身不进 trace（字形由 alpha 纹理与纹理区域绘制承载），否则不同后端
            // 的字体回退差异会淹没图元层的对照。只记长度，用于确认调用次数一致。
            record("draw_text", pos, font_size, color, text.size());
            inner_->draw_text(text, pos, font_size, color);
        }

        // ---- alpha textures ----
        [[nodiscard]] auto supports_alpha_textures() const -> bool override {
            return inner_->supports_alpha_textures();
        }
        [[nodiscard]] auto create_alpha_texture(
            const int width,
            const int height,
            const std::span<const std::uint8_t> alpha
        ) -> TextureHandle override {
            const auto handle = inner_->create_alpha_texture(width, height, alpha);
            record("create_alpha_texture", width, height, alpha.size(), handle.value);
            return handle;
        }
        void update_alpha_texture(
            const TextureHandle texture,
            const int width,
            const int height,
            const std::span<const std::uint8_t> alpha
        ) override {
            record("update_alpha_texture", texture.value, width, height, alpha.size());
            inner_->update_alpha_texture(texture, width, height, alpha);
        }
        void update_alpha_texture_region(
            const TextureHandle texture,
            const int full_width,
            const int full_height,
            const foundation::NanRect& region,
            const std::span<const std::uint8_t> alpha
        ) override {
            record(
                "update_alpha_texture_region",
                texture.value,
                full_width,
                full_height,
                region,
                alpha.size()
            );
            inner_->update_alpha_texture_region(texture, full_width, full_height, region, alpha);
        }
        void destroy_texture(const TextureHandle texture) override {
            record("destroy_texture", texture.value);
            inner_->destroy_texture(texture);
        }
        [[nodiscard]] auto create_rgba_texture(
            const int width,
            const int height,
            const std::span<const std::uint8_t> rgba
        ) -> TextureHandle override {
            const auto handle = inner_->create_rgba_texture(width, height, rgba);
            record("create_rgba_texture", width, height, rgba.size(), handle.value);
            return handle;
        }
        void draw_texture_region(
            const TextureHandle texture,
            const foundation::NanRect& source,
            const foundation::NanRect& destination,
            const foundation::NanColor& tint
        ) override {
            record("draw_texture_region", texture.value, source, destination, tint);
            inner_->draw_texture_region(texture, source, destination, tint);
        }
        [[nodiscard]] auto
        load_texture_from_file(std::string_view path, const ImageLoadOptions& options = {})
            -> TextureHandle override {
            const auto handle = inner_->load_texture_from_file(path, options);
            record("load_texture_from_file", path.size(), handle.value);
            return handle;
        }
        [[nodiscard]] auto load_texture_from_memory(
            const std::span<const std::uint8_t> bytes,
            const std::string_view media_type,
            const ImageLoadOptions& options = {}
        ) -> TextureHandle override {
            const auto handle = inner_->load_texture_from_memory(bytes, media_type, options);
            record("load_texture_from_memory", bytes.size(), media_type.size(), handle.value);
            return handle;
        }
        [[nodiscard]] auto texture_size(const TextureHandle texture)
            -> foundation::NanSize override {
            return inner_->texture_size(texture);
        }

        // ---- capabilities：一律如实转发，否则 trace 运行会掩盖真实的降级路径 ----
        [[nodiscard]] auto supports_rounded_rect() const -> bool override {
            return inner_->supports_rounded_rect();
        }
        [[nodiscard]] auto supports_rounded_clip() const -> bool override {
            return inner_->supports_rounded_clip();
        }
        [[nodiscard]] auto supports_soft_shadows() const -> bool override {
            return inner_->supports_soft_shadows();
        }
        [[nodiscard]] auto supports_direct_image_loading() const -> bool override {
            return inner_->supports_direct_image_loading();
        }
        [[nodiscard]] auto supports_rgba_texture_upload() const -> bool override {
            return inner_->supports_rgba_texture_upload();
        }

    private:
        void record(std::string_view name) {
            calls_.emplace_back(name);
        }

        template<typename... Args>
        void record(const std::string_view name, const Args&... args) {
            std::ostringstream line;
            line << name;
            (append(line, args), ...);
            calls_.push_back(line.str());
        }

        static void append(std::ostringstream& line, const float value) {
            line << ' ' << std::fixed << std::setprecision(3) << value;
        }
        static void append(std::ostringstream& line, const int value) {
            line << ' ' << value;
        }
        // size_t 与 uint64_t 在同一平台上可能是同一类型，只保留一个无符号重载。
        static void append(std::ostringstream& line, const std::uint64_t value) {
            line << ' ' << value;
        }
        static void append(std::ostringstream& line, const std::string_view value) {
            line << ' ' << value;
        }
        static void append(std::ostringstream& line, const foundation::NanPoint& point) {
            append(line, point.get_x());
            append(line, point.get_y());
        }
        static void append(std::ostringstream& line, const foundation::NanRect& rect) {
            append(line, rect.get_left());
            append(line, rect.get_top());
            append(line, rect.get_width());
            append(line, rect.get_height());
        }
        static void append(std::ostringstream& line, const foundation::NanColor& color) {
            // 记框架自己的 OKLCH 分量而不是 sRGB 8 位：两个后端拿到的是**同一个**颜色对象
            // （调用方相同），所以定点小数下逐字符可比；转成 8 位反而会掩盖真实差异。
            const auto value = color.oklch();
            append(line, value.light);
            append(line, value.chroma);
            append(line, value.hue);
            append(line, color.alpha());
        }

        std::unique_ptr<IRenderDevice> inner_;
        std::vector<std::string> calls_;
    };
} // namespace nandina::render

#endif // NANDINA_EXPERIMENT_RENDER_RECORDING_DEVICE_HPP
