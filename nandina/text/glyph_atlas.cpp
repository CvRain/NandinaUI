//
// text/glyph_atlas — CPU glyph packing and render-device texture bridge.
//

#include "glyph_atlas.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <utility>

namespace nandina::text
{

    GlyphAtlas::GlyphAtlas(
        std::shared_ptr<FreeTypeFontFace> face,
        int width,
        int height,
        int padding
    ):
        face_(std::move(face)),
        width_(width),
        height_(height),
        padding_(padding) {
        if (!face_) {
            throw std::invalid_argument("GlyphAtlas requires a font face");
        }
        if (width_ <= 0 || height_ <= 0 || padding_ < 0) {
            throw std::invalid_argument("GlyphAtlas dimensions and padding are invalid");
        }
        pixels_.resize(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_), 0);
    }

    auto GlyphAtlas::cache(char32_t codepoint, float pixel_size) -> const GlyphAtlasEntry& {
        const auto glyph_index = face_->glyph_index(codepoint);
        const auto key = key_for(glyph_index, pixel_size);
        if (const auto found = entries_.find(key); found != entries_.end()) {
            return found->second;
        }

        const auto& entry = cache_glyph(glyph_index, pixel_size);
        entries_.find(key)->second.codepoint = codepoint;
        return entry;
    }

    auto GlyphAtlas::cache_glyph(std::uint32_t glyph_index, float pixel_size)
        -> const GlyphAtlasEntry& {
        const auto key = key_for(glyph_index, pixel_size);
        if (const auto found = entries_.find(key); found != entries_.end()) {
            return found->second;
        }

        const auto bitmap = face_->rasterize_glyph(glyph_index, static_cast<float>(key.pixel_size));
        auto bounds = foundation::NanRect::empty();
        if (bitmap.width > 0 && bitmap.height > 0) {
            bounds = allocate(bitmap.width, bitmap.height);
            const int target_x = static_cast<int>(bounds.get_left());
            const int target_y = static_cast<int>(bounds.get_top());
            for (int row = 0; row < bitmap.height; ++row) {
                const auto source_offset = static_cast<std::size_t>(row * bitmap.pitch);
                const auto target_offset =
                    static_cast<std::size_t>((target_y + row) * width_ + target_x);
                std::copy_n(
                    bitmap.alpha.begin() + static_cast<std::ptrdiff_t>(source_offset),
                    bitmap.width,
                    pixels_.begin() + static_cast<std::ptrdiff_t>(target_offset)
                );
            }
        }

        auto [inserted, created] = entries_.emplace(
            key,
            GlyphAtlasEntry {
                .codepoint = 0,
                .pixel_size = static_cast<float>(key.pixel_size),
                .metrics = bitmap.metrics,
                .pixel_bounds = bounds,
            }
        );
        (void)created;
        ++revision_;
        return inserted->second;
    }

    auto GlyphAtlas::find(char32_t codepoint, float pixel_size) const -> const GlyphAtlasEntry* {
        return find_glyph(face_->glyph_index(codepoint), pixel_size);
    }

    auto GlyphAtlas::find_glyph(std::uint32_t glyph_index, float pixel_size) const
        -> const GlyphAtlasEntry* {
        const auto found = entries_.find(key_for(glyph_index, pixel_size));
        return found != entries_.end() ? &found->second : nullptr;
    }

    auto GlyphAtlas::width() const -> int {
        return width_;
    }

    auto GlyphAtlas::height() const -> int {
        return height_;
    }

    auto GlyphAtlas::revision() const -> std::uint64_t {
        return revision_;
    }

    auto GlyphAtlas::pixels() const -> std::span<const std::uint8_t> {
        return pixels_;
    }

    auto GlyphAtlas::face() const -> const FreeTypeFontFace& {
        return *face_;
    }

    auto GlyphAtlas::KeyHash::operator()(const Key& key) const noexcept -> std::size_t {
        const auto first = std::hash<std::uint32_t> {}(key.glyph_index);
        const auto second = std::hash<std::uint32_t> {}(key.pixel_size);
        return first ^ (second + 0x9E3779B9U + (first << 6U) + (first >> 2U));
    }

    auto GlyphAtlas::key_for(std::uint32_t glyph_index, float pixel_size) -> Key {
        return {
            .glyph_index = glyph_index,
            .pixel_size = static_cast<std::uint32_t>(std::max(1.0F, std::round(pixel_size))),
        };
    }

    auto GlyphAtlas::allocate(int width, int height) -> foundation::NanRect {
        if (cursor_x_ > 0 && cursor_x_ + width > width_) {
            cursor_x_ = 0;
            cursor_y_ += row_height_ + padding_;
            row_height_ = 0;
        }
        if (width > width_ || cursor_y_ + height > height_) {
            throw std::runtime_error("GlyphAtlas is full");
        }

        const auto bounds = foundation::NanRect::from_xywh(
            static_cast<float>(cursor_x_),
            static_cast<float>(cursor_y_),
            static_cast<float>(width),
            static_cast<float>(height)
        );
        cursor_x_ += width + padding_;
        row_height_ = std::max(row_height_, height);
        return bounds;
    }

    GlyphAtlasTexture::GlyphAtlasTexture(render::IRenderDevice& device, GlyphAtlas& atlas):
        device_(device),
        atlas_(atlas) {
        if (!device_.supports_alpha_textures()) {
            throw std::runtime_error("Render device does not support alpha textures");
        }
        texture_ = device_.create_alpha_texture(atlas_.width(), atlas_.height(), atlas_.pixels());
        if (!texture_) {
            throw std::runtime_error("Failed to create glyph atlas texture");
        }
        uploaded_revision_ = atlas_.revision();
    }

    GlyphAtlasTexture::~GlyphAtlasTexture() {
        device_.destroy_texture(texture_);
    }

    void GlyphAtlasTexture::sync() {
        if (uploaded_revision_ == atlas_.revision()) {
            return;
        }
        device_.update_alpha_texture(texture_, atlas_.width(), atlas_.height(), atlas_.pixels());
        uploaded_revision_ = atlas_.revision();
    }

    void GlyphAtlasTexture::draw(
        const GlyphAtlasEntry& glyph,
        foundation::NanPoint baseline_origin,
        foundation::NanColor color,
        const float screen_to_physical
    ) {
        if (!glyph.pixel_bounds.is_valid()) {
            return;
        }
        if (!std::isfinite(screen_to_physical) || screen_to_physical <= 0.0F) {
            throw std::invalid_argument(
                "GlyphAtlasTexture screen-to-physical scale must be finite and positive"
            );
        }
        sync();
        // FreeType 已在整像素网格生成 grayscale-AA bitmap。HarfBuzz 的 advance、
        // kerning 与 caret 仍保留亚像素精度，但最终 bitmap 左上角吸附到像素，
        // 避免 GPU 再把一张已抗锯齿的字形分摊到相邻像素。
        // 字形 bitmap 和 bearing 使用物理像素；目标矩形使用 screen-space 单位。
        // 先在物理像素网格吸附，再除以 DPI，可让 2x framebuffer 得到 0.5
        // screen-unit 的精确位置，而不是把高分屏字形错误放大两次。
        const float physical_baseline_x = baseline_origin.get_x() * screen_to_physical;
        const float physical_baseline_y = baseline_origin.get_y() * screen_to_physical;
        const float destination_x =
            std::round(physical_baseline_x + glyph.metrics.bearing_x) / screen_to_physical;
        const float destination_y =
            std::round(physical_baseline_y - glyph.metrics.bearing_y) / screen_to_physical;
        const auto destination = foundation::NanRect::from_xywh(
            destination_x,
            destination_y,
            glyph.pixel_bounds.get_width() / screen_to_physical,
            glyph.pixel_bounds.get_height() / screen_to_physical
        );
        device_.draw_texture_region(texture_, glyph.pixel_bounds, destination, color);
    }

    auto GlyphAtlasTexture::handle() const -> render::TextureHandle {
        return texture_;
    }

    auto GlyphAtlasTexture::uploaded_revision() const -> std::uint64_t {
        return uploaded_revision_;
    }

    auto GlyphAtlasTexture::atlas() const -> const GlyphAtlas& {
        return atlas_;
    }

} // namespace nandina::text
