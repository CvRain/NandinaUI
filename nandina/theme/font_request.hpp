// theme/font_request - font selection values without font engine dependencies.

#ifndef NANDINA_EXPERIMENT_THEME_FONT_REQUEST_HPP
#define NANDINA_EXPERIMENT_THEME_FONT_REQUEST_HPP

#include "../resource/resource.hpp"

#include <compare>
#include <optional>

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
} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_FONT_REQUEST_HPP
