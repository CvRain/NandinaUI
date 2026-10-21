#include "image_decoder.hpp"

#include "../foundation/nandina_color_space.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <vector>

// 只启用需要的格式并禁用 stdio：解码只走内存，避免额外依赖。
#define STB_IMAGE_STATIC
#define STB_IMAGE_RESIZE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#define STBI_ONLY_TGA
#define STBI_NO_STDIO
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

namespace nandina::render
{
    namespace
    {
        constexpr int kChannels = 4;

        [[nodiscard]] auto media_type_supported(const std::string_view media_type) -> bool {
            return media_type == "image/png" || media_type == "image/jpeg"
                || media_type == "image/bmp" || media_type == "image/gif"
                || media_type == "image/tga";
        }

        struct RgbaImage {
            int width = 0;
            int height = 0;
            std::vector<std::uint8_t> pixels;

            [[nodiscard]] auto valid() const -> bool {
                return width > 0 && height > 0
                    && pixels.size()
                    == static_cast<std::size_t>(width) * static_cast<std::size_t>(height)
                        * kChannels;
            }
        };

        [[nodiscard]] auto crop(RgbaImage& image, const foundation::NanRect& rect) -> bool {
            // 与 raylib ImageCrop 一致：浮点矩形截断成整数像素。
            const auto left = static_cast<int>(std::trunc(rect.get_left()));
            const auto top = static_cast<int>(std::trunc(rect.get_top()));
            const auto right = static_cast<int>(std::trunc(rect.get_right()));
            const auto bottom = static_cast<int>(std::trunc(rect.get_bottom()));
            const auto x0 = std::clamp(left, 0, image.width);
            const auto y0 = std::clamp(top, 0, image.height);
            const auto x1 = std::clamp(right, x0, image.width);
            const auto y1 = std::clamp(bottom, y0, image.height);
            const auto width = x1 - x0;
            const auto height = y1 - y0;
            if (width <= 0 || height <= 0) {
                return false;
            }
            if (x0 == 0 && y0 == 0 && width == image.width && height == image.height) {
                return true;
            }
            std::vector<std::uint8_t> cropped(
                static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * kChannels
            );
            const auto row_bytes = static_cast<std::size_t>(width) * kChannels;
            for (int row = 0; row < height; ++row) {
                const auto source =
                    (static_cast<std::size_t>(y0 + row) * image.width + x0) * kChannels;
                std::memcpy(
                    cropped.data() + static_cast<std::size_t>(row) * row_bytes,
                    image.pixels.data() + source,
                    row_bytes
                );
            }
            image.width = width;
            image.height = height;
            image.pixels = std::move(cropped);
            return true;
        }

        [[nodiscard]] auto resize(RgbaImage& image, const int width, const int height) -> bool {
            if (width == image.width && height == image.height) {
                return true;
            }
            if (width <= 0 || height <= 0) {
                return false;
            }
            const auto pixel_count =
                static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
            if (pixel_count > std::numeric_limits<std::size_t>::max() / kChannels) {
                return false;
            }
            std::vector<std::uint8_t> resized(pixel_count * kChannels);
            const auto* result = stbir_resize_uint8_linear(
                image.pixels.data(),
                image.width,
                image.height,
                0,
                resized.data(),
                width,
                height,
                0,
                STBIR_RGBA
            );
            if (result == nullptr) {
                return false;
            }
            image.width = width;
            image.height = height;
            image.pixels = std::move(resized);
            return true;
        }

        void tint(RgbaImage& image, const foundation::NanColor& color) {
            const auto hex = color.to<foundation::NanHexRgb>();
            const std::array<int, 4> factors {hex.red, hex.green, hex.blue, hex.alpha};
            for (std::size_t index = 0; index < image.pixels.size(); index += kChannels) {
                for (int channel = 0; channel < kChannels; ++channel) {
                    // 与 raylib ColorTint 一致：整数乘除以 255（含 alpha）。
                    image.pixels[index + static_cast<std::size_t>(channel)] =
                        static_cast<std::uint8_t>(
                            (static_cast<int>(
                                 image.pixels[index + static_cast<std::size_t>(channel)]
                             )
                             * factors[static_cast<std::size_t>(channel)])
                            / 255
                        );
                }
            }
        }

        class CpuImageDecoder final: public IImageDecoder {
        public:
            [[nodiscard]] auto decode_memory(
                const std::span<const std::uint8_t> bytes,
                const std::string_view media_type,
                const ImageLoadOptions& options
            ) -> DecodedImage override {
                if (!media_type_supported(media_type) || bytes.empty()
                    || bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
                {
                    return {};
                }
                int width = 0;
                int height = 0;
                int channels = 0;
                auto* pixels = stbi_load_from_memory(
                    bytes.data(),
                    static_cast<int>(bytes.size()),
                    &width,
                    &height,
                    &channels,
                    kChannels
                );
                if (pixels == nullptr) {
                    return {};
                }
                RgbaImage image {
                    .width = width,
                    .height = height,
                    .pixels = std::vector<std::uint8_t>(
                        pixels,
                        pixels
                            + static_cast<std::size_t>(width) * static_cast<std::size_t>(height)
                                * kChannels
                    ),
                };
                stbi_image_free(pixels);
                if (!image.valid()) {
                    return {};
                }
                if (options.crop && !crop(image, *options.crop)) {
                    return {};
                }
                if (options.resize
                    && !resize(
                        image,
                        static_cast<int>(std::trunc(options.resize->get_width())),
                        static_cast<int>(std::trunc(options.resize->get_height()))
                    ))
                {
                    return {};
                }
                if (options.tint) {
                    tint(image, *options.tint);
                }
                if (!image.valid()) {
                    return {};
                }
                return DecodedImage {
                    .width = image.width,
                    .height = image.height,
                    .rgba = std::move(image.pixels),
                };
            }
        };
    } // namespace

    auto DecodedImage::valid() const noexcept -> bool {
        if (width <= 0 || height <= 0)
            return false;
        const auto pixel_count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        return pixel_count <= std::numeric_limits<std::size_t>::max() / 4U
            && rgba.size() == pixel_count * 4U;
    }

    auto make_cpu_image_decoder() -> std::shared_ptr<IImageDecoder> {
        return std::make_shared<CpuImageDecoder>();
    }
} // namespace nandina::render
