//
// 默认 CPU 图片解码器：格式、预处理、错误输入与线程安全。
//

#include <nandina/render/image_decoder.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

namespace
{
    using nandina::foundation::NanColor;
    using nandina::foundation::NanRect;
    using nandina::render::DecodedImage;
    using nandina::render::ImageLoadOptions;
    using nandina::render::make_cpu_image_decoder;

    [[nodiscard]] auto fixture(const std::string& name) -> std::vector<std::uint8_t> {
        std::ifstream input(std::string(NANDINA_TEST_IMAGE_DIR) + "/" + name, std::ios::binary);
        REQUIRE(input.good());
        return std::vector<std::uint8_t>(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()
        );
    }

    [[nodiscard]] auto rgba_at(const DecodedImage& image, const int x, const int y)
        -> std::array<int, 4> {
        const auto offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width)
                             + static_cast<std::size_t>(x))
            * 4U;
        return {
            image.rgba[offset + 0],
            image.rgba[offset + 1],
            image.rgba[offset + 2],
            image.rgba[offset + 3],
        };
    }

    [[nodiscard]] auto color(const std::uint32_t hex) -> NanColor {
        return NanColor::from_hex(hex);
    }
} // namespace

TEST_CASE("cpu decoder decodes PNG pixels", "[image][decoder]") {
    auto decoder = make_cpu_image_decoder();
    const auto png = fixture("rgba-4x4.png");
    const auto decoded = decoder->decode_memory(png, "image/png");
    REQUIRE(decoded.valid());
    REQUIRE(decoded.width == 4);
    REQUIRE(decoded.height == 4);
    REQUIRE(rgba_at(decoded, 0, 0) == std::array {255, 0, 0, 255});
    REQUIRE(rgba_at(decoded, 3, 1) == std::array {0, 255, 0, 255});
    REQUIRE(rgba_at(decoded, 0, 2) == std::array {0, 0, 255, 255});
    REQUIRE(rgba_at(decoded, 3, 3) == std::array {255, 255, 255, 255});
}

TEST_CASE("cpu decoder preserves alpha", "[image][decoder]") {
    auto decoder = make_cpu_image_decoder();
    const auto png = fixture("transparent-2x2.png");
    const auto decoded = decoder->decode_memory(png, "image/png");
    REQUIRE(decoded.valid());
    REQUIRE(rgba_at(decoded, 0, 0) == std::array {10, 20, 30, 0});
    REQUIRE(rgba_at(decoded, 1, 0) == std::array {40, 50, 60, 128});
    REQUIRE(rgba_at(decoded, 0, 1) == std::array {70, 80, 90, 255});
    REQUIRE(rgba_at(decoded, 1, 1) == std::array {100, 110, 120, 64});
}

TEST_CASE("cpu decoder crops", "[image][decoder]") {
    auto decoder = make_cpu_image_decoder();
    const auto png = fixture("rgba-4x4.png");
    ImageLoadOptions options;
    options.crop = NanRect::from_ltrb(1.0F, 1.0F, 3.0F, 3.0F);
    const auto decoded = decoder->decode_memory(png, "image/png", options);
    REQUIRE(decoded.valid());
    REQUIRE(decoded.width == 2);
    REQUIRE(decoded.height == 2);
    REQUIRE(rgba_at(decoded, 0, 0) == std::array {0, 255, 0, 255});
    REQUIRE(rgba_at(decoded, 1, 1) == std::array {0, 0, 255, 255});
}

TEST_CASE("cpu decoder resizes", "[image][decoder]") {
    auto decoder = make_cpu_image_decoder();
    const auto png = fixture("rgba-4x4.png");
    ImageLoadOptions options;
    options.resize = nandina::foundation::NanSize(2.0F, 2.0F);
    const auto decoded = decoder->decode_memory(png, "image/png", options);
    REQUIRE(decoded.valid());
    REQUIRE(decoded.width == 2);
    REQUIRE(decoded.height == 2);
}

TEST_CASE("cpu decoder tints", "[image][decoder]") {
    auto decoder = make_cpu_image_decoder();
    const auto png = fixture("rgba-4x4.png");
    ImageLoadOptions options;
    options.tint = color(0x808080).with_alpha(128.0F / 255.0F);
    const auto decoded = decoder->decode_memory(png, "image/png", options);
    REQUIRE(decoded.valid());
    // 白色 * 0x80/0xFF = 128（整数乘除，含 alpha）。
    REQUIRE(rgba_at(decoded, 0, 3) == std::array {128, 128, 128, 128});
}

TEST_CASE("cpu decoder handles the other formats", "[image][decoder]") {
    auto decoder = make_cpu_image_decoder();
    for (const auto& [name, media_type, width, height]:
         std::array {
             std::tuple {"rgb-8x8.jpg", "image/jpeg", 8, 8},
             std::tuple {"sample-6x6.bmp", "image/bmp", 6, 6},
             std::tuple {"sample-5x5.gif", "image/gif", 5, 5},
             std::tuple {"sample-7x7.tga", "image/tga", 7, 7},
         })
    {
        const auto bytes = fixture(name);
        const auto decoded = decoder->decode_memory(bytes, media_type);
        INFO(name);
        REQUIRE(decoded.valid());
        REQUIRE(decoded.width == width);
        REQUIRE(decoded.height == height);
    }
}

TEST_CASE("cpu decoder rejects invalid input", "[image][decoder]") {
    auto decoder = make_cpu_image_decoder();
    // 损坏的 PNG 头 + 垃圾数据。
    REQUIRE_FALSE(decoder->decode_memory(fixture("corrupt.png"), "image/png").valid());
    // 空输入。
    REQUIRE_FALSE(decoder->decode_memory(std::span<const std::uint8_t> {}, "image/png").valid());
    // 不支持的媒体类型（stb 不提供 WebP）。
    REQUIRE_FALSE(decoder->decode_memory(fixture("rgba-4x4.png"), "image/webp").valid());
    // 越界的 crop 窗口。
    ImageLoadOptions options;
    options.crop = NanRect::from_ltrb(10.0F, 10.0F, 20.0F, 20.0F);
    REQUIRE_FALSE(decoder->decode_memory(fixture("rgba-4x4.png"), "image/png", options).valid());
}

TEST_CASE("cpu decoder is safe on a worker thread", "[image][decoder]") {
    auto decoder = make_cpu_image_decoder();
    const auto png = fixture("rgba-4x4.png");
    DecodedImage decoded;
    std::thread worker([&] { decoded = decoder->decode_memory(png, "image/png"); });
    worker.join();
    REQUIRE(decoded.valid());
    REQUIRE(decoded.width == 4);
}
