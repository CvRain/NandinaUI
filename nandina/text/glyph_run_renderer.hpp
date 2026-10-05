//
// text/glyph_run_renderer — draw shaped layout lines through a glyph atlas.
//

#ifndef NANDINA_EXPERIMENT_TEXT_GLYPH_RUN_RENDERER_HPP
#define NANDINA_EXPERIMENT_TEXT_GLYPH_RUN_RENDERER_HPP

#include "glyph_atlas.hpp"
#include "harfbuzz_text_backend.hpp"
#include "text_layout.hpp"
#include "text_layout_backend.hpp"

#include <span>
#include <vector>

namespace nandina::text
{

    struct GlyphAtlasBinding {
        GlyphAtlas* atlas = nullptr;
        GlyphAtlasTexture* texture = nullptr;
    };

    class GlyphRunRenderer final: public ITextLayoutRenderer {
    public:
        GlyphRunRenderer(GlyphAtlas& atlas, GlyphAtlasTexture& texture);
        GlyphRunRenderer(
            const HarfBuzzTextLayoutBackend& backend,
            std::span<const GlyphAtlasBinding> bindings
        );

        /// 绘制整个布局结果：逐行按 `TextLayoutLine::origin_x` 摆放后走字形图集。
        ///
        /// @param layout   已整形的布局结果。每行的 `origin_x` 是**绘制期**由
        ///                  `Text::draw_in()` 写入的水平对齐偏移；为 0 时行为与加入
        ///                  对齐之前完全一致，所以未对齐的调用方无需改动。
        /// @param context  绘制上下文，提供变换、裁剪栈与整体不透明度。
        /// @param position 文本块左上角（逻辑坐标）；某行的实际起点 = 它 + 该行 `origin_x`。
        /// @param color    文字颜色，会再乘一次上下文不透明度。
        void draw(
            const TextLayoutResult& layout,
            render::DrawContext& context,
            foundation::NanPoint position,
            foundation::NanColor color
        ) override;

        void draw_line(
            const TextLayoutLine& line,
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
