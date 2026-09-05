//
// widget/primitives/text_layout_backend — pluggable text layout contract.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_LAYOUT_BACKEND_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_LAYOUT_BACKEND_HPP

#include "text_layout.hpp"

namespace nandina::render
{
    class DrawContext;
}

namespace nandina::widget::primitives
{

    class ITextLayoutBackend {
    public:
        virtual ~ITextLayoutBackend() = default;

        [[nodiscard]] virtual auto layout(TextLayoutInput input) const -> TextLayoutResult = 0;
    };

    class ITextLayoutRenderer {
    public:
        virtual ~ITextLayoutRenderer() = default;

        virtual void draw(
            const TextLayoutResult& layout,
            render::DrawContext& context,
            foundation::NanPoint position,
            foundation::NanColor color
        ) = 0;
    };

    /// Process-lifetime fallback used until a shaping backend is installed.
    [[nodiscard]] auto deterministic_text_layout_backend() -> const ITextLayoutBackend&;

    struct TextPipeline {
        const ITextLayoutBackend* backend = &deterministic_text_layout_backend();
        ITextLayoutRenderer* renderer = nullptr;
    };

} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_LAYOUT_BACKEND_HPP
