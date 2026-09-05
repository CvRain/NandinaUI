//
// text/freetype_font_face — FreeType-backed font metrics and rasterization.
//

#include "font_face.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace nandina::text
{
    namespace
    {
        [[nodiscard]] auto pixels(FT_Pos value) -> float {
            return static_cast<float>(value) / 64.0F;
        }

        [[nodiscard]] auto pixel_height(float value) -> FT_UInt {
            return static_cast<FT_UInt>(std::max(1.0F, std::round(value)));
        }

        [[noreturn]] void throw_freetype_error(std::string_view operation, FT_Error error) {
            throw std::runtime_error(
                std::string(operation) + " failed with FreeType error " + std::to_string(error)
            );
        }
    } // namespace

    class FreeTypeFontFace::Impl {
    public:
        explicit Impl(const std::filesystem::path& path, long face_index) {
            if (const auto error = FT_Init_FreeType(&library); error != 0) {
                throw_freetype_error("FT_Init_FreeType", error);
            }

            const auto path_string = path.string();
            if (const auto error = FT_New_Face(library, path_string.c_str(), face_index, &face);
                error != 0)
            {
                FT_Done_FreeType(library);
                library = nullptr;
                throw std::runtime_error(
                    "Failed to load font '" + path_string + "' with FreeType error "
                    + std::to_string(error)
                );
            }
        }

        explicit Impl(resource::ResourceHandle source, long face_index):
            resource(std::move(source)) {
            if (!resource || resource->bytes().empty()
                || resource->size() > static_cast<std::size_t>(std::numeric_limits<FT_Long>::max()))
            {
                throw std::invalid_argument(
                    "FreeType memory face requires non-empty bounded resource bytes"
                );
            }
            if (const auto error = FT_Init_FreeType(&library); error != 0) {
                throw_freetype_error("FT_Init_FreeType", error);
            }
            const auto bytes = resource->bytes();
            if (const auto error = FT_New_Memory_Face(
                    library,
                    reinterpret_cast<const FT_Byte*>(bytes.data()),
                    static_cast<FT_Long>(bytes.size()),
                    face_index,
                    &face
                );
                error != 0)
            {
                FT_Done_FreeType(library);
                library = nullptr;
                throw_freetype_error("FT_New_Memory_Face", error);
            }
        }

        ~Impl() {
            if (face != nullptr) {
                FT_Done_Face(face);
            }
            if (library != nullptr) {
                FT_Done_FreeType(library);
            }
        }

        void set_pixel_size(float pixel_size) const {
            if (const auto error = FT_Set_Pixel_Sizes(face, 0, pixel_height(pixel_size));
                error != 0)
            {
                throw_freetype_error("FT_Set_Pixel_Sizes", error);
            }
        }

        [[nodiscard]] auto
        load_glyph(std::uint32_t glyph_index, float pixel_size, FT_Int32 flags) const
            -> FT_GlyphSlot {
            set_pixel_size(pixel_size);
            if (const auto error = FT_Load_Glyph(face, glyph_index, flags); error != 0) {
                throw_freetype_error("FT_Load_Glyph", error);
            }
            return face->glyph;
        }

        FT_Library library = nullptr;
        FT_Face face = nullptr;
        resource::ResourceHandle resource;
    };

    FreeTypeFontFace::FreeTypeFontFace(const std::filesystem::path& path, long face_index):
        impl_(std::make_unique<Impl>(path, face_index)) {}

    FreeTypeFontFace::FreeTypeFontFace(resource::ResourceHandle resource, long face_index):
        impl_(std::make_unique<Impl>(std::move(resource), face_index)) {}

    FreeTypeFontFace::~FreeTypeFontFace() = default;
    FreeTypeFontFace::FreeTypeFontFace(FreeTypeFontFace&&) noexcept = default;
    auto FreeTypeFontFace::operator=(FreeTypeFontFace&&) noexcept -> FreeTypeFontFace& = default;

    auto FreeTypeFontFace::family_name() const -> std::string_view {
        return impl_->face->family_name != nullptr ? impl_->face->family_name : "";
    }

    auto FreeTypeFontFace::style_name() const -> std::string_view {
        return impl_->face->style_name != nullptr ? impl_->face->style_name : "";
    }

    auto FreeTypeFontFace::glyph_index(char32_t codepoint) const -> std::uint32_t {
        return static_cast<std::uint32_t>(
            FT_Get_Char_Index(impl_->face, static_cast<FT_ULong>(codepoint))
        );
    }

    auto FreeTypeFontFace::metrics(float pixel_size) const -> FontMetrics {
        impl_->set_pixel_size(pixel_size);
        const auto& metrics = impl_->face->size->metrics;
        return {
            .ascender = pixels(metrics.ascender),
            .descender = pixels(metrics.descender),
            .line_height = pixels(metrics.height),
        };
    }

    auto FreeTypeFontFace::glyph_metrics(char32_t codepoint, float pixel_size) const
        -> GlyphMetrics {
        return glyph_metrics_by_index(glyph_index(codepoint), pixel_size);
    }

    auto FreeTypeFontFace::glyph_metrics_by_index(std::uint32_t index, float pixel_size) const
        -> GlyphMetrics {
        const auto glyph = impl_->load_glyph(index, pixel_size, FT_LOAD_DEFAULT);
        return {
            .glyph_index = index,
            .advance_x = pixels(glyph->advance.x),
            .bearing_x = pixels(glyph->metrics.horiBearingX),
            .bearing_y = pixels(glyph->metrics.horiBearingY),
            .width = pixels(glyph->metrics.width),
            .height = pixels(glyph->metrics.height),
        };
    }

    auto FreeTypeFontFace::rasterize(char32_t codepoint, float pixel_size) const -> GlyphBitmap {
        return rasterize_glyph(glyph_index(codepoint), pixel_size);
    }

    auto FreeTypeFontFace::rasterize_glyph(std::uint32_t index, float pixel_size) const
        -> GlyphBitmap {
        const auto glyph = impl_->load_glyph(index, pixel_size, FT_LOAD_RENDER);
        const auto& bitmap = glyph->bitmap;
        if (bitmap.pixel_mode != FT_PIXEL_MODE_GRAY && bitmap.width > 0 && bitmap.rows > 0) {
            throw std::runtime_error("FreeType glyph bitmap is not 8-bit grayscale");
        }

        GlyphBitmap result {
            .metrics =
                GlyphMetrics {
                    .glyph_index = index,
                    .advance_x = pixels(glyph->advance.x),
                    // rasterized bitmap 的实际原点由 bitmap_left/top 给出；轮廓
                    // metrics 在 hinting 后不保证与 bitmap 像素框完全一致。
                    .bearing_x = static_cast<float>(glyph->bitmap_left),
                    .bearing_y = static_cast<float>(glyph->bitmap_top),
                    .width = static_cast<float>(bitmap.width),
                    .height = static_cast<float>(bitmap.rows),
                },
            .width = static_cast<int>(bitmap.width),
            .height = static_cast<int>(bitmap.rows),
            .pitch = static_cast<int>(bitmap.width),
        };
        result.alpha.resize(static_cast<std::size_t>(result.width * result.height));

        const int source_pitch = bitmap.pitch;
        for (int row = 0; row < result.height; ++row) {
            const auto* source = source_pitch >= 0
                ? bitmap.buffer + row * source_pitch
                : bitmap.buffer + (result.height - 1 - row) * (-source_pitch);
            std::copy_n(source, result.width, result.alpha.begin() + row * result.width);
        }
        return result;
    }

    auto FreeTypeFontFace::native_face_handle() const -> void* {
        return impl_->face;
    }

} // namespace nandina::text
