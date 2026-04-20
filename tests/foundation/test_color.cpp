#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>

import nandina.foundation.color;

namespace {
    constexpr auto kChannelTolerance = 2;
    constexpr auto kFloatTolerance = 1.0e-3f;
}

TEST(NandinaColor, FromRgbRoundTripsToRgb) {
    const auto color = nandina::NanColor::from(nandina::NanRgb{170u, 173u, 111u, 200u});
    const auto rgb = color.to<nandina::NanRgb>();

    EXPECT_NEAR(rgb.red(), 170u, kChannelTolerance);
    EXPECT_NEAR(rgb.green(), 173u, kChannelTolerance);
    EXPECT_NEAR(rgb.blue(), 111u, kChannelTolerance);
    EXPECT_NEAR(rgb.alpha(), 200u, kChannelTolerance);
}

TEST(NandinaColor, PublicConvertApiConvertsBetweenColorSpaces) {
    const auto rgb = nandina::NanRgb{35u, 38u, 52u};
    const auto oklch = nandina::color::convert<nandina::NanRgb, nandina::NanOklch>(rgb);
    const auto round_trip = nandina::color::convert<nandina::NanOklch, nandina::NanRgb>(oklch);

    EXPECT_NEAR(oklch.lightness(), 0.2720f, 5.0e-3f);
    EXPECT_NEAR(oklch.chroma(), 0.0264f, 5.0e-3f);
    EXPECT_NEAR(oklch.hue(), 275.1f, 0.5f);
    EXPECT_NEAR(round_trip.red(), 35u, kChannelTolerance);
    EXPECT_NEAR(round_trip.green(), 38u, kChannelTolerance);
    EXPECT_NEAR(round_trip.blue(), 52u, kChannelTolerance);
}

TEST(NandinaColor, SetProxyAcceptsRgbAssignment) {
    auto color = nandina::NanColor{};
    color.set<nandina::NanRgb>() = nandina::NanRgb{170u, 173u, 111u, 200u};

    const auto rgb = color.to<nandina::NanRgb>();
    EXPECT_NEAR(rgb.red(), 170u, kChannelTolerance);
    EXPECT_NEAR(rgb.green(), 173u, kChannelTolerance);
    EXPECT_NEAR(rgb.blue(), 111u, kChannelTolerance);
    EXPECT_NEAR(rgb.alpha(), 200u, kChannelTolerance);
}

TEST(NandinaColor, ConvertsToOklch) {
    const auto color = nandina::NanColor::from(nandina::NanRgb{170u, 173u, 111u, 200u});
    const auto oklch = color.to<nandina::NanOklch>();

    EXPECT_GT(oklch.lightness(), 0.0f);
    EXPECT_GT(oklch.chroma(), 0.0f);
    EXPECT_GE(oklch.hue(), 0.0f);
    EXPECT_LT(oklch.hue(), 360.0f);
    EXPECT_NEAR(oklch.alpha(), 200.0f / 255.0f, kFloatTolerance);
}

TEST(NandinaColor, OklchRawRoundTripMatchesCrustRgb) {
    const auto color = nandina::NanColor::from(nandina::NanRgb{35u, 38u, 52u});
    const auto oklch = color.to<nandina::NanOklch>();
    const auto round_trip = nandina::NanColor::from(oklch).to<nandina::NanRgb>();

    EXPECT_NEAR(oklch.lightness(), 0.2720f, 5.0e-3f);
    EXPECT_NEAR(oklch.chroma(), 0.0264f, 5.0e-3f);
    EXPECT_NEAR(oklch.hue(), 275.1f, 0.5f);
    EXPECT_NEAR(round_trip.red(), 35u, kChannelTolerance);
    EXPECT_NEAR(round_trip.green(), 38u, kChannelTolerance);
    EXPECT_NEAR(round_trip.blue(), 52u, kChannelTolerance);
}

TEST(NandinaColor, OklchCssFactoryUsesMdnPercentageSemantics) {
    const auto color = nandina::NanColor::from(nandina::NanOklch::from_css(27.0f, 3.0f, 275.0f));
    const auto rgb = color.to<nandina::NanRgb>();

    EXPECT_NEAR(rgb.red(), 37u, kChannelTolerance);
    EXPECT_NEAR(rgb.green(), 38u, kChannelTolerance);
    EXPECT_NEAR(rgb.blue(), 44u, kChannelTolerance);
    EXPECT_EQ(rgb.alpha(), static_cast<std::uint8_t>(255u));
}

TEST(NandinaColor, HslConversionPreservesOpaqueRed) {
    const auto color = nandina::NanColor::from(nandina::NanHsl{0.0f, 1.0f, 0.5f, 1.0f});
    const auto rgb = color.to<nandina::NanRgb>();

    EXPECT_NEAR(rgb.red(), 255u, kChannelTolerance);
    EXPECT_NEAR(rgb.green(), 0u, kChannelTolerance);
    EXPECT_NEAR(rgb.blue(), 0u, kChannelTolerance);
    EXPECT_EQ(rgb.alpha(), static_cast<std::uint8_t>(255u));
}

TEST(NandinaColor, HsvConversionPreservesOpaqueGreen) {
    const auto color = nandina::NanColor::from(nandina::NanHsv{120.0f, 1.0f, 1.0f, 1.0f});
    const auto rgb = color.to<nandina::NanRgb>();

    EXPECT_NEAR(rgb.red(), 0u, kChannelTolerance);
    EXPECT_NEAR(rgb.green(), 255u, kChannelTolerance);
    EXPECT_NEAR(rgb.blue(), 0u, kChannelTolerance);
    EXPECT_EQ(rgb.alpha(), static_cast<std::uint8_t>(255u));
}

TEST(NandinaColor, RgbParsesHashTextWithoutAlpha) {
    const auto color = nandina::NanColor::from(nandina::NanRgb{"#A0B1C2"});
    const auto rgb = color.to<nandina::NanRgb>();

    EXPECT_EQ(rgb.red(), static_cast<std::uint8_t>(0xA0u));
    EXPECT_EQ(rgb.green(), static_cast<std::uint8_t>(0xB1u));
    EXPECT_EQ(rgb.blue(), static_cast<std::uint8_t>(0xC2u));
    EXPECT_EQ(rgb.alpha(), static_cast<std::uint8_t>(0xFFu));
}

TEST(NandinaColor, RgbParsesHashTextWithAlpha) {
    const auto color = nandina::NanColor::from(nandina::NanRgb{"#A0B1C2D3"});
    const auto rgb = color.to<nandina::NanRgb>();

    EXPECT_EQ(rgb.red(), static_cast<std::uint8_t>(0xA0u));
    EXPECT_EQ(rgb.green(), static_cast<std::uint8_t>(0xB1u));
    EXPECT_EQ(rgb.blue(), static_cast<std::uint8_t>(0xC2u));
    EXPECT_EQ(rgb.alpha(), static_cast<std::uint8_t>(0xD3u));
}

TEST(NandinaColor, HexParsesPackedValueWithoutAlpha) {
    const auto color = nandina::NanColor::from(nandina::NanHex{0xA0B1C2u});
    const auto rgb = color.to<nandina::NanRgb>();
    const auto hex = color.to<nandina::NanHex>();

    EXPECT_EQ(rgb.red(), static_cast<std::uint8_t>(0xA0u));
    EXPECT_EQ(rgb.green(), static_cast<std::uint8_t>(0xB1u));
    EXPECT_EQ(rgb.blue(), static_cast<std::uint8_t>(0xC2u));
    EXPECT_EQ(rgb.alpha(), static_cast<std::uint8_t>(0xFFu));
    EXPECT_FALSE(hex.has_alpha());
    EXPECT_EQ(hex.packed(), 0xA0B1C2u);
}

TEST(NandinaColor, HexParsesPackedValueWithAlpha) {
    const auto color = nandina::NanColor::from(nandina::NanHex{0xA0B1C2D3u});
    const auto rgb = color.to<nandina::NanRgb>();
    const auto hex = color.to<nandina::NanHex>();

    EXPECT_EQ(rgb.red(), static_cast<std::uint8_t>(0xA0u));
    EXPECT_EQ(rgb.green(), static_cast<std::uint8_t>(0xB1u));
    EXPECT_EQ(rgb.blue(), static_cast<std::uint8_t>(0xC2u));
    EXPECT_EQ(rgb.alpha(), static_cast<std::uint8_t>(0xD3u));
    EXPECT_TRUE(hex.has_alpha());
    EXPECT_EQ(hex.packed(), 0xA0B1C2D3u);
}

TEST(NandinaColor, RawOklabTracksAssignedValue) {
    auto color = nandina::NanColor{};
    color.assign(nandina::NanOklab{0.5f, 0.1f, -0.2f, 0.75f});

    const auto &oklab = color.raw_oklab();
    EXPECT_NEAR(oklab.lightness(), 0.5f, kFloatTolerance);
    EXPECT_NEAR(oklab.a_axis(), 0.1f, kFloatTolerance);
    EXPECT_NEAR(oklab.b_axis(), -0.2f, kFloatTolerance);
    EXPECT_NEAR(oklab.alpha(), 0.75f, kFloatTolerance);
}
