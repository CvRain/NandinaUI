// theme/font_request - font selection values without font engine dependencies.

#ifndef NANDINA_EXPERIMENT_THEME_FONT_REQUEST_HPP
#define NANDINA_EXPERIMENT_THEME_FONT_REQUEST_HPP

#include "../resource/resource.hpp"

#include <compare>
#include <cstdint>
#include <optional>
#include <vector>

namespace nandina::theme
{
    enum class FontSlant { normal, italic, oblique };

    /// Describes the requested family, weight and slant; loading remains in text.
    struct FontRequest {
        std::optional<resource::ResourceKey> family;
        int weight = 400;

        FontSlant slant = FontSlant::normal;

        auto operator<=>(const FontRequest&) const = default;
    };

    /// Pure declarative font face entry parsed from style documents (no font-engine handle).
    struct FontFaceDeclaration {
        resource::ResourceKey resource;
        std::uint32_t face_index = 0;
        int weight = 400;
        FontSlant slant = FontSlant::normal;

        auto operator==(const FontFaceDeclaration&) const -> bool = default;
    };

    /// Pure declarative font family entry parsed from style documents; registration lives in text.
    struct FontFamilyDeclaration {
        resource::ResourceKey family;
        std::vector<FontFaceDeclaration> faces;
        std::vector<resource::ResourceKey> fallbacks;
        bool is_default = false;

        auto operator==(const FontFamilyDeclaration&) const -> bool = default;
    };
} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_FONT_REQUEST_HPP
