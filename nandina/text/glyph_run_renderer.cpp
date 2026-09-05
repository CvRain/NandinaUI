//
// text/glyph_run_renderer — draw shaped layout lines through a glyph atlas.
//

#include "glyph_run_renderer.hpp"

#include "../render/draw_context.hpp"

#include <cmath>
#include <stdexcept>
#include <vector>

namespace nandina::text
{

    GlyphRunRenderer::GlyphRunRenderer(GlyphAtlas& atlas, GlyphAtlasTexture& texture):
        bindings_({GlyphAtlasBinding {.atlas = &atlas, .texture = &texture}}) {
        if (&texture.atlas() != &atlas) {
            throw std::invalid_argument("GlyphRunRenderer atlas binding does not match texture");
        }
    }

    GlyphRunRenderer::GlyphRunRenderer(
        const HarfBuzzTextLayoutBackend& backend,
        std::span<const GlyphAtlasBinding> bindings
    ):
        bindings_(bindings.begin(), bindings.end()) {
        if (bindings_.empty()) {
            throw std::invalid_argument("GlyphRunRenderer requires at least one atlas binding");
        }
        if (bindings_.size() != backend.font_count()) {
            throw std::invalid_argument(
                "GlyphRunRenderer bindings do not cover backend font slots"
            );
        }
        for (std::size_t index = 0; index < bindings_.size(); ++index) {
            const auto& binding = bindings_[index];
            if (binding.atlas == nullptr || binding.texture == nullptr) {
                throw std::invalid_argument("GlyphRunRenderer atlas binding is incomplete");
            }
            if (&binding.texture->atlas() != binding.atlas) {
                throw std::invalid_argument(
                    "GlyphRunRenderer atlas binding does not match texture"
                );
            }
            if (&binding.atlas->face() != backend.font_face(index).get()) {
                throw std::invalid_argument(
                    "GlyphRunRenderer atlas face does not match backend slot"
                );
            }
        }
    }

    void GlyphRunRenderer::draw(
        const widget::primitives::TextLayoutResult& layout,
        render::DrawContext& context,
        foundation::NanPoint position,
        foundation::NanColor color
    ) {
        const auto scale = context.scale();
        float line_top = position.get_y();
        for (const auto& line: layout.lines) {
            draw_line(
                line,
                foundation::NanPoint(
                    position.get_x(),
                    line_top + context.logical_to_screen(line.baseline)
                ),
                color,
                layout.font_size,
                scale.logical_to_screen,
                scale.screen_to_physical
            );
            line_top += context.logical_to_screen(line.size.get_height());
        }
    }

    void GlyphRunRenderer::draw_line(
        const widget::primitives::TextLayoutLine& line,
        foundation::NanPoint baseline_origin,
        foundation::NanColor color,
        const float logical_pixel_size,
        const float logical_to_screen,
        const float screen_to_physical
    ) {
        if (!std::isfinite(logical_to_screen) || logical_to_screen <= 0.0F
            || !std::isfinite(screen_to_physical) || screen_to_physical <= 0.0F)
        {
            throw std::invalid_argument("GlyphRunRenderer scales must be finite and positive");
        }
        const float raster_pixel_size =
            logical_pixel_size * logical_to_screen * screen_to_physical;
        struct CachedGlyph {
            std::size_t binding = 0;
            const GlyphAtlasEntry* entry = nullptr;
        };

        std::vector<CachedGlyph> glyphs;
        glyphs.reserve(line.glyphs.size());
        std::vector<bool> used_bindings(bindings_.size(), false);
        for (const auto& shaped: line.glyphs) {
            if (shaped.font_index >= bindings_.size()) {
                throw std::out_of_range("Glyph font index has no atlas binding");
            }
            auto& binding = bindings_[shaped.font_index];
            glyphs.push_back(
                CachedGlyph {
                    .binding = shaped.font_index,
                    .entry = &binding.atlas->cache_glyph(
                        shaped.glyph_index,
                        raster_pixel_size
                    ),
                }
            );
            used_bindings[shaped.font_index] = true;
        }
        for (std::size_t index = 0; index < bindings_.size(); ++index) {
            if (used_bindings[index]) {
                bindings_[index].texture->sync();
            }
        }

        float pen_x = baseline_origin.get_x();
        float pen_y = baseline_origin.get_y();
        for (std::size_t index = 0; index < line.glyphs.size(); ++index) {
            const auto& shaped = line.glyphs[index];
            const auto& cached = glyphs[index];
            bindings_[cached.binding].texture->draw(
                *cached.entry,
                foundation::NanPoint(
                    pen_x + shaped.x_offset * logical_to_screen,
                    pen_y - shaped.y_offset * logical_to_screen
                ),
                color,
                screen_to_physical
            );
            pen_x += shaped.x_advance * logical_to_screen;
            pen_y -= shaped.y_advance * logical_to_screen;
        }
    }

} // namespace nandina::text
