// Compatibility aliases must coexist with scene's canonical forward declaration.

#include <nandina/scene/node.hpp>
#include <nandina/widget/primitives/text_layout.hpp>
#include <nandina/widget/primitives/text_layout_backend.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <string>
#include <type_traits>

using namespace nandina;

static_assert(std::same_as<widget::primitives::TextAlign, text::TextAlign>);
static_assert(std::same_as<widget::primitives::TextOverflow, text::TextOverflow>);
static_assert(std::same_as<widget::primitives::TextAffinity, text::TextAffinity>);
static_assert(std::same_as<widget::primitives::TextStyle, text::TextStyle>);
static_assert(std::same_as<widget::primitives::TextLayoutInput, text::TextLayoutInput>);
static_assert(std::same_as<widget::primitives::TextCaretStop, text::TextCaretStop>);
static_assert(std::same_as<widget::primitives::TextLayoutLine, text::TextLayoutLine>);
static_assert(std::same_as<widget::primitives::TextLayoutResult, text::TextLayoutResult>);
static_assert(std::same_as<widget::primitives::ITextLayoutBackend, text::ITextLayoutBackend>);
static_assert(std::same_as<widget::primitives::ITextLayoutRenderer, text::ITextLayoutRenderer>);
static_assert(std::same_as<widget::primitives::TextPipeline, text::TextPipeline>);
static_assert(std::same_as<
              decltype(&widget::primitives::text_align_offset),
              decltype(&text::text_align_offset)>);
static_assert(std::same_as<
              decltype(&widget::primitives::glyph_overhang_allowance),
              decltype(&text::glyph_overhang_allowance)>);
static_assert(std::same_as<
              decltype(&widget::primitives::deterministic_text_layout_backend),
              decltype(&text::deterministic_text_layout_backend)>);
static_assert(std::same_as<widget::primitives::TextLayoutLine::Glyph, text::TextLayoutLine::Glyph>);
static_assert(requires(scene::NanNode& node, widget::primitives::TextPipeline pipeline) {
    node.apply_default_text_pipeline(pipeline);
});

namespace
{
    class LegacyBackend final: public widget::primitives::ITextLayoutBackend {
    public:
        mutable std::string observed_text;

        auto layout(widget::primitives::TextLayoutInput input) const
            -> widget::primitives::TextLayoutResult override {
            observed_text = std::string(input.text);
            return {.size = foundation::NanSize(23.0F, 11.0F)};
        }
    };

    class LegacyRenderer final: public widget::primitives::ITextLayoutRenderer {
    public:
        void draw(
            const widget::primitives::TextLayoutResult&,
            render::DrawContext&,
            foundation::NanPoint,
            foundation::NanColor
        ) override {}
    };

    static_assert(!std::is_abstract_v<LegacyBackend>);
    static_assert(!std::is_abstract_v<LegacyRenderer>);
} // namespace

TEST_CASE(
    "legacy text names share default instances and helper identities",
    "[text][header][compat]"
) {
    const widget::primitives::TextPipeline legacy;
    const text::TextPipeline canonical;
    REQUIRE(legacy.backend == canonical.backend);
    REQUIRE(legacy.backend == &widget::primitives::deterministic_text_layout_backend());
    REQUIRE(legacy.backend == &text::deterministic_text_layout_backend());
    REQUIRE(legacy.renderer == nullptr);
    REQUIRE(canonical.renderer == nullptr);
    REQUIRE(&widget::primitives::text_align_offset == &text::text_align_offset);
    REQUIRE(&widget::primitives::glyph_overhang_allowance == &text::glyph_overhang_allowance);
    REQUIRE(
        &widget::primitives::deterministic_text_layout_backend
        == &text::deterministic_text_layout_backend
    );
    const widget::primitives::TextStyle legacy_style;
    text::TextStyle canonical_style;
    REQUIRE(canonical_style.approx_equals(legacy_style));
    canonical_style.font_size = 20.0F;
    REQUIRE_FALSE(canonical_style.approx_equals(legacy_style));
}

TEST_CASE("legacy override signatures work in a canonical pipeline", "[text][header][compat]") {
    LegacyBackend backend;
    LegacyRenderer renderer;
    const text::TextPipeline pipeline {.backend = &backend, .renderer = &renderer};
    const widget::primitives::TextLayoutInput input {.text = "legacy"};
    const widget::primitives::TextLayoutResult result = pipeline.backend->layout(input);
    REQUIRE(backend.observed_text == "legacy");
    REQUIRE(result.size.get_width() == Catch::Approx(23.0F));
    REQUIRE(result.size.get_height() == Catch::Approx(11.0F));
    REQUIRE(pipeline.renderer == &renderer);
}
