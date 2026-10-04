// widget/primitives/text_layout_backend - compatibility names for canonical text protocols.

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_LAYOUT_BACKEND_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_LAYOUT_BACKEND_HPP

#include "../../text/text_layout_backend.hpp"
#include "text_layout.hpp"

namespace nandina::widget::primitives
{
    using text::deterministic_text_layout_backend;
    using text::ITextLayoutBackend;
    using text::ITextLayoutRenderer;
    using text::TextPipeline;
} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_LAYOUT_BACKEND_HPP
