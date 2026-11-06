// theme/theme_scheme — stable scheme identities, catalog entries and selection values.
#ifndef NANDINA_EXPERIMENT_THEME_THEME_SCHEME_HPP
#define NANDINA_EXPERIMENT_THEME_THEME_SCHEME_HPP

#include "appearance.hpp"
#include "design_system.hpp"

#include <cstddef>
#include <expected>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace nandina::theme
{
    enum class ThemeSchemeIdErrorKind {
        empty,
        too_long,
        missing_separator,
        invalid_namespace,
        invalid_name,
    };

    struct ThemeSchemeIdError {
        ThemeSchemeIdErrorKind kind = ThemeSchemeIdErrorKind::empty;
        std::size_t offset = 0;
    };

    class ThemeSchemeId {
    public:
        /// Empty is an invalid sentinel, never an implicit default scheme.
        ThemeSchemeId() = default;

        [[nodiscard]] static auto parse(std::string_view text)
            -> std::expected<ThemeSchemeId, ThemeSchemeIdError>;
        [[nodiscard]] auto valid() const noexcept -> bool {
            return !text_.empty();
        }
        [[nodiscard]] auto text() const noexcept -> std::string_view {
            return text_;
        }

        friend auto operator==(const ThemeSchemeId&, const ThemeSchemeId&) -> bool = default;

    private:
        explicit ThemeSchemeId(std::string text): text_(std::move(text)) {}
        std::string text_;
    };

    class ThemeFamilyId {
    public:
        ThemeFamilyId() = default;
        [[nodiscard]] static auto parse(std::string_view text)
            -> std::expected<ThemeFamilyId, ThemeSchemeIdError>;
        [[nodiscard]] auto valid() const noexcept -> bool {
            return !text_.empty();
        }
        [[nodiscard]] auto text() const noexcept -> std::string_view {
            return text_;
        }
        friend auto operator==(const ThemeFamilyId&, const ThemeFamilyId&) -> bool = default;

    private:
        explicit ThemeFamilyId(std::string text): text_(std::move(text)) {}
        std::string text_;
    };

    struct ThemeSchemeFamily {
        ThemeFamilyId id;
        std::string display_name;
    };

    /// One appearance's complete immutable content; DesignSystem is only a renderer adapter.
    struct ThemeSchemeContent {
        NanTokens tokens;
        NanColorScheme palette;
        TypographyRoles typography;
        ComponentRecipes components;

        [[nodiscard]] static auto from(const DesignSystem& system, ColorAppearance appearance)
            -> ThemeSchemeContent;
        [[nodiscard]] auto design_system() const -> DesignSystem;
    };

    struct UnregisteredThemeSource {
        std::shared_ptr<const ThemeSchemeContent> content;
        ColorAppearance appearance = ColorAppearance::light;

        friend auto operator==(const UnregisteredThemeSource&, const UnregisteredThemeSource&)
            -> bool = default;
    };

    using FixedThemeSource = std::variant<ThemeSchemeId, UnregisteredThemeSource>;

    struct ThemeScheme {
        ThemeSchemeId id;
        std::string display_name;
        ColorAppearance appearance = ColorAppearance::light;
        std::optional<ThemeFamilyId> family;
        std::shared_ptr<const ThemeSchemeContent> content;
    };

    struct FixedThemeSelection {
        FixedThemeSource source;

        friend auto operator==(const FixedThemeSelection&, const FixedThemeSelection&)
            -> bool = default;
    };

    struct AppearanceThemeSelection {
        ThemeSchemeId light;
        ThemeSchemeId dark;
        ThemePreference preference = ThemePreference::system;

        friend auto operator==(const AppearanceThemeSelection&, const AppearanceThemeSelection&)
            -> bool = default;
    };

    using ThemeSelection = std::variant<FixedThemeSelection, AppearanceThemeSelection>;

    enum class ThemeCatalogErrorKind {
        invalid_id,
        invalid_family,
        duplicate_id,
        duplicate_family,
        not_found,
        in_use,
        null_content,
        appearance_mismatch,
    };

    struct ThemeCatalogError {
        ThemeCatalogErrorKind kind = ThemeCatalogErrorKind::invalid_id;
        ThemeSchemeId id;
    };

    enum class ThemeSelectionErrorKind {
        unknown_scheme,
        null_content,
        appearance_mismatch,
    };

    struct ThemeSelectionError {
        ThemeSelectionErrorKind kind = ThemeSelectionErrorKind::unknown_scheme;
        ThemeSchemeId id;
    };

    /// Value directory used by ThemeManager; a staged copy permits failure-atomic commits.
    class ThemeSchemeCatalog {
    public:
        ThemeSchemeCatalog() = default;

        [[nodiscard]] auto register_family(ThemeSchemeFamily family)
            -> std::expected<void, ThemeCatalogError>;
        [[nodiscard]] auto remove_family(const ThemeFamilyId& id)
            -> std::expected<void, ThemeCatalogError>;
        [[nodiscard]] auto families() const -> std::vector<ThemeSchemeFamily>;

        [[nodiscard]] auto register_scheme(ThemeScheme scheme)
            -> std::expected<void, ThemeCatalogError>;
        [[nodiscard]] auto replace_scheme(ThemeScheme scheme)
            -> std::expected<void, ThemeCatalogError>;
        [[nodiscard]] auto remove_scheme(const ThemeSchemeId& id)
            -> std::expected<void, ThemeCatalogError>;
        /// Borrowed entry; replacement may invalidate all entry pointers in this value catalog.
        [[nodiscard]] auto find(const ThemeSchemeId& id) const -> const ThemeScheme*;
        [[nodiscard]] auto schemes() const -> std::vector<ThemeScheme>;
        [[nodiscard]] auto schemes_in_family(const ThemeFamilyId& id) const
            -> std::vector<ThemeScheme>;

        [[nodiscard]] auto select(ThemeSelection selection, ColorAppearance system_appearance)
            -> std::expected<void, ThemeSelectionError>;
        [[nodiscard]] auto selection() const noexcept -> const std::optional<ThemeSelection>&;
        [[nodiscard]] auto active() const noexcept -> const ThemeScheme*;
        [[nodiscard]] auto active_content() const noexcept -> const ThemeSchemeContent*;
        [[nodiscard]] auto active_appearance() const noexcept -> ColorAppearance;

    private:
        struct Resolved {
            const ThemeScheme* scheme = nullptr;
            std::shared_ptr<const ThemeSchemeContent> anonymous_content;
            ColorAppearance appearance = ColorAppearance::light;
        };
        [[nodiscard]] auto
        resolve(const ThemeSelection& selection, ColorAppearance system_appearance) const
            -> std::expected<Resolved, ThemeSelectionError>;
        [[nodiscard]] auto uses(const ThemeSchemeId& id) const -> bool;

        std::map<std::string, ThemeSchemeFamily, std::less<>> families_;
        std::map<std::string, ThemeScheme, std::less<>> schemes_;
        std::optional<ThemeSelection> selection_;
        std::optional<ThemeSchemeId> active_id_;
        std::shared_ptr<const ThemeSchemeContent> active_anonymous_content_;
        ColorAppearance active_appearance_ = ColorAppearance::light;
        ColorAppearance system_appearance_ = ColorAppearance::light;
    };

    [[nodiscard]] auto describe(const ThemeSchemeIdError& error) -> std::string;
    [[nodiscard]] auto describe(const ThemeCatalogError& error) -> std::string;
    [[nodiscard]] auto describe(const ThemeSelectionError& error) -> std::string;
} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_THEME_SCHEME_HPP
