//
// text/glyph_run_renderer — draw shaped layout lines through a glyph atlas.
//

#ifndef NANDINA_EXPERIMENT_TEXT_GLYPH_RUN_RENDERER_HPP
#define NANDINA_EXPERIMENT_TEXT_GLYPH_RUN_RENDERER_HPP

#include "../widget/primitives/text_layout.hpp"
#include "../widget/primitives/text_layout_backend.hpp"
#include "glyph_atlas.hpp"
#include "harfbuzz_text_backend.hpp"

#include <span>
#include <vector>

namespace nandina::text
{

    struct GlyphAtlasBinding {
        GlyphAtlas* atlas = nullptr;
        GlyphAtlasTexture* texture = nullptr;
    };

    class GlyphRunRenderer final: public widget::primitives::ITextLayoutRenderer {
    public:
        GlyphRunRenderer(GlyphAtlas& atlas, GlyphAtlasTexture& texture);
        GlyphRunRenderer(
            const HarfBuzzTextLayoutBackend& backend,
            std::span<const GlyphAtlasBinding> bindings
        );

        void draw(
            const widget::primitives::TextLayoutResult& layout,
            render::DrawContext& context,
            foundation::NanPoint position,
            foundation::NanColor color
        ) override;

        void draw_line(
            const widget::primitives::TextLayoutLine& line,
            foundation::NanPoint baseline_origin,
            foundation::NanColor color,
            float logical_pixel_size,
            float logical_to_screen = 1.0F,
            float screen_to_physical = 1.0F
        );

    private:
        std::vector<GlyphAtlasBinding> bindings_;
    };

} // namespace nandina::text

#endif // NANDINA_EXPERIMENT_TEXT_GLYPH_RUN_RENDERER_HPP
