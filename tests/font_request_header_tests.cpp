// Font descriptions remain usable through the lightweight canonical header alone.

#include <nandina/theme/font_request.hpp>

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <optional>
#include <type_traits>

using namespace nandina;

static_assert(std::is_aggregate_v<theme::FontRequest>);
static_assert(
    std::same_as<decltype(theme::FontRequest {}.family), std::optional<resource::ResourceKey>>
);
static_assert(std::same_as<decltype(theme::FontRequest {}.weight), int>);
static_assert(std::same_as<decltype(theme::FontRequest {}.slant), theme::FontSlant>);
static_assert(static_cast<int>(theme::FontSlant::normal) == 0);
static_assert(static_cast<int>(theme::FontSlant::italic) == 1);
static_assert(static_cast<int>(theme::FontSlant::oblique) == 2);

TEST_CASE(
    "font request defaults preserve empty family regular weight and normal slant",
    "[theme][font-request][header]"
) {
    const theme::FontRequest request;
    REQUIRE_FALSE(request.family.has_value());
    REQUIRE(request.weight == 400);
    REQUIRE(request.slant == theme::FontSlant::normal);
}

TEST_CASE(
    "font requests retain positional designated and copy initialization",
    "[theme][font-request][header]"
) {
    const theme::FontRequest positional {
        resource::ResourceKey("families/a"),
        600,
        theme::FontSlant::italic
    };
    const theme::FontRequest designated {
        .family = resource::ResourceKey("families/a"),
        .weight = 600,
        .slant = theme::FontSlant::italic,
    };
    REQUIRE(positional == designated);
    REQUIRE(positional.family.has_value());
    REQUIRE(positional.family->value() == "families/a");
    REQUIRE(positional.weight == 600);
    REQUIRE(positional.slant == theme::FontSlant::italic);
    const auto copied = positional;
    REQUIRE(copied == designated);
}

TEST_CASE("changing any font request field changes its value", "[theme][font-request][header]") {
    const theme::FontRequest original {
        .family = resource::ResourceKey("families/a"),
        .weight = 400,
        .slant = theme::FontSlant::normal,
    };
    auto changed = original;
    SECTION("family") {
        changed.family = resource::ResourceKey("families/b");
    }
    SECTION("weight") {
        changed.weight = 700;
    }
    SECTION("slant") {
        changed.slant = theme::FontSlant::italic;
    }
    REQUIRE(changed != original);
}

TEST_CASE(
    "font request family keys determine equality and take ordering priority",
    "[theme][font-request][header]"
) {
    const theme::FontRequest first {.family = resource::ResourceKey("families/a")};
    const theme::FontRequest same_family {.family = resource::ResourceKey("families/a")};
    const theme::FontRequest other_family {.family = resource::ResourceKey("families/b")};
    REQUIRE(first == same_family);
    REQUIRE(first != other_family);
    REQUIRE(first < other_family);
    const theme::FontRequest family_a_heavy {
        .family = resource::ResourceKey("families/a"),
        .weight = 900,
        .slant = theme::FontSlant::oblique,
    };
    const theme::FontRequest family_b_light {
        .family = resource::ResourceKey("families/b"),
        .weight = 100,
        .slant = theme::FontSlant::normal,
    };
    REQUIRE(family_a_heavy < family_b_light);
    REQUIRE(theme::FontRequest {} < first);
}

TEST_CASE(
    "font request weight and slant ordering retain unbounded assigned weights",
    "[theme][font-request][header]"
) {
    const auto family = resource::ResourceKey("families/a");
    theme::FontRequest light {.family = family, .slant = theme::FontSlant::oblique};
    theme::FontRequest heavy {.family = family};
    light.weight = -1;
    heavy.weight = 1200;
    REQUIRE(light.weight == -1);
    REQUIRE(heavy.weight == 1200);
    REQUIRE(light < heavy);
    const theme::FontRequest normal {family, 400, theme::FontSlant::normal};
    const theme::FontRequest italic {family, 400, theme::FontSlant::italic};
    const theme::FontRequest oblique {family, 400, theme::FontSlant::oblique};
    REQUIRE(normal < italic);
    REQUIRE(italic < oblique);
    REQUIRE(oblique > normal);
}
