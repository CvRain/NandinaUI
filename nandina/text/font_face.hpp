//
// text/font_face — backend-neutral font metrics and raster values.
//

#ifndef NANDINA_EXPERIMENT_TEXT_FONT_FACE_HPP
#define NANDINA_EXPERIMENT_TEXT_FONT_FACE_HPP

#include "../resource/resource.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

namespace nandina::text
{

    struct FontMetrics {
        float ascender = 0.0F;
        float descender = 0.0F;
        float line_height = 0.0F;
    };

    struct GlyphMetrics {
        std::uint32_t glyph_index = 0;
        float advance_x = 0.0F;
        float bearing_x = 0.0F;
        float bearing_y = 0.0F;
        float width = 0.0F;
        float height = 0.0F;
    };

    struct GlyphBitmap {
        GlyphMetrics metrics;
        int width = 0;
        int height = 0;
        int pitch = 0;
        std::vector<std::uint8_t> alpha;
    };

    class FreeTypeFontFace {
    public:
        explicit FreeTypeFontFace(const std::filesystem::path& path, long face_index = 0);
        explicit FreeTypeFontFace(resource::ResourceHandle resource, long face_index = 0);
        ~FreeTypeFontFace();

        FreeTypeFontFace(const FreeTypeFontFace&) = delete;
        auto operator=(const FreeTypeFontFace&) -> FreeTypeFontFace& = delete;
        FreeTypeFontFace(FreeTypeFontFace&&) noexcept;
        auto operator=(FreeTypeFontFace&&) noexcept -> FreeTypeFontFace&;

        [[nodiscard]] auto family_name() const -> std::string_view;
        [[nodiscard]] auto style_name() const -> std::string_view;
        [[nodiscard]] auto glyph_index(char32_t codepoint) const -> std::uint32_t;
        [[nodiscard]] auto metrics(float pixel_size) const -> FontMetrics;
        [[nodiscard]] auto glyph_metrics(char32_t codepoint, float pixel_size) const
            -> GlyphMetrics;
        [[nodiscard]] auto glyph_metrics_by_index(std::uint32_t glyph_index, float pixel_size) const
            -> GlyphMetrics;
        [[nodiscard]] auto rasterize(char32_t codepoint, float pixel_size) const -> GlyphBitmap;
        [[nodiscard]] auto rasterize_glyph(std::uint32_t glyph_index, float pixel_size) const
            -> GlyphBitmap;

        /// Internal integration handle for shaping backends. The pointer is an FT_Face.
        [[nodiscard]] auto native_face_handle() const -> void*;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

} // namespace nandina::text

#endif // NANDINA_EXPERIMENT_TEXT_FONT_FACE_HPP
