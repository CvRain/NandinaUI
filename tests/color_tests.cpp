//
// Foundation / color tests.
//

#include "foundation/nandina_color.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace nandina;
using foundation::NanColor;
using foundation::NanHexRgb;
using foundation::NanRgb;

TEST_CASE("NanColor::from_hex unpacks 0xRRGGBB", "[foundation][color]") {
    const auto red = NanColor::from_hex(0xFF0000);
    const auto rgb = red.to<NanRgb>();
    REQUIRE(rgb.red == Catch::Approx(1.0F).margin(0.001F));
    REQUIRE(rgb.green == Catch::Approx(0.0F).margin(0.001F));
    REQUIRE(rgb.blue == Catch::Approx(0.0F).margin(0.001F));

    const auto black = NanColor::from_hex(0x000000);
    REQUIRE(black.oklch().light == Catch::Approx(0.0F).margin(0.001F));

    const auto white = NanColor::from_hex(0xFFFFFF);
    REQUIRE(white.oklch().light == Catch::Approx(1.0F).margin(0.001F));
}

TEST_CASE("NanColor::from_hex respects explicit alpha", "[foundation][color]") {
    const auto half = NanColor::from_hex(0x112233, 0.5F);
    REQUIRE(half.alpha() == Catch::Approx(0.5F));
}

TEST_CASE("NanHexRgb packs and unpacks 0xRRGGBBAA", "[foundation][color]") {
    constexpr std::uint32_t value = 0xFAF6EC80;
    const auto hex = NanHexRgb::from_uint32(value);
    REQUIRE(hex.red == 0xFA);
    REQUIRE(hex.green == 0xF6);
    REQUIRE(hex.blue == 0xEC);
    REQUIRE(hex.alpha == 0x80);
    REQUIRE(hex.packed() == value);
}

TEST_CASE("NanColor equality is exact and approximate", "[foundation][color]") {
    const auto a = NanColor::from_hex(0x112233);
    const auto b = NanColor::from_hex(0x112233);
    const auto c = NanColor::from_hex(0x112244);

    REQUIRE(a == b);
    REQUIRE(a != c);
    REQUIRE(a.approx_equals(b));
    REQUIRE_FALSE(a.approx_equals(c));
}

TEST_CASE("NanColor::approx_equals compares hue along the shortest arc", "[foundation][color]") {
    const auto near_zero = NanColor::from_oklch(0.5F, 0.1F, 359.999F);
    const auto zero = NanColor::from_oklch(0.5F, 0.1F, 0.001F);
    REQUIRE(near_zero.approx_equals(zero, 0.01F));
}

TEST_CASE("NanColor::mix interpolates hue along the shortest arc", "[foundation][color]") {
    const auto a = NanColor::from_oklch(0.5F, 0.1F, 350.0F);
    const auto b = NanColor::from_oklch(0.5F, 0.1F, 10.0F);
    const auto mid = a.mix(b, 0.5F);
    // 350° 与 10° 的最短弧经过 0°，而非 180°。
    REQUIRE(mid.oklch().hue == Catch::Approx(0.0F).margin(0.001F));
    REQUIRE(mid.oklch().light == Catch::Approx(0.5F));
}

TEST_CASE("NanColor::from_rgb and from_oklch match the generic factories", "[foundation][color]") {
    const auto red_rgb = NanColor::from_rgb(1.0F, 0.0F, 0.0F);
    const auto red_hex = NanColor::from_hex(0xFF0000);
    REQUIRE(red_rgb.approx_equals(red_hex, 0.01F));

    const auto oklch = NanColor::from_oklch(0.7F, 0.1F, 40.0F);
    REQUIRE(oklch.oklch().light == Catch::Approx(0.7F));
    REQUIRE(oklch.oklch().chroma == Catch::Approx(0.1F));
    REQUIRE(oklch.oklch().hue == Catch::Approx(40.0F));
}
