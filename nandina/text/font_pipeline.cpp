#include "font_pipeline.hpp"

#include <limits>
#include <stdexcept>

namespace nandina::text
{
    FontPipeline::FontPipeline(
        render::IRenderDevice& device,
        ResolvedFontFamily family,
        const FontPipelineOptions options
    ):
        faces_(std::move(family.faces)) {
        if (faces_.empty() || options.atlas_width <= 0 || options.atlas_height <= 0
            || options.atlas_padding < 0)
        {
            throw std::invalid_argument("FontPipeline requires faces and valid atlas dimensions");
        }
        backend_ = std::make_unique<HarfBuzzTextLayoutBackend>(
            faces_.front(),
            std::vector<std::shared_ptr<FreeTypeFontFace>>(faces_.begin() + 1, faces_.end())
        );
        atlases_.reserve(faces_.size());
        textures_.reserve(faces_.size());
        bindings_.reserve(faces_.size());
        for (const auto& face: faces_) {
            auto atlas = std::make_unique<GlyphAtlas>(
                face,
                options.atlas_width,
                options.atlas_height,
                options.atlas_padding
            );
            auto texture = std::make_unique<GlyphAtlasTexture>(device, *atlas);
            bindings_.push_back({.atlas = atlas.get(), .texture = texture.get()});
            atlases_.push_back(std::move(atlas));
            textures_.push_back(std::move(texture));
        }
        renderer_ = std::make_unique<GlyphRunRenderer>(*backend_, bindings_);
    }

    auto FontPipeline::pipeline() const -> widget::primitives::TextPipeline {
        return {.backend = backend_.get(), .renderer = renderer_.get()};
    }
    auto FontPipeline::backend() const -> const HarfBuzzTextLayoutBackend& {
        return *backend_;
    }
    auto FontPipeline::font_count() const -> std::size_t {
        return faces_.size();
    }

    FontPipelineCache::FontPipelineCache(
        render::IRenderDevice& device,
        FontLoader& loader,
        const FontFamilyRegistry& families,
        const FontPipelineCacheLimits limits
    ):
        device_(&device),
        loader_(&loader),
        families_(&families),
        limits_(limits) {}

    auto FontPipelineCache::get(FontRequest request, const FontPipelineOptions options)
        -> FontResult<std::shared_ptr<FontPipeline>> {
        const Key key {
            .family = request.family,
            .weight = request.weight,
            .slant = request.slant,
            .options = options,
        };
        std::lock_guard lock(mutex_);
        prune_expired();
        if (const auto found = cache_.find(key); found != cache_.end()) {
            if (auto cached = found->second.lock()) {
                retain(key, cached, options);
                return cached;
            }
        }
        auto family = families_->resolve(request, *loader_);
        if (!family) {
            return std::unexpected(family.error());
        }
        try {
            auto pipeline = std::make_shared<FontPipeline>(*device_, std::move(*family), options);
            cache_[key] = pipeline;
            retain(key, pipeline, options);
            return pipeline;
        }
        catch (const std::exception& exception) {
            return std::unexpected(
                FontError {
                    .code = FontErrorCode::pipeline_failure,
                    .operation = "font.pipeline",
                    .message = exception.what(),
                }
            );
        }
    }
    void FontPipelineCache::clear() {
        std::lock_guard lock(mutex_);
        cache_.clear();
        retained_.clear();
        retained_bytes_ = 0;
    }

    auto FontPipelineCache::retained_pipeline_count() const -> std::size_t {
        std::lock_guard lock(mutex_);
        return retained_.size();
    }

    auto FontPipelineCache::retained_bytes() const -> std::size_t {
        std::lock_guard lock(mutex_);
        return retained_bytes_;
    }

    void FontPipelineCache::retain(
        const Key& key,
        const std::shared_ptr<FontPipeline>& pipeline,
        const FontPipelineOptions options
    ) {
        for (auto it = retained_.begin(); it != retained_.end(); ++it) {
            if (it->key == key) {
                retained_bytes_ -= it->estimated_bytes;
                retained_.erase(it);
                break;
            }
        }
        const auto bytes = estimate_bytes(*pipeline, options);
        if (retained_bytes_ > std::numeric_limits<std::size_t>::max() - bytes) {
            retained_bytes_ = std::numeric_limits<std::size_t>::max();
        }
        else {
            retained_bytes_ += bytes;
        }
        retained_.push_front(
            RetainedPipeline {.key = key, .pipeline = pipeline, .estimated_bytes = bytes}
        );
        trim();
    }

    void FontPipelineCache::trim() {
        while (!retained_.empty()
               && (retained_.size() > limits_.max_retained_pipelines
                   || retained_bytes_ > limits_.max_retained_bytes))
        {
            const auto bytes = retained_.back().estimated_bytes;
            retained_bytes_ = retained_bytes_ >= bytes ? retained_bytes_ - bytes : 0;
            retained_.pop_back();
        }
        prune_expired();
    }

    void FontPipelineCache::prune_expired() {
        for (auto it = cache_.begin(); it != cache_.end();) {
            if (it->second.expired()) {
                it = cache_.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    auto FontPipelineCache::estimate_bytes(
        const FontPipeline& pipeline,
        const FontPipelineOptions options
    ) -> std::size_t {
        constexpr std::size_t raylib_bytes_per_atlas_pixel = 5;
        const auto width = static_cast<std::size_t>(options.atlas_width);
        const auto height = static_cast<std::size_t>(options.atlas_height);
        const auto faces = pipeline.font_count();
        if (width == 0 || height == 0 || faces == 0) {
            return 0;
        }
        constexpr auto maximum = std::numeric_limits<std::size_t>::max();
        if (width > maximum / height) {
            return maximum;
        }
        const auto pixels = width * height;
        if (pixels > maximum / raylib_bytes_per_atlas_pixel) {
            return maximum;
        }
        const auto bytes_per_face = pixels * raylib_bytes_per_atlas_pixel;
        return faces > maximum / bytes_per_face ? maximum : faces * bytes_per_face;
    }
} // namespace nandina::text
