// Legacy text names remain compatible with canonical styles and font-engine APIs.

#include <nandina/text/font_request.hpp>

#include <nandina/theme/style_context.hpp>

#include <nandina/text/font_family.hpp>

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <memory>

using namespace nandina;

static_assert(std::same_as<text::FontRequest, theme::FontRequest>);
static_assert(std::same_as<text::FontSlant, theme::FontSlant>);
static_assert(
    std::same_as<theme::StyleValue<text::FontRequest>, theme::StyleValue<theme::FontRequest>>
);
static_assert(
    std::same_as<decltype(theme::StyleContext {}.font), theme::StyleValue<theme::FontRequest>>
);
static_assert(std::same_as<decltype(theme::ResolvedStyleContext {}.font), theme::FontRequest>);

namespace
{
    using ResolveSignature =
        text::FontResult<text::ResolvedFontFamily> (text::FontFamilyRegistry::*)(
            const theme::FontRequest&,
            text::FontLoader&
        ) const;
    using RegisterFaceSignature = text::FontResult<void> (text::FontFamilyRegistry::*)(
        resource::ResourceKey,
        std::shared_ptr<text::FreeTypeFontFace>,
        int,
        theme::FontSlant
    );
    static_assert(std::same_as<decltype(&text::FontFamilyRegistry::resolve), ResolveSignature>);
    static_assert(
        std::same_as<decltype(&text::FontFamilyRegistry::register_face), RegisterFaceSignature>
    );
    static_assert(requires(
        text::FontFamilyRegistry& registry,
        const text::FontFamilyRegistry& const_registry,
        const theme::FontRequest& request,
        text::FontLoader& loader,
        resource::ResourceKey family,
        std::shared_ptr<text::FreeTypeFontFace> face
    ) {
        {
            const_registry.resolve(request, loader)
        } -> std::same_as<text::FontResult<text::ResolvedFontFamily>>;
        { registry.register_face(family, face) } -> std::same_as<text::FontResult<void>>;
        {
            registry.register_face(family, face, 500, theme::FontSlant::italic)
        } -> std::same_as<text::FontResult<void>>;
    });
} // namespace

TEST_CASE(
    "legacy font requests enter canonical StyleValue and style resolution",
    "[font-request][header][compat]"
) {
    const text::FontRequest legacy {
        .family = resource::ResourceKey("families/a"),
        .weight = 700,
        .slant = text::FontSlant::italic,
    };
    const theme::StyleValue<theme::FontRequest> canonical_value =
        theme::StyleValue<text::FontRequest>::explicit_value(legacy);
    theme::StyleContext context;
    context.font = canonical_value;
    const auto resolved = theme::resolve_style_context(context);
    REQUIRE(canonical_value.value() == legacy);
    REQUIRE(resolved.font == legacy);
    REQUIRE(resolved.font_from_context);
}

TEST_CASE(
    "legacy spelling preserves inherited and explicit font requests",
    "[font-request][header][compat]"
) {
    const text::FontRequest parent_font {
        .family = resource::ResourceKey("families/a"),
        .weight = 700,
        .slant = text::FontSlant::italic,
    };
    const theme::ResolvedStyleContext parent {.font = parent_font, .font_from_context = true};
    theme::StyleContext child;
    child.font = theme::StyleValue<text::FontRequest>::inherit();
    const auto inherited = theme::resolve_style_context(child, &parent);
    REQUIRE(inherited.font == parent_font);
    REQUIRE(inherited.font_from_context);

    const text::FontRequest child_font {
        .family = resource::ResourceKey("families/b"),
        .weight = 500,
        .slant = text::FontSlant::oblique,
    };
    child.font = theme::StyleValue<theme::FontRequest>::explicit_value(child_font);
    const auto explicit_child = theme::resolve_style_context(child, &parent);
    REQUIRE(explicit_child.font == child_font);
    REQUIRE(explicit_child.font_from_context);
}
