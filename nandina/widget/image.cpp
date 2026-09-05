//
// widget/image - textured image node implementation.
//

#include "image.hpp"

#include "../foundation/nan_logger.hpp"
#include "../render/draw_context.hpp"
#include "../render/texture_cache.hpp"
#include "../resource/resource_manager.hpp"
#include "../resource/resource_uri.hpp"

#include <algorithm>
#include <utility>

namespace nandina::widget
{
    Image::Image(std::string source): source_(std::move(source)) {}

    auto Image::create(std::string source) -> std::shared_ptr<Image> {
        return std::make_shared<Image>(std::move(source));
    }

    void Image::set_source(std::string path) {
        if (source_ == path) {
            return;
        }
        source_ = std::move(path);
        reset_load();
    }

    auto Image::source() const -> std::string_view {
        return source_;
    }

    void Image::set_resource_manager(resource::ResourceManager* resources) {
        if (resources_ == resources) {
            return;
        }
        resources_ = resources;
        reset_load();
    }

    auto Image::resource_manager() const noexcept -> resource::ResourceManager* {
        return resources_;
    }

    void Image::set_texture_cache(render::TextureCache* cache) {
        if (texture_cache_ == cache) {
            return;
        }
        texture_cache_ = cache;
        reset_load();
    }

    auto Image::texture_cache() const noexcept -> render::TextureCache* {
        return texture_cache_;
    }

    void Image::set_tint(const foundation::NanColor tint) {
        tint_ = tint;
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Image::tint() const -> foundation::NanColor {
        return tint_;
    }

    void Image::set_source_rect(const foundation::NanRect rect) {
        source_rect_ = rect;
        mark_dirty(scene::DirtyFlags::paint);
    }

    void Image::clear_source_rect() {
        source_rect_.reset();
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Image::source_rect() const -> const std::optional<foundation::NanRect>& {
        return source_rect_;
    }

    void Image::set_scale_mode(const ImageScale mode) {
        scale_mode_ = mode;
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Image::scale_mode() const -> ImageScale {
        return scale_mode_;
    }

    void Image::set_alignment(const ImageAlignment alignment) {
        alignment_ = alignment;
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Image::alignment() const -> ImageAlignment {
        return alignment_;
    }

    auto Image::natural_size() const -> foundation::NanSize {
        return natural_size_;
    }

    void Image::set_load_options(render::ImageLoadOptions options) {
        load_options_ = std::move(options);
        reset_load();
    }

    auto Image::load_options() const -> const render::ImageLoadOptions& {
        return load_options_;
    }

    void Image::set_load_mode(const ImageLoadMode mode) {
        if (load_mode_ == mode)
            return;
        load_mode_ = mode;
        reset_load();
    }

    auto Image::load_mode() const noexcept -> ImageLoadMode {
        return load_mode_;
    }

    auto Image::load_state() const noexcept -> ImageLoadState {
        return load_state_;
    }

    void Image::set_placeholder_color(const foundation::NanColor color) {
        placeholder_color_ = color;
        mark_dirty(scene::DirtyFlags::paint);
    }

    void Image::clear_placeholder_color() {
        placeholder_color_.reset();
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Image::placeholder_color() const -> const std::optional<foundation::NanColor>& {
        return placeholder_color_;
    }

    void Image::set_prefetch_distance(const float distance) {
        prefetch_distance_ = std::max(0.0F, distance);
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Image::prefetch_distance() const noexcept -> float {
        return prefetch_distance_;
    }

    auto Image::on_draw(render::DrawContext& ctx) -> void {
        const auto world = render::world_bounds_from_local(ctx.world_transform(), local_rect());
        const auto clip = ctx.clip().current();
        // C5.6b：视口门控——图片尚未加载且启用了预取时，只有世界矩形与当前 clip 视口
        // （外扩预取距离）相交才触发加载，否则保持 idle 只画占位。
        if (load_state_ == ImageLoadState::idle && prefetch_distance_ > 0.0F) {
            if (clip.is_valid() && !clip.expanded(prefetch_distance_).intersects(world)) {
                if (placeholder_color_) {
                    ctx.device().draw_rect(
                        world,
                        placeholder_color_->with_alpha(placeholder_color_->alpha() * ctx.opacity())
                    );
                }
                return;
            }
        }
        // C5.6：加载优先级 = 世界矩形到 clip 视口的轴对齐间距（相交为 0 = 可见）。
        int priority = 0;
        if (clip.is_valid() && !clip.intersects(world)) {
            const float dx = std::max(
                0.0F,
                std::max(world.get_left() - clip.get_right(), clip.get_left() - world.get_right())
            );
            const float dy = std::max(
                0.0F,
                std::max(world.get_top() - clip.get_bottom(), clip.get_top() - world.get_bottom())
            );
            priority = static_cast<int>(std::ceil(dx + dy));
        }
        ensure_loaded(ctx.device(), priority);
        if (!texture_) {
            if (placeholder_color_) {
                ctx.device().draw_rect(
                    world,
                    placeholder_color_->with_alpha(placeholder_color_->alpha() * ctx.opacity())
                );
            }
            return;
        }
        const auto rects = compute_rects(world);
        const auto tint = tint_.with_alpha(tint_.alpha() * ctx.opacity());
        ctx.device().draw_texture_region(texture_->handle(), rects.source, rects.destination, tint);
    }

    void Image::apply_texture_cache(render::TextureCache& cache) {
        set_texture_cache(&cache);
    }

    auto Image::on_measure(const scene::LayoutConstraints constraints) -> foundation::NanSize {
        if (natural_size_.get_width() > 0.0F || natural_size_.get_height() > 0.0F) {
            return constraints.constrain(natural_size_);
        }
        return constraints.constrain(size());
    }

    void Image::ensure_loaded(render::IRenderDevice& device, const int priority) {
        if (load_state_ != ImageLoadState::idle || source_.empty()) {
            return;
        }
        load_state_ = ImageLoadState::loading;
        if (source_.starts_with("res://")) {
            if (!resources_) {
                log::get("widget.image")
                    .warn("Image: resource manager unavailable for {}", source_);
                load_state_ = ImageLoadState::failed;
                return;
            }
            const auto uri = resource::ResourceUri::parse(source_);
            if (!uri || uri->scheme() != resource::ResourceUriScheme::res) {
                log::get("widget.image").warn("Image: invalid resource URI {}", source_);
                load_state_ = ImageLoadState::failed;
                return;
            }
            const auto key = uri->resource_key();
            if (!key) {
                log::get("widget.image").warn("Image: resource URI has no key {}", source_);
                load_state_ = ImageLoadState::failed;
                return;
            }
            const bool wants_async = texture_cache_ != nullptr
                && load_mode_ != ImageLoadMode::synchronous
                && texture_cache_->supports_async_loading();
            if (wants_async) {
                // C5.5：整条 res:// 读取 + 解码在后台，UI 线程只上传；这里不再 require。
                const auto generation = load_generation_;
                const auto source = source_;
                const auto weak =
                    std::weak_ptr<Image>(std::static_pointer_cast<Image>(shared_from_this()));
                if (texture_cache_->load_resource_async(
                        *resources_,
                        *key,
                        load_options_,
                        [weak, generation, source](std::shared_ptr<render::CachedTexture> texture) {
                            if (const auto image = weak.lock()) {
                                image->complete_async(generation, source, std::move(texture));
                            }
                        },
                        priority
                    ))
                {
                    return;
                }
                // 后台提交失败：回退同步路径。
            }
            const auto loaded = resources_->require(*key);
            if (!loaded) {
                log::get("widget.image")
                    .warn(
                        "Image: resource lookup failed for {}: {}",
                        source_,
                        loaded.error().message
                    );
                load_state_ = ImageLoadState::failed;
                return;
            }
            if (!*loaded) {
                log::get("widget.image").warn("Image: resource not found for {}", source_);
                load_state_ = ImageLoadState::failed;
                return;
            }
            if (!(*loaded)->media_type().starts_with("image/")) {
                log::get("widget.image")
                    .warn(
                        "Image: resource is not an image {} ({})",
                        source_,
                        (*loaded)->media_type()
                    );
                load_state_ = ImageLoadState::failed;
                return;
            }
            if (texture_cache_ != nullptr) {
                texture_ = texture_cache_->load_memory(
                    (*loaded)->id().to_string(),
                    (*loaded)->bytes(),
                    (*loaded)->media_type(),
                    load_options_
                );
            }
            else {
                const auto handle = device.load_texture_from_memory(
                    (*loaded)->bytes(),
                    (*loaded)->media_type(),
                    load_options_
                );
                if (handle) {
                    texture_ = std::make_shared<render::CachedTexture>(
                        device,
                        handle,
                        device.texture_size(handle)
                    );
                }
            }
        }
        else {
            if (texture_cache_ != nullptr) {
                texture_ = texture_cache_->load_file(source_, load_options_);
            }
            else {
                const auto handle = device.load_texture_from_file(source_, load_options_);
                if (handle) {
                    texture_ = std::make_shared<render::CachedTexture>(
                        device,
                        handle,
                        device.texture_size(handle)
                    );
                }
            }
        }
        if (!texture_) {
            log::get("widget.image").warn("Image: texture decode/upload failed for {}", source_);
        }
        load_state_ = texture_ ? ImageLoadState::ready : ImageLoadState::failed;
        natural_size_ = texture_ != nullptr ? texture_->size() : foundation::NanSize {};
        if (natural_size_.get_width() > 0.0F || natural_size_.get_height() > 0.0F) {
            mark_layout_dirty();
        }
    }

    void Image::reset_load() {
        ++load_generation_;
        texture_.reset();
        natural_size_ = foundation::NanSize {};
        load_state_ = ImageLoadState::idle;
        mark_layout_dirty();
    }

    void Image::complete_async(
        const std::uint64_t generation,
        std::string source,
        std::shared_ptr<render::CachedTexture> texture
    ) {
        if (generation != load_generation_ || source != source_)
            return;
        texture_ = std::move(texture);
        load_state_ = texture_ ? ImageLoadState::ready : ImageLoadState::failed;
        natural_size_ = texture_ ? texture_->size() : foundation::NanSize {};
        if (!texture_) {
            log::get("widget.image").warn("Image: texture decode/upload failed for {}", source_);
        }
        if (natural_size_.get_width() > 0.0F || natural_size_.get_height() > 0.0F) {
            mark_layout_dirty();
        }
        else {
            mark_dirty(scene::DirtyFlags::paint);
        }
    }

    auto Image::compute_rects(const foundation::NanRect& world) const -> DrawRects {
        const foundation::NanRect source_base = source_rect_.value_or(
            foundation::NanRect::from_xywh(
                0.0F,
                0.0F,
                natural_size_.get_width(),
                natural_size_.get_height()
            )
        );
        const float sw = source_base.get_width();
        const float sh = source_base.get_height();
        const float dw = world.get_width();
        const float dh = world.get_height();
        if (sw <= 0.0F || sh <= 0.0F || dw <= 0.0F || dh <= 0.0F) {
            return DrawRects {source_base, world};
        }

        if (scale_mode_ == ImageScale::contain) {
            const float scale = std::min(dw / sw, dh / sh);
            const float drawn_w = sw * scale;
            const float drawn_h = sh * scale;
            float dx = world.get_left();
            float dy = world.get_top();
            switch (alignment_) {
                case ImageAlignment::start:
                    dx = world.get_left();
                    dy = world.get_top();
                    break;
                case ImageAlignment::end:
                    dx = world.get_right() - drawn_w;
                    dy = world.get_bottom() - drawn_h;
                    break;
                case ImageAlignment::center:
                default:
                    dx = world.get_left() + (dw - drawn_w) * 0.5F;
                    dy = world.get_top() + (dh - drawn_h) * 0.5F;
                    break;
            }
            return DrawRects {
                source_base,
                foundation::NanRect::from_xywh(dx, dy, drawn_w, drawn_h),
            };
        }

        if (scale_mode_ == ImageScale::cover) {
            const float src_ratio = sw / sh;
            const float dst_ratio = dw / dh;
            float crop_w = sw;
            float crop_h = sh;
            if (src_ratio > dst_ratio) {
                crop_w = sh * dst_ratio; // 源更宽：裁宽度
            }
            else {
                crop_h = sw / dst_ratio; // 源更高：裁高度
            }
            const float crop_x = source_base.get_left() + (sw - crop_w) * 0.5F;
            const float crop_y = source_base.get_top() + (sh - crop_h) * 0.5F;
            return DrawRects {
                foundation::NanRect::from_xywh(crop_x, crop_y, crop_w, crop_h),
                world,
            };
        }

        // stretch（默认）：整源拉伸到整目标。
        return DrawRects {source_base, world};
    }
} // namespace nandina::widget
