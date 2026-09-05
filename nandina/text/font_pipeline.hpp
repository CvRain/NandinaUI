#ifndef NANDINA_EXPERIMENT_TEXT_FONT_PIPELINE_HPP
#define NANDINA_EXPERIMENT_TEXT_FONT_PIPELINE_HPP

#include "font_family.hpp"
#include "glyph_atlas.hpp"
#include "glyph_run_renderer.hpp"

#include <list>
#include <map>
#include <mutex>

namespace nandina::text
{
    struct FontPipelineOptions {
        int atlas_width = 1024;
        int atlas_height = 1024;
        int atlas_padding = 1;
        auto operator<=>(const FontPipelineOptions&) const = default;
    };

    class FontPipeline final {
    public:
        FontPipeline(
            render::IRenderDevice& device,
            ResolvedFontFamily family,
            FontPipelineOptions options = {}
        );

        [[nodiscard]] auto pipeline() const -> widget::primitives::TextPipeline;
        [[nodiscard]] auto backend() const -> const HarfBuzzTextLayoutBackend&;
        [[nodiscard]] auto font_count() const -> std::size_t;

    private:
        std::vector<std::shared_ptr<FreeTypeFontFace>> faces_;
        std::unique_ptr<HarfBuzzTextLayoutBackend> backend_;
        std::vector<std::unique_ptr<GlyphAtlas>> atlases_;
        std::vector<std::unique_ptr<GlyphAtlasTexture>> textures_;
        std::vector<GlyphAtlasBinding> bindings_;
        std::unique_ptr<GlyphRunRenderer> renderer_;
    };

    struct FontPipelineCacheLimits {
        std::size_t max_retained_pipelines = 16;
        std::size_t max_retained_bytes = 128U * 1024U * 1024U;
    };

    class FontPipelineCache final {
    public:
        FontPipelineCache(
            render::IRenderDevice& device,
            FontLoader& loader,
            const FontFamilyRegistry& families,
            FontPipelineCacheLimits limits = {}
        );

        [[nodiscard]] auto get(FontRequest request, FontPipelineOptions options = {})
            -> FontResult<std::shared_ptr<FontPipeline>>;
        void clear();
        [[nodiscard]] auto retained_pipeline_count() const -> std::size_t;
        [[nodiscard]] auto retained_bytes() const -> std::size_t;

    private:
        struct Key {
            std::optional<resource::ResourceKey> family;
            int weight = 400;
            FontSlant slant = FontSlant::normal;
            FontPipelineOptions options;
            auto operator<=>(const Key&) const = default;
        };

        struct RetainedPipeline {
            Key key;
            std::shared_ptr<FontPipeline> pipeline;
            std::size_t estimated_bytes = 0;
        };

        void retain(
            const Key& key,
            const std::shared_ptr<FontPipeline>& pipeline,
            FontPipelineOptions options
        );
        void trim();
        void prune_expired();
        [[nodiscard]] static auto estimate_bytes(
            const FontPipeline& pipeline,
            FontPipelineOptions options
        ) -> std::size_t;

        render::IRenderDevice* device_;
        FontLoader* loader_;
        const FontFamilyRegistry* families_;
        FontPipelineCacheLimits limits_;
        mutable std::mutex mutex_;
        std::map<Key, std::weak_ptr<FontPipeline>> cache_;
        std::list<RetainedPipeline> retained_;
        std::size_t retained_bytes_ = 0;
    };
} // namespace nandina::text
#endif
