// theme/theme_scheme — parsing and diagnostics for stable scheme identities.
#include "theme_scheme.hpp"

#include <algorithm>

namespace nandina::theme
{
    auto ThemeSchemeContent::from(const DesignSystem& system, const ColorAppearance appearance)
        -> ThemeSchemeContent {
        return {system.tokens, system.palette(appearance), system.typography, system.components};
    }

    auto ThemeSchemeContent::design_system() const -> DesignSystem {
        return {tokens, palette, palette, typography, components};
    }

    auto ThemeFamilyId::parse(const std::string_view text)
        -> std::expected<ThemeFamilyId, ThemeSchemeIdError> {
        auto parsed = ThemeSchemeId::parse(text);
        if (!parsed) {
            return std::unexpected(parsed.error());
        }
        return ThemeFamilyId(std::string(parsed->text()));
    }

    namespace
    {
        [[nodiscard]] auto valid_namespace_segment(std::string_view segment) -> bool {
            if (segment.empty() || segment.front() < 'a' || segment.front() > 'z') {
                return false;
            }
            return std::ranges::all_of(segment.substr(1), [](const char value) {
                return (value >= 'a' && value <= 'z') || (value >= '0' && value <= '9')
                    || value == '-';
            });
        }

        [[nodiscard]] auto valid_name_segment(std::string_view segment) -> bool {
            if (segment.empty() || segment.front() < 'a' || segment.front() > 'z') {
                return false;
            }
            return std::ranges::all_of(segment.substr(1), [](const char value) {
                return (value >= 'a' && value <= 'z') || (value >= '0' && value <= '9')
                    || value == '-' || value == '_';
            });
        }

        [[nodiscard]] auto all_segments(
            std::string_view value,
            const char separator,
            auto&& valid_segment,
            std::size_t& bad_offset
        ) -> bool {
            std::size_t begin = 0;
            while (begin <= value.size()) {
                const auto end = value.find(separator, begin);
                const auto length =
                    end == std::string_view::npos ? value.size() - begin : end - begin;
                if (!valid_segment(value.substr(begin, length))) {
                    bad_offset = begin;
                    return false;
                }
                if (end == std::string_view::npos) {
                    return true;
                }
                begin = end + 1;
            }
            return false;
        }
    } // namespace

    auto ThemeSchemeId::parse(const std::string_view text)
        -> std::expected<ThemeSchemeId, ThemeSchemeIdError> {
        if (text.empty()) {
            return std::unexpected(ThemeSchemeIdError {.kind = ThemeSchemeIdErrorKind::empty});
        }
        if (text.size() > 255) {
            return std::unexpected(
                ThemeSchemeIdError {.kind = ThemeSchemeIdErrorKind::too_long, .offset = 255}
            );
        }
        const auto separator = text.find(':');
        if (separator == std::string_view::npos || separator == 0 || separator + 1 == text.size()) {
            return std::unexpected(
                ThemeSchemeIdError {
                    .kind = ThemeSchemeIdErrorKind::missing_separator,
                    .offset = separator == std::string_view::npos ? text.size() : separator,
                }
            );
        }
        std::size_t bad_offset = 0;
        if (!all_segments(text.substr(0, separator), '.', valid_namespace_segment, bad_offset)) {
            return std::unexpected(
                ThemeSchemeIdError {
                    .kind = ThemeSchemeIdErrorKind::invalid_namespace,
                    .offset = bad_offset,
                }
            );
        }
        if (!all_segments(text.substr(separator + 1), '/', valid_name_segment, bad_offset)) {
            return std::unexpected(
                ThemeSchemeIdError {
                    .kind = ThemeSchemeIdErrorKind::invalid_name,
                    .offset = separator + 1 + bad_offset,
                }
            );
        }
        return ThemeSchemeId(std::string(text));
    }

    auto describe(const ThemeSchemeIdError& error) -> std::string {
        switch (error.kind) {
            case ThemeSchemeIdErrorKind::empty:
                return "theme scheme id is empty";
            case ThemeSchemeIdErrorKind::too_long:
                return "theme scheme id exceeds 255 bytes";
            case ThemeSchemeIdErrorKind::missing_separator:
                return "theme scheme id must contain namespace:name";
            case ThemeSchemeIdErrorKind::invalid_namespace:
                return "theme scheme id has an invalid namespace segment at offset "
                    + std::to_string(error.offset);
            case ThemeSchemeIdErrorKind::invalid_name:
                return "theme scheme id has an invalid name segment at offset "
                    + std::to_string(error.offset);
        }
        return "invalid theme scheme id";
    }

    auto describe(const ThemeCatalogError& error) -> std::string {
        const auto suffix = error.id.valid() ? ": " + std::string(error.id.text()) : std::string {};
        switch (error.kind) {
            case ThemeCatalogErrorKind::invalid_id:
                return "invalid theme scheme id" + suffix;
            case ThemeCatalogErrorKind::invalid_family:
                return "invalid theme family id" + suffix;
            case ThemeCatalogErrorKind::duplicate_id:
                return "theme scheme id is already registered" + suffix;
            case ThemeCatalogErrorKind::duplicate_family:
                return "theme family id is already registered" + suffix;
            case ThemeCatalogErrorKind::not_found:
                return "theme scheme id was not found" + suffix;
            case ThemeCatalogErrorKind::in_use:
                return "theme scheme id is used by the current selection" + suffix;
            case ThemeCatalogErrorKind::null_content:
                return "theme scheme content is null" + suffix;
            case ThemeCatalogErrorKind::appearance_mismatch:
                return "theme scheme appearance conflicts with the current selection" + suffix;
        }
        return "invalid theme catalog operation";
    }

    auto describe(const ThemeSelectionError& error) -> std::string {
        const auto suffix = error.id.valid() ? ": " + std::string(error.id.text()) : std::string {};
        switch (error.kind) {
            case ThemeSelectionErrorKind::unknown_scheme:
                return "theme selection references an unknown scheme" + suffix;
            case ThemeSelectionErrorKind::null_content:
                return "theme selection contains null content" + suffix;
            case ThemeSelectionErrorKind::appearance_mismatch:
                return "theme selection scheme appearance does not match its slot" + suffix;
        }
        return "invalid theme selection";
    }

    auto ThemeSchemeCatalog::register_family(ThemeSchemeFamily family)
        -> std::expected<void, ThemeCatalogError> {
        if (!family.id.valid()) {
            return std::unexpected(
                ThemeCatalogError {.kind = ThemeCatalogErrorKind::invalid_family}
            );
        }
        const auto key = std::string(family.id.text());
        if (families_.contains(key)) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::duplicate_family,
                    .id = ThemeSchemeId::parse(key).value_or(ThemeSchemeId {}),
                }
            );
        }
        families_.emplace(key, std::move(family));
        return {};
    }

    auto ThemeSchemeCatalog::remove_family(const ThemeFamilyId& id)
        -> std::expected<void, ThemeCatalogError> {
        const auto key = std::string(id.text());
        if (!families_.contains(key)) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::not_found,
                    .id = ThemeSchemeId::parse(key).value_or(ThemeSchemeId {}),
                }
            );
        }
        if (std::ranges::any_of(
                schemes_,
                [&id](const auto& entry) { return entry.second.family == id; }
            ))
        {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::in_use,
                    .id = ThemeSchemeId::parse(key).value_or(ThemeSchemeId {}),
                }
            );
        }
        families_.erase(key);
        return {};
    }

    auto ThemeSchemeCatalog::families() const -> std::vector<ThemeSchemeFamily> {
        std::vector<ThemeSchemeFamily> result;
        result.reserve(families_.size());
        for (const auto& [key, family]: families_) {
            (void)key;
            result.push_back(family);
        }
        return result;
    }

    auto ThemeSchemeCatalog::register_scheme(ThemeScheme scheme)
        -> std::expected<void, ThemeCatalogError> {
        if (!scheme.id.valid()) {
            return std::unexpected(ThemeCatalogError {.kind = ThemeCatalogErrorKind::invalid_id});
        }
        if (!scheme.content) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::null_content,
                    .id = scheme.id,
                }
            );
        }
        if (scheme.family && !families_.contains(std::string(scheme.family->text()))) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::invalid_family,
                    .id = scheme.id,
                }
            );
        }
        const auto key = std::string(scheme.id.text());
        if (schemes_.contains(key)) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::duplicate_id,
                    .id = scheme.id,
                }
            );
        }
        schemes_.emplace(key, std::move(scheme));
        return {};
    }

    auto ThemeSchemeCatalog::replace_scheme(ThemeScheme scheme)
        -> std::expected<void, ThemeCatalogError> {
        if (!scheme.id.valid()) {
            return std::unexpected(ThemeCatalogError {.kind = ThemeCatalogErrorKind::invalid_id});
        }
        if (!scheme.content) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::null_content,
                    .id = scheme.id,
                }
            );
        }
        if (scheme.family && !families_.contains(std::string(scheme.family->text()))) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::invalid_family,
                    .id = scheme.id,
                }
            );
        }
        const auto key = std::string(scheme.id.text());
        if (!schemes_.contains(key)) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::not_found,
                    .id = scheme.id,
                }
            );
        }
        ThemeSchemeCatalog staged = *this;
        staged.schemes_.insert_or_assign(key, std::move(scheme));
        std::optional<Resolved> resolved;
        if (staged.selection_) {
            auto result = staged.resolve(*staged.selection_, staged.system_appearance_);
            if (!result) {
                return std::unexpected(
                    ThemeCatalogError {
                        .kind = ThemeCatalogErrorKind::appearance_mismatch,
                        .id = ThemeSchemeId::parse(key).value_or(ThemeSchemeId {}),
                    }
                );
            }
            resolved = std::move(*result);
        }
        schemes_ = std::move(staged.schemes_);
        if (resolved) {
            active_id_ = resolved->scheme ? std::optional(resolved->scheme->id) : std::nullopt;
            active_anonymous_content_ = std::move(resolved->anonymous_content);
            active_appearance_ = resolved->appearance;
        }
        return {};
    }

    auto ThemeSchemeCatalog::remove_scheme(const ThemeSchemeId& id)
        -> std::expected<void, ThemeCatalogError> {
        const auto key = std::string(id.text());
        if (!schemes_.contains(key)) {
            return std::unexpected(
                ThemeCatalogError {.kind = ThemeCatalogErrorKind::not_found, .id = id}
            );
        }
        if (uses(id)) {
            return std::unexpected(
                ThemeCatalogError {.kind = ThemeCatalogErrorKind::in_use, .id = id}
            );
        }
        schemes_.erase(key);
        return {};
    }

    auto ThemeSchemeCatalog::find(const ThemeSchemeId& id) const -> const ThemeScheme* {
        const auto found = schemes_.find(id.text());
        return found == schemes_.end() ? nullptr : &found->second;
    }

    auto ThemeSchemeCatalog::schemes() const -> std::vector<ThemeScheme> {
        std::vector<ThemeScheme> result;
        result.reserve(schemes_.size());
        for (const auto& [key, scheme]: schemes_) {
            (void)key;
            result.push_back(scheme);
        }
        return result;
    }

    auto ThemeSchemeCatalog::schemes_in_family(const ThemeFamilyId& id) const
        -> std::vector<ThemeScheme> {
        std::vector<ThemeScheme> result;
        for (const auto& [key, scheme]: schemes_) {
            (void)key;
            if (scheme.family == id) {
                result.push_back(scheme);
            }
        }
        return result;
    }

    auto ThemeSchemeCatalog::resolve(
        const ThemeSelection& selection,
        const ColorAppearance system_appearance
    ) const -> std::expected<Resolved, ThemeSelectionError> {
        if (const auto* fixed = std::get_if<FixedThemeSelection>(&selection)) {
            if (const auto* scheme_id = std::get_if<ThemeSchemeId>(&fixed->source)) {
                const auto* scheme = find(*scheme_id);
                if (!scheme) {
                    return std::unexpected(
                        ThemeSelectionError {
                            .kind = ThemeSelectionErrorKind::unknown_scheme,
                            .id = *scheme_id,
                        }
                    );
                }
                if (!scheme->content) {
                    return std::unexpected(
                        ThemeSelectionError {
                            .kind = ThemeSelectionErrorKind::null_content,
                            .id = *scheme_id,
                        }
                    );
                }
                return Resolved {.scheme = scheme, .appearance = scheme->appearance};
            }
            const auto& source = std::get<UnregisteredThemeSource>(fixed->source);
            if (!source.content) {
                return std::unexpected(
                    ThemeSelectionError {
                        .kind = ThemeSelectionErrorKind::null_content,
                    }
                );
            }
            return Resolved {
                .anonymous_content = source.content,
                .appearance = source.appearance,
            };
        }

        const auto& appearance = std::get<AppearanceThemeSelection>(selection);
        const auto* light = find(appearance.light);
        if (!light) {
            return std::unexpected(
                ThemeSelectionError {
                    .kind = ThemeSelectionErrorKind::unknown_scheme,
                    .id = appearance.light,
                }
            );
        }
        const auto* dark = find(appearance.dark);
        if (!dark) {
            return std::unexpected(
                ThemeSelectionError {
                    .kind = ThemeSelectionErrorKind::unknown_scheme,
                    .id = appearance.dark,
                }
            );
        }
        if (light->appearance != ColorAppearance::light) {
            return std::unexpected(
                ThemeSelectionError {
                    .kind = ThemeSelectionErrorKind::appearance_mismatch,
                    .id = light->id,
                }
            );
        }
        if (dark->appearance != ColorAppearance::dark) {
            return std::unexpected(
                ThemeSelectionError {
                    .kind = ThemeSelectionErrorKind::appearance_mismatch,
                    .id = dark->id,
                }
            );
        }
        const auto selected_appearance = appearance.preference == ThemePreference::system
            ? system_appearance
            : appearance.preference == ThemePreference::dark ? ColorAppearance::dark
                                                             : ColorAppearance::light;
        const auto* selected = selected_appearance == ColorAppearance::dark ? dark : light;
        if (!selected->content) {
            return std::unexpected(
                ThemeSelectionError {
                    .kind = ThemeSelectionErrorKind::null_content,
                    .id = selected->id,
                }
            );
        }
        return Resolved {.scheme = selected, .appearance = selected_appearance};
    }

    auto ThemeSchemeCatalog::uses(const ThemeSchemeId& id) const -> bool {
        if (!selection_) {
            return false;
        }
        if (const auto* fixed = std::get_if<FixedThemeSelection>(&*selection_)) {
            if (const auto* registered = std::get_if<ThemeSchemeId>(&fixed->source)) {
                return *registered == id;
            }
            return false;
        }
        const auto& appearance = std::get<AppearanceThemeSelection>(*selection_);
        return appearance.light == id || appearance.dark == id;
    }

    auto
    ThemeSchemeCatalog::select(ThemeSelection selection, const ColorAppearance system_appearance)
        -> std::expected<void, ThemeSelectionError> {
        auto resolved = resolve(selection, system_appearance);
        if (!resolved) {
            return std::unexpected(resolved.error());
        }
        selection_ = std::move(selection);
        active_id_ = resolved->scheme ? std::optional(resolved->scheme->id) : std::nullopt;
        active_anonymous_content_ = std::move(resolved->anonymous_content);
        active_appearance_ = resolved->appearance;
        system_appearance_ = system_appearance;
        return {};
    }

    auto ThemeSchemeCatalog::selection() const noexcept -> const std::optional<ThemeSelection>& {
        return selection_;
    }

    auto ThemeSchemeCatalog::active() const noexcept -> const ThemeScheme* {
        return active_id_ ? find(*active_id_) : nullptr;
    }

    auto ThemeSchemeCatalog::active_content() const noexcept -> const ThemeSchemeContent* {
        const auto* current = active();
        return current ? current->content.get() : active_anonymous_content_.get();
    }

    auto ThemeSchemeCatalog::active_appearance() const noexcept -> ColorAppearance {
        return active_appearance_;
    }
} // namespace nandina::theme
