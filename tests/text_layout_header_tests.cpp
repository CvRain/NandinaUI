// The canonical text protocol must work without any scene or widget include.

#include <nandina/text/text_layout_backend.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <type_traits>

using namespace nandina;

namespace
{
    class CanonicalBackend final: public text::ITextLayoutBackend {
    public:
        mutable std::string observed_text;
        mutable int calls = 0;

        auto layout(text::TextLayoutInput input) const -> text::TextLayoutResult override {
            observed_text = std::string(input.text);
            ++calls;
            return {.size = foundation::NanSize(37.0F, 14.0F), .font_size = input.style.font_size};
        }
    };

    class CanonicalRenderer final: public text::ITextLayoutRenderer {
    public:
        void draw(
            const text::TextLayoutResult&,
            render::DrawContext&,
            foundation::NanPoint,
            foundation::NanColor
        ) override {}
    };

    static_assert(!std::is_abstract_v<CanonicalBackend>);
    static_assert(!std::is_abstract_v<CanonicalRenderer>);
} // namespace

TEST_CASE(
    "canonical text header provides the shared ASCII fallback and caret queries",
    "[text][header]"
) {
    const text::TextPipeline pipeline;
    REQUIRE(pipeline.backend == &text::deterministic_text_layout_backend());
    REQUIRE(pipeline.renderer == nullptr);
    text::TextLayoutInput input {.text = "CAT"};
    input.style.font_size = 10.0F;
    input.style.overflow = text::TextOverflow::clip;
    const auto result = pipeline.backend->layout(input);
    REQUIRE(result.size.get_width() == Catch::Approx(16.8F));
    REQUIRE(result.size.get_height() == Catch::Approx(12.0F));
    REQUIRE(result.font_size == Catch::Approx(10.0F));
    REQUIRE(result.baseline == Catch::Approx(10.0F));
    REQUIRE_FALSE(result.overflowed);
    REQUIRE(result.lines.size() == 1);
    const auto& line = result.lines.front();
    REQUIRE(line.text_offset == 0);
    REQUIRE(line.text_length == 3);
    REQUIRE(line.visible_text == "CAT");
    REQUIRE(line.caret_stops.size() == 4);
    const auto after_c = line.caret_for_source(1, text::TextAffinity::upstream);
    REQUIRE(after_c.source_offset == 1);
    REQUIRE(after_c.x == Catch::Approx(5.6F));
    REQUIRE(after_c.affinity == text::TextAffinity::upstream);
    const auto near_a = line.caret_for_x(10.8F);
    REQUIRE(near_a.source_offset == 2);
    REQUIRE(near_a.x == Catch::Approx(11.2F));
    const auto near_end = result.caret_for_point(foundation::NanPoint(15.0F, 6.0F));
    REQUIRE(near_end.source_offset == 3);
    REQUIRE(near_end.x == Catch::Approx(16.8F));
}

TEST_CASE(
    "canonical backend and renderer override the public protocol signatures",
    "[text][header]"
) {
    CanonicalBackend backend;
    CanonicalRenderer renderer;
    const text::TextPipeline pipeline {.backend = &backend, .renderer = &renderer};
    text::TextLayoutInput input {.text = "custom"};
    input.style.font_size = 18.0F;
    const auto result = pipeline.backend->layout(input);
    REQUIRE(backend.calls == 1);
    REQUIRE(backend.observed_text == "custom");
    REQUIRE(result.size.get_width() == Catch::Approx(37.0F));
    REQUIRE(result.size.get_height() == Catch::Approx(14.0F));
    REQUIRE(result.font_size == Catch::Approx(18.0F));
    REQUIRE(pipeline.renderer == &renderer);
}
