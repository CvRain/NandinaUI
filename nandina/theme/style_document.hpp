//
// theme/style_document — styles.toml data compiled into the runtime theme objects.
//

#ifndef NANDINA_EXPERIMENT_THEME_STYLE_DOCUMENT_HPP
#define NANDINA_EXPERIMENT_THEME_STYLE_DOCUMENT_HPP

#include "font_request.hpp"
#include "theme_manager.hpp"

#include <expected>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::theme
{
    /// File-format declaration; this pair is not runtime theme identity.
    struct StyleThemePair {
        std::string light_theme;
        std::string dark_theme;
    };

    struct StyleDocument {
        std::map<std::string, NanReferencePalette, std::less<>> reference_palettes;
        std::map<std::string, NanTheme, std::less<>> themes;
        std::map<std::string, StyleThemePair, std::less<>> theme_families;
        std::string active_theme = "default";
        std::string active_family;
        ThemePreference preference = ThemePreference::system;
        std::shared_ptr<NanStyle> style = std::make_shared<NanStyle>();
        std::vector<FontFamilyDeclaration> font_families;

        /// Theme aliases, directory, selection and style rules commit once after validation.
        /// Declarative `font_families` are registered separately by upper-layer text/app hosts.
        [[nodiscard]] auto apply(ThemeManager& manager) const -> std::expected<void, std::string>;
    };

    [[nodiscard]] auto
    parse_style_document(std::string_view source, std::string_view source_name = "styles.toml")
        -> std::expected<StyleDocument, std::string>;

    [[nodiscard]] auto load_style_document(const std::filesystem::path& path)
        -> std::expected<StyleDocument, std::string>;

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_STYLE_DOCUMENT_HPP
