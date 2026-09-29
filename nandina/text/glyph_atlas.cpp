//
// text/glyph_atlas — CPU glyph packing and render-device texture bridge.
//

#include "glyph_atlas.hpp"

#include "../foundation/nan_logger.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <new>
#include <stdexcept>
#include <utility>

namespace nandina::text
{

    GlyphAtlas::GlyphAtlas(
        std::shared_ptr<FreeTypeFontFace> face,
        int width,
        int height,
        int padding,
        int max_dimension
    ):
        face_(std::move(face)),
        width_(width),
        height_(height),
        padding_(padding),
        max_dimension_(std::max(1, max_dimension)) {
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
        }
        // 只有真的分配到了空间才拷贝像素。allocate() 在放不下时会返回无效矩形
        // （而不是抛异常），此时若照着 (0,0) 拷贝就会写穿图集缓冲。
        if (bounds.is_valid()) {
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

    auto GlyphAtlas::dirty_bounds() const -> foundation::NanRect {
        if (!dirty_) {
            return foundation::NanRect::empty();
        }
        return foundation::NanRect::from_xywh(
            static_cast<float>(dirty_left_),
            static_cast<float>(dirty_top_),
            static_cast<float>(dirty_right_ - dirty_left_),
            static_cast<float>(dirty_bottom_ - dirty_top_)
        );
    }

    auto GlyphAtlas::consume_dirty_bounds() -> foundation::NanRect {
        const auto bounds = dirty_bounds();
        dirty_ = false;
        return bounds;
    }

    void GlyphAtlas::mark_dirty(const foundation::NanRect& bounds) {
        if (!bounds.is_valid()) {
            return;
        }
        const int left = static_cast<int>(bounds.get_left());
        const int top = static_cast<int>(bounds.get_top());
        const int right = static_cast<int>(bounds.get_right());
        const int bottom = static_cast<int>(bounds.get_bottom());
        if (!dirty_) {
            dirty_ = true;
            dirty_left_ = left;
            dirty_top_ = top;
            dirty_right_ = right;
            dirty_bottom_ = bottom;
            return;
        }
        dirty_left_ = std::min(dirty_left_, left);
        dirty_top_ = std::min(dirty_top_, top);
        dirty_right_ = std::max(dirty_right_, right);
        dirty_bottom_ = std::max(dirty_bottom_, bottom);
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
        if (width <= 0 || height <= 0) {
            return foundation::NanRect::empty();
        }

        // 先按当前尺寸模拟一次换行，判断现有空间够不够。
        int probe_x = cursor_x_;
        int probe_y = cursor_y_;
        if (probe_x > 0 && probe_x + width > width_) {
            probe_x = 0;
            probe_y += row_height_ + padding_;
        }
        if (probe_x + width > width_ || probe_y + height > height_) {
            if (!grow_to_fit(
                    std::max(width, probe_x + width),
                    std::max(height, probe_y + height)
                ))
            {
                // 连扩容上限都放不下（例如字号大于整张图集）。
                //
                // 这里**不抛异常**：这条路径在渲染过程中（draw → cache_glyph →
                // allocate），抛出去会穿出 tick() 并终止进程。返回无效矩形即可，
                // GlyphAtlasTexture::draw 对无效 pixel_bounds 本来就会跳过，
                // 代价只是这一个字形不显示，而不是整个应用崩掉。
                log::error(
                    "GlyphAtlas: glyph {}x{} does not fit a {}x{} atlas; skipping it",
                    width,
                    height,
                    max_dimension_,
                    max_dimension_
                );
                return foundation::NanRect::empty();
            }
        }

        if (cursor_x_ > 0 && cursor_x_ + width > width_) {
            cursor_x_ = 0;
            cursor_y_ += row_height_ + padding_;
            row_height_ = 0;
        }
        if (cursor_x_ + width > width_ || cursor_y_ + height > height_) {
            // 扩容后仍放不下：理论上不可达，兜住而不是抛。
            log::error("GlyphAtlas: allocation still does not fit after growing; skipping glyph");
            return foundation::NanRect::empty();
        }

        const auto bounds = foundation::NanRect::from_xywh(
            static_cast<float>(cursor_x_),
            static_cast<float>(cursor_y_),
            static_cast<float>(width),
            static_cast<float>(height)
        );
        cursor_x_ += width + padding_;
        row_height_ = std::max(row_height_, height);
        // 只有真正被写入的区域才算脏：padding 从不写像素。
        mark_dirty(bounds);
        return bounds;
    }

    auto GlyphAtlas::grow_to_fit(const int required_width, const int required_height) -> bool {
        if (required_width > max_dimension_ || required_height > max_dimension_) {
            return false;
        }
        int next_width = width_;
        int next_height = height_;
        while (next_width < required_width || next_height < required_height) {
            if (next_width < required_width) {
                next_width = std::min(max_dimension_, next_width * 2);
            }
            if (next_height < required_height) {
                next_height = std::min(max_dimension_, next_height * 2);
            }
        }
        return resize(next_width, next_height);
    }

    auto GlyphAtlas::resize(const int width, const int height) -> bool {
        if (width == width_ && height == height_) {
            return true;
        }
        if (width < width_ || height < height_) {
            return false;
        }
        // 只加宽/加高、不重排：已缓存字形的 pixel_bounds 保持有效，只是行距变了。
        try {
            std::vector<std::uint8_t> grown(
                static_cast<std::size_t>(width) * static_cast<std::size_t>(height),
                0
            );
            for (int row = 0; row < height_; ++row) {
                const auto* source = pixels_.data() + static_cast<std::size_t>(row) * width_;
                std::copy_n(
                    source,
                    static_cast<std::size_t>(width_),
                    grown.data() + static_cast<std::size_t>(row) * width
                );
            }
            pixels_ = std::move(grown);
        }
        catch (const std::bad_alloc&) {
            // 同样不抛：让调用方按"放不下"处理，丢掉一个字形而不是崩掉。
            return false;
        }
        width_ = width;
        height_ = height;
        return true;
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
        uploaded_width_ = atlas_.width();
        uploaded_height_ = atlas_.height();
        // 构造时已经整张上传过一次，所以此前累积的脏区域不必再传。
        (void)atlas_.consume_dirty_bounds();
    }

    GlyphAtlasTexture::~GlyphAtlasTexture() {
        device_.destroy_texture(texture_);
    }

    void GlyphAtlasTexture::sync() {
        // 图集扩容后纹理尺寸必须跟着变，只能重建；这条路径很罕见（图集写满时
        // 才发生），而且重建后整张上传一次即可，不像以前每次同步都整张上传。
        if (atlas_.width() != uploaded_width_ || atlas_.height() != uploaded_height_) {
            rebuild();
            return;
        }
        if (uploaded_revision_ == atlas_.revision()) {
            return;
        }
        // 只上传自上次同步以来被写入的区域。整张图集重传一次是十毫秒量级的
        // 阻塞（转换 + GPU 上传），而一次布局会连续写入许多小字形；按脏矩形传
        // 的代价只跟新增字形的面积成正比。
        if (const auto region = atlas_.consume_dirty_bounds(); region.is_valid()) {
            device_.update_alpha_texture_region(
                texture_,
                atlas_.width(),
                atlas_.height(),
                region,
                atlas_.pixels()
            );
        }
        uploaded_revision_ = atlas_.revision();
    }

    void GlyphAtlasTexture::rebuild() {
        device_.destroy_texture(texture_);
        texture_ = device_.create_alpha_texture(atlas_.width(), atlas_.height(), atlas_.pixels());
        if (!texture_) {
            throw std::runtime_error("Failed to recreate glyph atlas texture");
        }
        uploaded_width_ = atlas_.width();
        uploaded_height_ = atlas_.height();
        uploaded_revision_ = atlas_.revision();
        (void)atlas_.consume_dirty_bounds();
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
