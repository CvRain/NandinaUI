//
// theme/style_document — styles.toml data compiled into the runtime theme objects.
//

#include "style_document.hpp"

#include <toml++/toml.hpp>

#include <array>
#include <cstdint>
#include <sstream>
#include <stdexcept>

namespace nandina::theme
{
    namespace
    {
        [[nodiscard]] auto number(const toml::node& node, std::string_view field) -> float {
            if (const auto value = node.value<double>()) {
                return static_cast<float>(*value);
            }
            if (const auto value = node.value<std::int64_t>()) {
                return static_cast<float>(*value);
            }
            throw std::runtime_error(std::string(field) + " must be a number");
        }

        [[nodiscard]] auto color(const toml::node& node, std::string_view field) -> NanColor {
            const auto* values = node.as_array();
            if (values == nullptr || (values->size() != 3 && values->size() != 4)) {
                throw std::runtime_error(
                    std::string(field) + " must be [light, chroma, hue] or include alpha"
                );
            }
            return nan_color(
                number(*values->get(0), field),
                number(*values->get(1), field),
                number(*values->get(2), field),
                values->size() == 4 ? number(*values->get(3), field) : 1.0F
            );
        }

        constexpr std::array<std::pair<std::string_view, ColorShade>, NanColorScale::stop_count>
            color_shades {{
                {"50", ColorShade::shade_50},
                {"100", ColorShade::shade_100},
                {"200", ColorShade::shade_200},
                {"300", ColorShade::shade_300},
                {"400", ColorShade::shade_400},
                {"500", ColorShade::shade_500},
                {"600", ColorShade::shade_600},
                {"700", ColorShade::shade_700},
                {"800", ColorShade::shade_800},
                {"900", ColorShade::shade_900},
                {"950", ColorShade::shade_950},
            }};

        [[nodiscard]] auto parse_color_scale(const toml::table& table, std::string_view field)
            -> NanColorScale {
            NanColorScale scale;
            for (const auto& [name, shade]: color_shades) {
                const auto* node = table.get(name);
                if (node == nullptr) {
                    throw std::runtime_error(
                        std::string(field) + " requires color stop " + std::string(name)
                    );
                }
                scale.at(shade) = color(*node, field);
            }
            return scale;
        }

        void parse_reference_scale(
            const toml::table& table,
            std::string_view name,
            NanColorScale& scale,
            std::string_view field
        ) {
            if (const auto* values = table[name].as_table()) {
                scale = parse_color_scale(*values, field);
            }
        }

        [[nodiscard]] auto parse_reference_palette(const toml::table& table, std::string_view field)
            -> NanReferencePalette {
            NanReferencePalette palette;
            parse_reference_scale(table, "primary", palette.primary, field);
            parse_reference_scale(table, "secondary", palette.secondary, field);
            parse_reference_scale(table, "tertiary", palette.tertiary, field);
            parse_reference_scale(table, "neutral", palette.neutral, field);
            parse_reference_scale(table, "success", palette.success, field);
            parse_reference_scale(table, "warning", palette.warning, field);
            parse_reference_scale(table, "error", palette.error, field);
            return palette;
        }

        [[nodiscard]] auto
        reference_scale(const NanReferencePalette& palette, std::string_view name)
            -> const NanColorScale* {
            if (name == "primary")
                return &palette.primary;
            if (name == "secondary")
                return &palette.secondary;
            if (name == "tertiary")
                return &palette.tertiary;
            if (name == "neutral")
                return &palette.neutral;
            if (name == "success")
                return &palette.success;
            if (name == "warning")
                return &palette.warning;
            if (name == "error")
                return &palette.error;
            return nullptr;
        }

        [[nodiscard]] auto color_shade(std::string_view name) -> std::optional<ColorShade> {
            for (const auto& [candidate, shade]: color_shades) {
                if (candidate == name)
                    return shade;
            }
            return std::nullopt;
        }

        [[nodiscard]] auto palette_reference(
            std::string_view value,
            const std::map<std::string, NanReferencePalette, std::less<>>& palettes
        ) -> std::optional<NanColor> {
            constexpr std::string_view prefix = "$palettes.";
            if (!value.starts_with(prefix))
                return std::nullopt;
            value.remove_prefix(prefix.size());
            const auto first = value.find('.');
            const auto second = first == std::string_view::npos ? std::string_view::npos
                                                                : value.find('.', first + 1);
            if (first == std::string_view::npos || second == std::string_view::npos) {
                throw std::runtime_error(
                    "palette reference must include palette, scale, and shade"
                );
            }
            const auto found = palettes.find(value.substr(0, first));
            if (found == palettes.end())
                throw std::runtime_error("palette reference uses an unknown palette");
            const auto* scale =
                reference_scale(found->second, value.substr(first + 1, second - first - 1));
            const auto shade = color_shade(value.substr(second + 1));
            if (scale == nullptr || !shade)
                throw std::runtime_error("palette reference uses an unknown scale or shade");
            return scale->at(*shade);
        }

        void assign_color(
            const toml::table& table,
            std::string_view key,
            NanColor& target,
            const std::map<std::string, NanReferencePalette, std::less<>>& palettes
        ) {
            if (const auto* node = table.get(key)) {
                if (const auto reference = node->value<std::string_view>()) {
                    const auto resolved = palette_reference(*reference, palettes);
                    if (!resolved)
                        throw std::runtime_error(
                            std::string(key) + " has an unknown color reference"
                        );
                    target = *resolved;
                }
                else {
                    target = color(*node, key);
                }
            }
        }

        void assign_scalar(const toml::table& table, std::string_view key, float& target) {
            if (const auto* node = table.get(key)) {
                target = number(*node, key);
            }
        }

        void parse_palette(
            const toml::table& table,
            NanColorScheme& palette,
            const std::map<std::string, NanReferencePalette, std::less<>>& palettes
        ) {
            assign_color(table, "background", palette.background, palettes);
            assign_color(table, "on_background", palette.on_background, palettes);
            assign_color(table, "primary", palette.primary, palettes);
            assign_color(table, "on_primary", palette.on_primary, palettes);
            assign_color(table, "secondary", palette.secondary, palettes);
            assign_color(table, "on_secondary", palette.on_secondary, palettes);
            assign_color(table, "tertiary", palette.tertiary, palettes);
            assign_color(table, "on_tertiary", palette.on_tertiary, palettes);
            assign_color(table, "surface", palette.surface, palettes);
            assign_color(table, "on_surface", palette.on_surface, palettes);
            assign_color(table, "surface_variant", palette.surface_variant, palettes);
            assign_color(table, "on_surface_variant", palette.on_surface_variant, palettes);
            assign_color(table, "outline", palette.outline, palettes);
            assign_color(table, "outline_variant", palette.outline_variant, palettes);
            assign_color(table, "success", palette.success, palettes);
            assign_color(table, "on_success", palette.on_success, palettes);
            assign_color(table, "warning", palette.warning, palettes);
            assign_color(table, "on_warning", palette.on_warning, palettes);
            assign_color(table, "error", palette.error, palettes);
            assign_color(table, "on_error", palette.on_error, palettes);
            assign_color(table, "focus_ring", palette.focus_ring, palettes);
            assign_color(table, "selection", palette.selection, palettes);
        }

        void parse_tokens(const toml::table& table, NanTokens& tokens) {
            if (const auto* values = table["spacing"].as_table()) {
                assign_scalar(*values, "xs", tokens.spacing.xs);
                assign_scalar(*values, "sm", tokens.spacing.sm);
                assign_scalar(*values, "md", tokens.spacing.md);
                assign_scalar(*values, "lg", tokens.spacing.lg);
                assign_scalar(*values, "xl", tokens.spacing.xl);
            }
            if (const auto* values = table["radius"].as_table()) {
                assign_scalar(*values, "sm", tokens.radius.sm);
                assign_scalar(*values, "md", tokens.radius.md);
                assign_scalar(*values, "lg", tokens.radius.lg);
                assign_scalar(*values, "full", tokens.radius.full);
            }
            if (const auto* values = table["border"].as_table()) {
                assign_scalar(*values, "thin", tokens.border.thin);
                assign_scalar(*values, "medium", tokens.border.medium);
                assign_scalar(*values, "focus_ring", tokens.border.focus_ring);
            }
            if (const auto* values = table["opacity"].as_table()) {
                assign_scalar(*values, "disabled", tokens.opacity.disabled);
                assign_scalar(*values, "hover_overlay", tokens.opacity.hover_overlay);
                assign_scalar(*values, "pressed_overlay", tokens.opacity.pressed_overlay);
            }
            if (const auto* values = table["typography"].as_table()) {
                assign_scalar(*values, "label_sm", tokens.typography.label_sm);
                assign_scalar(*values, "label_md", tokens.typography.label_md);
                assign_scalar(*values, "label_lg", tokens.typography.label_lg);
            }
        }

        [[nodiscard]] auto color_token(std::string_view value) -> std::optional<ColorToken> {
            if (value == "$palette.background")
                return ColorToken::background;
            if (value == "$palette.on_background")
                return ColorToken::on_background;
            if (value == "$palette.primary")
                return ColorToken::primary;
            if (value == "$palette.on_primary")
                return ColorToken::on_primary;
            if (value == "$palette.secondary")
                return ColorToken::secondary;
            if (value == "$palette.on_secondary")
                return ColorToken::on_secondary;
            if (value == "$palette.tertiary")
                return ColorToken::tertiary;
            if (value == "$palette.on_tertiary")
                return ColorToken::on_tertiary;
            if (value == "$palette.surface")
                return ColorToken::surface;
            if (value == "$palette.on_surface")
                return ColorToken::on_surface;
            if (value == "$palette.surface_variant")
                return ColorToken::surface_variant;
            if (value == "$palette.on_surface_variant")
                return ColorToken::on_surface_variant;
            if (value == "$palette.outline")
                return ColorToken::outline;
            if (value == "$palette.outline_variant")
                return ColorToken::outline_variant;
            if (value == "$palette.success")
                return ColorToken::success;
            if (value == "$palette.on_success")
                return ColorToken::on_success;
            if (value == "$palette.warning")
                return ColorToken::warning;
            if (value == "$palette.on_warning")
                return ColorToken::on_warning;
            if (value == "$palette.error")
                return ColorToken::error;
            if (value == "$palette.on_error")
                return ColorToken::on_error;
            if (value == "$palette.focus_ring")
                return ColorToken::focus_ring;
            if (value == "$palette.selection")
                return ColorToken::selection;
            return std::nullopt;
        }

        [[nodiscard]] auto scalar_token(std::string_view value) -> std::optional<ScalarToken> {
            if (value == "$tokens.spacing.xs")
                return ScalarToken::spacing_xs;
            if (value == "$tokens.spacing.sm")
                return ScalarToken::spacing_sm;
            if (value == "$tokens.spacing.md")
                return ScalarToken::spacing_md;
            if (value == "$tokens.spacing.lg")
                return ScalarToken::spacing_lg;
            if (value == "$tokens.spacing.xl")
                return ScalarToken::spacing_xl;
            if (value == "$tokens.radius.sm")
                return ScalarToken::radius_sm;
            if (value == "$tokens.radius.md")
                return ScalarToken::radius_md;
            if (value == "$tokens.radius.lg")
                return ScalarToken::radius_lg;
            if (value == "$tokens.radius.full")
                return ScalarToken::radius_full;
            if (value == "$tokens.border.thin")
                return ScalarToken::border_thin;
            if (value == "$tokens.border.medium")
                return ScalarToken::border_medium;
            if (value == "$tokens.border.focus_ring")
                return ScalarToken::border_focus_ring;
            if (value == "$tokens.opacity.disabled")
                return ScalarToken::opacity_disabled;
            if (value == "$tokens.opacity.hover_overlay")
                return ScalarToken::opacity_hover_overlay;
            if (value == "$tokens.opacity.pressed_overlay")
                return ScalarToken::opacity_pressed_overlay;
            if (value == "$tokens.typography.label_sm")
                return ScalarToken::typography_label_sm;
            if (value == "$tokens.typography.label_md")
                return ScalarToken::typography_label_md;
            if (value == "$tokens.typography.label_lg")
                return ScalarToken::typography_label_lg;
            if (value == "$tokens.motion.short_duration")
                return ScalarToken::motion_short_duration;
            if (value == "$tokens.motion.medium_duration")
                return ScalarToken::motion_medium_duration;
            if (value == "$tokens.motion.long_duration")
                return ScalarToken::motion_long_duration;
            return std::nullopt;
        }

        [[nodiscard]] auto theme_color(const toml::node& node, std::string_view field)
            -> ThemeColor {
            if (const auto reference = node.value<std::string_view>()) {
                if (const auto token = color_token(*reference)) {
                    return ThemeColor::token(*token);
                }
                throw std::runtime_error(std::string(field) + " has an unknown color token");
            }
            return ThemeColor::literal(color(node, field));
        }

        [[nodiscard]] auto theme_scalar(const toml::node& node, std::string_view field)
            -> ThemeScalar {
            if (const auto reference = node.value<std::string_view>()) {
                if (const auto token = scalar_token(*reference)) {
                    return ThemeScalar::token(*token);
                }
                throw std::runtime_error(std::string(field) + " has an unknown scalar token");
            }
            return ThemeScalar::literal(number(node, field));
        }

        template<typename Enum>
        [[nodiscard]] auto required_enum(
            const toml::table& table,
            std::string_view key,
            const std::map<std::string_view, Enum>& values
        ) -> std::optional<Enum> {
            const auto* node = table.get(key);
            if (node == nullptr)
                return std::nullopt;
            const auto value = node->value<std::string_view>();
            if (!value || !values.contains(*value)) {
                throw std::runtime_error(std::string(key) + " has an unknown value");
            }
            return values.at(*value);
        }

        [[nodiscard]] auto parse_button_rule(const toml::table& table) -> ButtonStyleRule {
            ButtonStyleRule rule;
            rule.selector.tone = required_enum(
                table,
                "tone",
                std::map<std::string_view, ButtonTone> {
                    {"primary", ButtonTone::primary},
                    {"secondary", ButtonTone::secondary},
                    {"neutral", ButtonTone::neutral},
                    {"danger", ButtonTone::danger},
                }
            );
            rule.selector.treatment = required_enum(
                table,
                "treatment",
                std::map<std::string_view, ButtonTreatment> {
                    {"filled", ButtonTreatment::filled},
                    {"tonal", ButtonTreatment::tonal},
                    {"outlined", ButtonTreatment::outlined},
                    {"ghost", ButtonTreatment::ghost},
                    {"link", ButtonTreatment::link},
                }
            );
            rule.selector.size = required_enum(
                table,
                "size",
                std::map<std::string_view, ButtonSize> {
                    {"small", ButtonSize::small},
                    {"medium", ButtonSize::medium},
                    {"large", ButtonSize::large},
                }
            );
            rule.selector.state = required_enum(
                table,
                "state",
                std::map<std::string_view, ButtonVisualState> {
                    {"normal", ButtonVisualState::normal},
                    {"hovered", ButtonVisualState::hovered},
                    {"pressed", ButtonVisualState::pressed},
                    {"focused", ButtonVisualState::focused},
                    {"disabled", ButtonVisualState::disabled},
                }
            );

            if (const auto* value = table.get("background"))
                rule.background = theme_color(*value, "background");
            if (const auto* value = table.get("foreground"))
                rule.foreground = theme_color(*value, "foreground");
            if (const auto* value = table.get("border_color"))
                rule.border_color = theme_color(*value, "border_color");
            if (const auto* value = table.get("focus_ring_color"))
                rule.focus_ring_color = theme_color(*value, "focus_ring_color");
            if (const auto* value = table.get("border_width"))
                rule.border_width = theme_scalar(*value, "border_width");
            if (const auto* value = table.get("radius"))
                rule.radius = theme_scalar(*value, "radius");
            if (const auto* value = table.get("focus_ring_width"))
                rule.focus_ring_width = theme_scalar(*value, "focus_ring_width");
            if (const auto* value = table.get("height"))
                rule.height = theme_scalar(*value, "height");
            if (const auto* value = table.get("padding_x"))
                rule.padding_x = theme_scalar(*value, "padding_x");
            if (const auto* value = table.get("font_size"))
                rule.font_size = theme_scalar(*value, "font_size");
            return rule;
        }

        [[nodiscard]] auto parse_text_field_rule(const toml::table& table) -> TextFieldStyleRule {
            TextFieldStyleRule rule;
            rule.state = required_enum(
                table,
                "state",
                std::map<std::string_view, TextFieldVisualState> {
                    {"normal", TextFieldVisualState::normal},
                    {"focused", TextFieldVisualState::focused},
                    {"disabled", TextFieldVisualState::disabled},
                    {"invalid", TextFieldVisualState::invalid},
                }
            );
            if (const auto* value = table.get("background"))
                rule.background = theme_color(*value, "background");
            if (const auto* value = table.get("foreground"))
                rule.foreground = theme_color(*value, "foreground");
            if (const auto* value = table.get("placeholder"))
                rule.placeholder = theme_color(*value, "placeholder");
            if (const auto* value = table.get("border_color"))
                rule.border_color = theme_color(*value, "border_color");
            if (const auto* value = table.get("focus_ring_color"))
                rule.focus_ring_color = theme_color(*value, "focus_ring_color");
            if (const auto* value = table.get("selection"))
                rule.selection = theme_color(*value, "selection");
            if (const auto* value = table.get("border_width"))
                rule.border_width = theme_scalar(*value, "border_width");
            if (const auto* value = table.get("radius"))
                rule.radius = theme_scalar(*value, "radius");
            if (const auto* value = table.get("focus_ring_width"))
                rule.focus_ring_width = theme_scalar(*value, "focus_ring_width");
            if (const auto* value = table.get("height"))
                rule.height = theme_scalar(*value, "height");
            if (const auto* value = table.get("padding_x"))
                rule.padding_x = theme_scalar(*value, "padding_x");
            if (const auto* value = table.get("font_size"))
                rule.font_size = theme_scalar(*value, "font_size");
            return rule;
        }

        [[nodiscard]] auto parse_font_family(const toml::table& table) -> FontFamilyDeclaration {
            const auto name = table["name"].value<std::string>();
            if (!name)
                throw std::runtime_error("font family requires name");
            FontFamilyDeclaration declaration {.family = resource::ResourceKey(*name)};
            declaration.is_default = table["default"].value_or(false);
            if (const auto* fallbacks = table["fallbacks"].as_array()) {
                for (const auto& node: *fallbacks) {
                    const auto fallback = node.value<std::string>();
                    if (!fallback)
                        throw std::runtime_error("font fallback must be a string");
                    declaration.fallbacks.emplace_back(*fallback);
                }
            }
            const auto* faces = table["faces"].as_array();
            if (faces == nullptr || faces->empty()) {
                throw std::runtime_error("font family requires at least one face");
            }
            for (const auto& node: *faces) {
                const auto* face = node.as_table();
                if (face == nullptr)
                    throw std::runtime_error("font face must be a table");
                const auto resource = (*face)["resource"].value<std::string>();
                if (!resource)
                    throw std::runtime_error("font face requires resource");
                text::FontSlant slant = text::FontSlant::normal;
                if (const auto value = (*face)["slant"].value<std::string_view>()) {
                    if (*value == "italic")
                        slant = text::FontSlant::italic;
                    else if (*value == "oblique")
                        slant = text::FontSlant::oblique;
                    else if (*value != "normal")
                        throw std::runtime_error("unknown font slant");
                }
                declaration.faces.push_back({
                    .resource = resource::ResourceKey(*resource),
                    .face_index = static_cast<std::uint32_t>((*face)["face_index"].value_or(0)),
                    .weight = static_cast<int>((*face)["weight"].value_or(400)),
                    .slant = slant,
                });
            }
            return declaration;
        }
    } // namespace

    auto StyleDocument::apply(ThemeManager& manager, text::FontFamilyRegistry* fonts) const
        -> std::expected<void, std::string> {
        for (const auto& [name, value]: themes) {
            (void)manager.register_theme(name, value);
        }
        try {
            for (const auto& [name, family]: theme_families) {
                (void)manager.register_family(name, family.light_theme, family.dark_theme);
            }
        }
        catch (const std::invalid_argument& error) {
            return std::unexpected(error.what());
        }
        manager.set_style(style);
        if (!active_family.empty()) {
            manager.set_preference(preference);
            if (!manager.activate_family(active_family)) {
                return std::unexpected(
                    "styles document selects an unknown theme family: " + active_family
                );
            }
        }
        else if (!manager.activate(active_theme)) {
            return std::unexpected("styles document selects an unknown theme: " + active_theme);
        }
        if (fonts == nullptr) {
            return {};
        }
        for (const auto& declaration: font_families) {
            if (auto result = fonts->register_family(declaration.family, declaration.faces);
                !result)
            {
                return std::unexpected(result.error().message);
            }
        }
        for (const auto& declaration: font_families) {
            if (!declaration.fallbacks.empty()) {
                if (auto result = fonts->set_fallbacks(declaration.family, declaration.fallbacks);
                    !result)
                {
                    return std::unexpected(result.error().message);
                }
            }
            if (declaration.is_default) {
                if (auto result = fonts->set_default_family(declaration.family); !result) {
                    return std::unexpected(result.error().message);
                }
            }
        }
        return {};
    }

    auto parse_style_document(const std::string_view source, const std::string_view source_name)
        -> std::expected<StyleDocument, std::string> {
        try {
            const auto root = toml::parse(source, source_name);
            StyleDocument document;
            document.active_theme = root["active_theme"].value_or(std::string("default"));
            document.active_family = root["active_family"].value_or(std::string {});
            if (const auto appearance = root["appearance"].value<std::string_view>()) {
                if (*appearance == "system")
                    document.preference = ThemePreference::system;
                else if (*appearance == "light")
                    document.preference = ThemePreference::light;
                else if (*appearance == "dark")
                    document.preference = ThemePreference::dark;
                else
                    throw std::runtime_error("appearance must be system, light, or dark");
            }
            if (const auto* palettes = root["palettes"].as_table()) {
                for (const auto& [name, node]: *palettes) {
                    const auto* table = node.as_table();
                    if (table == nullptr)
                        throw std::runtime_error("reference palette must be a table");
                    document.reference_palettes.emplace(
                        std::string(name.str()),
                        parse_reference_palette(*table, name.str())
                    );
                }
            }
            if (const auto* themes = root["themes"].as_table()) {
                for (const auto& [name, node]: *themes) {
                    const auto* table = node.as_table();
                    if (table == nullptr)
                        throw std::runtime_error("theme must be a table");
                    auto value = default_theme();
                    if (const auto* palette = (*table)["palette"].as_table()) {
                        parse_palette(*palette, value.palette, document.reference_palettes);
                    }
                    if (const auto* tokens = (*table)["tokens"].as_table()) {
                        parse_tokens(*tokens, value.tokens);
                    }
                    document.themes.emplace(std::string(name.str()), std::move(value));
                }
            }
            if (const auto* families = root["theme_families"].as_table()) {
                for (const auto& [name, node]: *families) {
                    const auto* table = node.as_table();
                    if (table == nullptr)
                        throw std::runtime_error("theme family must be a table");
                    const auto light = (*table)["light"].value<std::string>();
                    const auto dark = (*table)["dark"].value<std::string>();
                    if (!light || !dark)
                        throw std::runtime_error("theme family requires light and dark variants");
                    document.theme_families.emplace(
                        std::string(name.str()),
                        ThemeFamily {.light_theme = *light, .dark_theme = *dark}
                    );
                }
            }
            if (const auto* rules = root["styles"]["button"].as_array()) {
                for (const auto& node: *rules) {
                    const auto* table = node.as_table();
                    if (table == nullptr)
                        throw std::runtime_error("button rule must be a table");
                    document.style->add_button_rule(parse_button_rule(*table));
                }
            }
            if (const auto* rules = root["styles"]["text_field"].as_array()) {
                for (const auto& node: *rules) {
                    const auto* table = node.as_table();
                    if (table == nullptr) {
                        throw std::runtime_error("text_field rule must be a table");
                    }
                    document.style->add_text_field_rule(parse_text_field_rule(*table));
                }
            }
            if (const auto* families = root["fonts"]["family"].as_array()) {
                for (const auto& node: *families) {
                    const auto* table = node.as_table();
                    if (table == nullptr)
                        throw std::runtime_error("font family must be a table");
                    document.font_families.push_back(parse_font_family(*table));
                }
            }
            return document;
        }
        catch (const toml::parse_error& error) {
            std::ostringstream message;
            message << error;
            return std::unexpected(message.str());
        }
        catch (const std::exception& error) {
            return std::unexpected(std::string(source_name) + ": " + error.what());
        }
    }

    auto load_style_document(const std::filesystem::path& path)
        -> std::expected<StyleDocument, std::string> {
        try {
            const auto root = toml::parse_file(path.string());
            std::ostringstream source;
            source << root;
            return parse_style_document(source.str(), path.string());
        }
        catch (const toml::parse_error& error) {
            std::ostringstream message;
            message << error;
            return std::unexpected(message.str());
        }
    }

} // namespace nandina::theme
