// theme/theme_scheme — parsing and diagnostics for stable scheme identities.
#include "theme_scheme.hpp"

#include <cmath>

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

    auto ThemeSchemeContent::from_reference(
        const NanReferencePalette& reference,
        const ColorAppearance appearance,
        const PaletteVariantPolicy policy
    ) -> ThemeSchemeContent {
        const auto template_system = default_design_system();
        return {
            template_system.tokens,
            make_color_scheme(reference, appearance, policy),
            template_system.typography,
            template_system.components,
            reference,
        };
    }

    auto ThemeSchemeContent::from_semantic_colors(const SemanticColorSpec& spec)
        -> ThemeSchemeContent {
        const auto template_system = default_design_system();
        return {
            template_system.tokens,
            NanColorScheme(spec),
            template_system.typography,
            template_system.components,
            std::nullopt,
        };
    }

    namespace
    {
        [[nodiscard]] auto finite_color(const NanColor& color) -> bool {
            const auto value = color.oklch();
            return std::isfinite(value.light) && std::isfinite(value.chroma)
                && std::isfinite(value.hue) && std::isfinite(value.alpha);
        }

        using PaletteField = NanColor NanColorScheme::*;

        /// 规范语义角色到 NanColorScheme 字段的唯一映射；别名先经 canonical_color_token。
        [[nodiscard]] auto canonical_field(const ColorToken role) noexcept -> PaletteField {
            switch (role) {
                case ColorToken::background:
                    return &NanColorScheme::background;
                case ColorToken::foreground:
                    return &NanColorScheme::foreground;
                case ColorToken::card:
                    return &NanColorScheme::card;
                case ColorToken::card_foreground:
                    return &NanColorScheme::card_foreground;
                case ColorToken::popover:
                    return &NanColorScheme::popover;
                case ColorToken::popover_foreground:
                    return &NanColorScheme::popover_foreground;
                case ColorToken::primary:
                    return &NanColorScheme::primary;
                case ColorToken::primary_foreground:
                    return &NanColorScheme::primary_foreground;
                case ColorToken::secondary:
                    return &NanColorScheme::secondary;
                case ColorToken::secondary_foreground:
                    return &NanColorScheme::secondary_foreground;
                case ColorToken::muted:
                    return &NanColorScheme::muted;
                case ColorToken::muted_foreground:
                    return &NanColorScheme::muted_foreground;
                case ColorToken::accent:
                    return &NanColorScheme::accent;
                case ColorToken::accent_foreground:
                    return &NanColorScheme::accent_foreground;
                case ColorToken::destructive:
                    return &NanColorScheme::destructive;
                case ColorToken::destructive_foreground:
                    return &NanColorScheme::destructive_foreground;
                case ColorToken::border:
                    return &NanColorScheme::border;
                case ColorToken::input:
                    return &NanColorScheme::input;
                case ColorToken::ring:
                    return &NanColorScheme::ring;
                case ColorToken::surface:
                    return &NanColorScheme::surface;
                case ColorToken::surface_foreground:
                    return &NanColorScheme::surface_foreground;
                case ColorToken::surface_variant:
                    return &NanColorScheme::surface_variant;
                case ColorToken::surface_variant_foreground:
                    return &NanColorScheme::surface_variant_foreground;
                case ColorToken::tertiary:
                    return &NanColorScheme::tertiary;
                case ColorToken::tertiary_foreground:
                    return &NanColorScheme::tertiary_foreground;
                case ColorToken::success:
                    return &NanColorScheme::success;
                case ColorToken::success_foreground:
                    return &NanColorScheme::success_foreground;
                case ColorToken::warning:
                    return &NanColorScheme::warning;
                case ColorToken::warning_foreground:
                    return &NanColorScheme::warning_foreground;
                case ColorToken::error:
                    return &NanColorScheme::error;
                case ColorToken::error_foreground:
                    return &NanColorScheme::error_foreground;
                case ColorToken::info:
                    return &NanColorScheme::info;
                case ColorToken::info_foreground:
                    return &NanColorScheme::info_foreground;
                case ColorToken::selection:
                    return &NanColorScheme::selection;
                case ColorToken::on_background:
                case ColorToken::on_primary:
                case ColorToken::on_secondary:
                case ColorToken::on_tertiary:
                case ColorToken::on_surface:
                case ColorToken::on_surface_variant:
                case ColorToken::on_muted:
                case ColorToken::outline:
                case ColorToken::outline_variant:
                case ColorToken::on_success:
                case ColorToken::on_warning:
                case ColorToken::on_error:
                case ColorToken::on_info:
                case ColorToken::focus_ring:
                    break; // canonicalized before this lookup
            }
            return &NanColorScheme::background;
        }

        /// 兼容别名与规范字段同值；只有规范字段是存储来源。
        void sync_color_aliases(NanColorScheme& palette) {
            palette.on_background = palette.foreground;
            palette.on_primary = palette.primary_foreground;
            palette.on_secondary = palette.secondary_foreground;
            palette.on_tertiary = palette.tertiary_foreground;
            palette.on_surface = palette.surface_foreground;
            palette.on_surface_variant = palette.surface_variant_foreground;
            palette.on_muted = palette.muted_foreground;
            palette.outline = palette.border;
            palette.outline_variant = palette.surface_variant;
            palette.on_success = palette.success_foreground;
            palette.on_warning = palette.warning_foreground;
            palette.on_error = palette.error_foreground;
            palette.on_info = palette.info_foreground;
            palette.focus_ring = palette.ring;
        }

        [[nodiscard]] auto validate_semantic_color(const ColorToken role, const NanColor& color)
            -> std::expected<void, ThemeColorError> {
            if (!finite_color(color)) {
                return std::unexpected(
                    ThemeColorError {.kind = ThemeColorErrorKind::non_finite_color, .role = role}
                );
            }
            return {};
        }
    } // namespace

    auto describe(const ThemeColorError& error) -> std::string_view {
        switch (error.kind) {
            case ThemeColorErrorKind::non_finite_color:
                return "semantic color has non-finite OKLCH components";
        }
        return "invalid semantic color";
    }

    auto canonical_color_token(const ColorToken role) noexcept -> ColorToken {
        switch (role) {
            case ColorToken::on_background:
                return ColorToken::foreground;
            case ColorToken::on_primary:
                return ColorToken::primary_foreground;
            case ColorToken::on_secondary:
                return ColorToken::secondary_foreground;
            case ColorToken::on_tertiary:
                return ColorToken::tertiary_foreground;
            case ColorToken::on_surface:
                return ColorToken::surface_foreground;
            case ColorToken::on_surface_variant:
                return ColorToken::surface_variant_foreground;
            case ColorToken::on_muted:
                return ColorToken::muted_foreground;
            case ColorToken::outline:
                return ColorToken::border;
            case ColorToken::outline_variant:
                return ColorToken::surface_variant;
            case ColorToken::on_success:
                return ColorToken::success_foreground;
            case ColorToken::on_warning:
                return ColorToken::warning_foreground;
            case ColorToken::on_error:
                return ColorToken::error_foreground;
            case ColorToken::on_info:
                return ColorToken::info_foreground;
            case ColorToken::focus_ring:
                return ColorToken::ring;
            default:
                return role;
        }
    }

    auto semantic_color(const NanColorScheme& palette, const ColorToken role) noexcept
        -> const NanColor& {
        return palette.*canonical_field(canonical_color_token(role));
    }

    auto set_semantic_color(NanColorScheme& palette, const ColorToken role, NanColor color)
        -> std::expected<void, ThemeColorError> {
        auto result = validate_semantic_color(role, color);
        if (!result) {
            return std::unexpected(result.error());
        }
        palette.*canonical_field(canonical_color_token(role)) = std::move(color);
        sync_color_aliases(palette);
        return {};
    }

    auto apply_semantic_colors(
        NanColorScheme& palette,
        const std::span<const SemanticColorOverride> overrides
    ) -> std::expected<void, ThemeColorError> {
        for (const auto& override: overrides) {
            auto result = validate_semantic_color(override.role, override.color);
            if (!result) {
                return std::unexpected(result.error());
            }
        }
        auto staged = palette;
        for (const auto& override: overrides) {
            staged.*canonical_field(canonical_color_token(override.role)) = override.color;
        }
        sync_color_aliases(staged);
        palette = std::move(staged);
        return {};
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
        const auto scheme = error.scheme && !error.scheme->text().empty()
            ? ": " + std::string(error.scheme->text())
            : std::string {};
        const auto family = error.family && !error.family->text().empty()
            ? ": " + std::string(error.family->text())
            : std::string {};
        switch (error.kind) {
            case ThemeCatalogErrorKind::invalid_scheme_id:
                return "invalid theme scheme id" + scheme;
            case ThemeCatalogErrorKind::invalid_family_id:
                return "invalid theme family id" + family;
            case ThemeCatalogErrorKind::duplicate_scheme:
                return "theme scheme is already registered" + scheme;
            case ThemeCatalogErrorKind::duplicate_family:
                return "theme family is already registered" + family;
            case ThemeCatalogErrorKind::scheme_not_found:
                return "theme scheme was not found" + scheme;
            case ThemeCatalogErrorKind::family_not_found:
                return "theme family was not found" + family;
            case ThemeCatalogErrorKind::scheme_in_use:
                return "theme scheme is used by the current selection" + scheme;
            case ThemeCatalogErrorKind::family_in_use:
                return "theme family still has registered schemes" + family;
            case ThemeCatalogErrorKind::null_content:
                return "theme scheme content is null" + scheme;
            case ThemeCatalogErrorKind::unknown_family:
                return "theme scheme references an unknown family: scheme" + scheme + ", family"
                    + family;
            case ThemeCatalogErrorKind::appearance_mismatch:
                return "theme scheme appearance conflicts with the current selection" + scheme;
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
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::invalid_family_id,
                    .family = family.id,
                }
            );
        }
        const auto key = std::string(family.id.text());
        if (families_.contains(key)) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::duplicate_family,
                    .family = family.id,
                }
            );
        }
        families_.emplace(key, std::move(family));
        ++version_;
        return {};
    }

    auto ThemeSchemeCatalog::remove_family(const ThemeFamilyId& id)
        -> std::expected<void, ThemeCatalogError> {
        const auto key = std::string(id.text());
        if (!families_.contains(key)) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::family_not_found,
                    .family = id,
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
                    .kind = ThemeCatalogErrorKind::family_in_use,
                    .family = id,
                }
            );
        }
        families_.erase(key);
        ++version_;
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

    auto ThemeSchemeCatalog::contains_family(const ThemeFamilyId& id) const -> bool {
        return families_.contains(std::string(id.text()));
    }

    auto ThemeSchemeCatalog::register_scheme(ThemeScheme scheme)
        -> std::expected<void, ThemeCatalogError> {
        if (!scheme.id.valid()) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::invalid_scheme_id,
                    .scheme = scheme.id,
                }
            );
        }
        if (!scheme.content) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::null_content,
                    .scheme = scheme.id,
                }
            );
        }
        if (scheme.family && !families_.contains(std::string(scheme.family->text()))) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::unknown_family,
                    .scheme = scheme.id,
                    .family = scheme.family,
                }
            );
        }
        const auto key = std::string(scheme.id.text());
        if (schemes_.contains(key)) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::duplicate_scheme,
                    .scheme = scheme.id,
                }
            );
        }
        schemes_.emplace(key, std::move(scheme));
        ++version_;
        return {};
    }

    auto ThemeSchemeCatalog::replace_scheme(ThemeScheme scheme)
        -> std::expected<void, ThemeCatalogError> {
        if (!scheme.id.valid()) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::invalid_scheme_id,
                    .scheme = scheme.id,
                }
            );
        }
        if (!scheme.content) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::null_content,
                    .scheme = scheme.id,
                }
            );
        }
        if (scheme.family && !families_.contains(std::string(scheme.family->text()))) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::unknown_family,
                    .scheme = scheme.id,
                    .family = scheme.family,
                }
            );
        }
        const auto key = std::string(scheme.id.text());
        if (!schemes_.contains(key)) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::scheme_not_found,
                    .scheme = scheme.id,
                }
            );
        }
        if (schemes_.at(key) == scheme) {
            return {}; // Equivalent directory writes do not invalidate observers or effects.
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
                        .scheme = scheme.id,
                    }
                );
            }
            resolved = std::move(*result);
        }
        schemes_ = std::move(staged.schemes_);
        ++version_;
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
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::scheme_not_found,
                    .scheme = id,
                }
            );
        }
        if (uses(id)) {
            return std::unexpected(
                ThemeCatalogError {
                    .kind = ThemeCatalogErrorKind::scheme_in_use,
                    .scheme = id,
                }
            );
        }
        schemes_.erase(key);
        ++version_;
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

    auto ThemeSchemeCatalog::version() const noexcept -> std::uint64_t {
        return version_;
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

    // ─── 统一作者入口：派生与结构校验 ─────────────────────────────────────────

    auto derive_content(const ThemeSchemeContent& base) -> ThemeSchemeContent {
        // 显式深拷贝：作者改派生内容不会污染基础方案；primitive 来源记录一并保留。
        return ThemeSchemeContent {
            base.tokens,
            base.palette,
            base.typography,
            base.components,
            base.reference,
        };
    }

    auto derive_scheme(const ThemeScheme& base, ThemeSchemeId id, std::string display_name)
        -> ThemeScheme {
        return ThemeScheme {
            .id = std::move(id),
            .display_name = std::move(display_name),
            .appearance = base.appearance,
            .family = base.family,
            .content = base.content
                ? std::make_shared<const ThemeSchemeContent>(derive_content(*base.content))
                : nullptr,
        };
    }

    auto validate_scheme(const ThemeScheme& scheme, const ThemeSchemeCatalog& catalog)
        -> std::expected<void, ThemeSchemeError> {
        if (!scheme.id.valid()) {
            return std::unexpected(ThemeSchemeError {.kind = ThemeSchemeErrorKind::invalid_id});
        }
        if (scheme.display_name.empty()) {
            return std::unexpected(
                ThemeSchemeError {
                    .kind = ThemeSchemeErrorKind::empty_display_name,
                    .id = scheme.id,
                }
            );
        }
        if (!scheme.content) {
            return std::unexpected(
                ThemeSchemeError {
                    .kind = ThemeSchemeErrorKind::null_content,
                    .id = scheme.id,
                }
            );
        }
        if (catalog.find(scheme.id) != nullptr) {
            return std::unexpected(
                ThemeSchemeError {
                    .kind = ThemeSchemeErrorKind::duplicate_id,
                    .id = scheme.id,
                }
            );
        }
        if (scheme.family && !catalog.contains_family(*scheme.family)) {
            return std::unexpected(
                ThemeSchemeError {
                    .kind = ThemeSchemeErrorKind::unknown_family,
                    .id = scheme.id,
                }
            );
        }
        return {};
    }

    auto describe(const ThemeSchemeError& error) -> std::string {
        const auto suffix = error.id.valid() ? ": " + std::string(error.id.text()) : std::string {};
        switch (error.kind) {
            case ThemeSchemeErrorKind::invalid_id:
                return "theme scheme id is invalid" + suffix;
            case ThemeSchemeErrorKind::empty_display_name:
                return "theme scheme display name must not be empty" + suffix;
            case ThemeSchemeErrorKind::null_content:
                return "theme scheme content is null" + suffix;
            case ThemeSchemeErrorKind::duplicate_id:
                return "theme scheme id is already registered" + suffix;
            case ThemeSchemeErrorKind::unknown_family:
                return "theme scheme references an unknown family" + suffix;
        }
        return "invalid theme scheme" + suffix;
    }

} // namespace nandina::theme
