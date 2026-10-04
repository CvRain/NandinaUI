// Legacy scene names and text inputs must use the same pure constraint type.

#include <nandina/scene/control.hpp>
#include <nandina/widget/primitives/text_layout.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <type_traits>

using namespace nandina;

static_assert(std::same_as<scene::LayoutConstraints, foundation::NanLayoutConstraints>);
static_assert(std::same_as<
              decltype(widget::primitives::TextLayoutInput {}.constraints),
              foundation::NanLayoutConstraints>);
static_assert(std::is_aggregate_v<scene::LayoutConstraints>);
static_assert(std::is_aggregate_v<widget::primitives::TextLayoutInput>);

namespace
{
    constexpr scene::LayoutConstraints positional {12.0F, 120.0F, 8.0F, 80.0F};
    static_assert(positional.min_width == 12.0F);
    static_assert(positional.max_width == 120.0F);
    static_assert(positional.min_height == 8.0F);
    static_assert(positional.max_height == 80.0F);

    static_assert(requires(scene::NanControl& control, foundation::NanLayoutConstraints limits) {
        { control.measure_layout(limits) } -> std::same_as<foundation::NanSize>;
        { control.layout_to(foundation::NanRect {}) } -> std::same_as<void>;
    });

    class LegacyMeasureProbe final: public scene::NanControl {
    protected:
        auto on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize override {
            return constraints.constrain(foundation::NanSize(96.0F, 24.0F));
        }
    };
} // namespace

TEST_CASE("legacy aggregate constraints remain usable in text inputs", "[layout][compat]") {
    const scene::LayoutConstraints
        legacy {.min_width = 12.0F, .max_width = 120.0F, .min_height = 8.0F, .max_height = 80.0F};
    const widget::primitives::TextLayoutInput input {.text = "Title", .constraints = legacy};
    REQUIRE(input.text == "Title");
    REQUIRE(input.constraints.min_width == Catch::Approx(12.0F));
    REQUIRE(input.constraints.max_width == Catch::Approx(120.0F));
    REQUIRE(input.constraints.min_height == Catch::Approx(8.0F));
    REQUIRE(input.constraints.max_height == Catch::Approx(80.0F));

    const widget::primitives::TextLayoutInput default_input {};
    const auto natural = default_input.constraints.constrain(foundation::NanSize(96.0F, 24.0F));
    REQUIRE(natural.get_width() == Catch::Approx(96.0F));
    REQUIRE(natural.get_height() == Catch::Approx(24.0F));
}

TEST_CASE(
    "legacy measurement overrides accept foundation constraints at the public interface",
    "[layout][compat]"
) {
    LegacyMeasureProbe content;
    const scene::LayoutConstraints legacy_limits {
        .min_width = 60.0F,
        .max_width = 100.0F,
        .min_height = 12.0F,
        .max_height = 36.0F
    };
    const auto natural = content.measure_layout(legacy_limits);
    REQUIRE(natural.get_width() == Catch::Approx(96.0F));
    REQUIRE(natural.get_height() == Catch::Approx(24.0F));

    const auto viewport =
        foundation::NanLayoutConstraints::tight(foundation::NanSize(144.0F, 44.0F));
    const auto allocated = content.measure_layout(viewport);
    REQUIRE(allocated.get_width() == Catch::Approx(144.0F));
    REQUIRE(allocated.get_height() == Catch::Approx(44.0F));
    content.layout_to(foundation::NanRect::from_xywh(10.0F, 20.0F, 144.0F, 44.0F));
    REQUIRE(content.position() == foundation::NanPoint(10.0F, 20.0F));
    REQUIRE(content.size() == allocated);
}
