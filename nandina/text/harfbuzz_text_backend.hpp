//
// text/harfbuzz_text_backend — HarfBuzz shaping into TextLayoutResult glyph runs.
//

#ifndef NANDINA_EXPERIMENT_TEXT_HARFBUZZ_TEXT_BACKEND_HPP
#define NANDINA_EXPERIMENT_TEXT_HARFBUZZ_TEXT_BACKEND_HPP

#include "../widget/primitives/text_layout_backend.hpp"
#include "font_face.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace nandina::text
{

    class HarfBuzzTextLayoutBackend final: public widget::primitives::ITextLayoutBackend {
    public:
        explicit HarfBuzzTextLayoutBackend(std::shared_ptr<FreeTypeFontFace> face);
        HarfBuzzTextLayoutBackend(
            std::shared_ptr<FreeTypeFontFace> primary,
            std::vector<std::shared_ptr<FreeTypeFontFace>> fallbacks
        );
        ~HarfBuzzTextLayoutBackend() override;

        HarfBuzzTextLayoutBackend(const HarfBuzzTextLayoutBackend&) = delete;
        auto operator=(const HarfBuzzTextLayoutBackend&) -> HarfBuzzTextLayoutBackend& = delete;

        [[nodiscard]] auto layout(widget::primitives::TextLayoutInput input) const
            -> widget::primitives::TextLayoutResult override;
        [[nodiscard]] auto font_face() const -> const std::shared_ptr<FreeTypeFontFace>&;
        [[nodiscard]] auto font_face(std::size_t index) const
            -> const std::shared_ptr<FreeTypeFontFace>&;
        [[nodiscard]] auto font_count() const -> std::size_t;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

} // namespace nandina::text

#endif // NANDINA_EXPERIMENT_TEXT_HARFBUZZ_TEXT_BACKEND_HPP
