#ifndef NANDINA_EXPERIMENT_TEXT_FONT_PIPELINE_HPP
#define NANDINA_EXPERIMENT_TEXT_FONT_PIPELINE_HPP

#include "font_family.hpp"
#include "glyph_atlas.hpp"
#include "glyph_run_renderer.hpp"
#include "text_layout_backend.hpp"

#include <list>
#include <map>
#include <mutex>

namespace nandina::text
{
    struct FontPipelineOptions {
        /// 图集的**起始**尺寸。写满后 `GlyphAtlas::grow_to_fit()` 会按需翻倍到
        /// `GlyphAtlas::kMaxDimension`，所以这里唯一的作用是「不要预先占一大片空图集」。
        ///
        /// 1024² 起步时，一条管线每个 face 立刻占 1 MB CPU alpha + 4 MB RGBA8 纹理
        /// （SDL3 后端把 alpha 展开成 RGBA8）；showcase 的 Label 页用到 4 条管线 ×
        /// 2 个 face，光这一项约 8 MB + 32 MB，而实际用到的字形只填满几百 KB。
        /// 256² 起步时同样的页面约 0.5 MB + 2 MB，需要更多时再增长。
        /// 见 docs/references/text_shaping_cost.md §7.2。
        int atlas_width = 256;
        int atlas_height = 256;
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

        [[nodiscard]] auto pipeline() const -> TextPipeline;
        [[nodiscard]] auto backend() const -> const HarfBuzzTextLayoutBackend&;
        [[nodiscard]] auto font_count() const -> std::size_t;
        /// 所有 font face 图集当前的像素总数（图集写满后会扩容，所以这不是配置值）。
        [[nodiscard]] auto atlas_pixel_count() const -> std::size_t;

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

        [[nodiscard]] auto get(theme::FontRequest request, FontPipelineOptions options = {})
            -> FontResult<std::shared_ptr<FontPipeline>>;
        void clear();
        [[nodiscard]] auto retained_pipeline_count() const -> std::size_t;
        [[nodiscard]] auto retained_bytes() const -> std::size_t;

    private:
        struct Key {
            std::optional<resource::ResourceKey> family;
            int weight = 400;
            theme::FontSlant slant = theme::FontSlant::normal;
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
        [[nodiscard]] static auto
        estimate_bytes(const FontPipeline& pipeline, FontPipelineOptions options) -> std::size_t;

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
