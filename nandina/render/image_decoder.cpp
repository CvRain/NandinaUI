#include "image_decoder.hpp"

#include "../foundation/nandina_color_space.hpp"

#include <raylib.h>

#include <cstring>
#include <limits>
#include <optional>
#include <string>

namespace nandina::render
{
    namespace
    {
        [[nodiscard]] auto image_extension(const std::string_view media_type)
            -> std::optional<std::string_view> {
            if (media_type == "image/png")
                return ".png";
            if (media_type == "image/jpeg")
                return ".jpg";
            if (media_type == "image/webp")
                return ".webp";
            if (media_type == "image/bmp")
                return ".bmp";
            if (media_type == "image/gif")
                return ".gif";
            if (media_type == "image/qoi")
                return ".qoi";
            if (media_type == "image/tga")
                return ".tga";
            return std::nullopt;
        }

        [[nodiscard]] auto to_raylib(const foundation::NanColor& color) -> ::Color {
            const auto hex = color.to<foundation::NanHexRgb>();
            return {hex.red, hex.green, hex.blue, hex.alpha};
        }

        [[nodiscard]] auto to_raylib(const foundation::NanRect& rect) -> ::Rectangle {
            return {rect.get_left(), rect.get_top(), rect.get_width(), rect.get_height()};
        }

        class RaylibImageDecoder final: public IImageDecoder {
        public:
            [[nodiscard]] auto decode_memory(
                const std::span<const std::uint8_t> bytes,
                const std::string_view media_type,
                const ImageLoadOptions& options
            ) -> DecodedImage override {
                const auto extension = image_extension(media_type);
                if (!extension || bytes.empty()
                    || bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
                {
                    return {};
                }
                const std::string extension_text(*extension);
                Image image = LoadImageFromMemory(
                    extension_text.c_str(),
                    bytes.data(),
                    static_cast<int>(bytes.size())
                );
                if (image.data == nullptr) {
                    return {};
                }
                if (options.crop)
                    ImageCrop(&image, to_raylib(*options.crop));
                if (options.resize) {
                    ImageResize(
                        &image,
                        static_cast<int>(options.resize->get_width()),
                        static_cast<int>(options.resize->get_height())
                    );
                }
                if (options.tint)
                    ImageColorTint(&image, to_raylib(*options.tint));
                ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                if (image.data == nullptr || image.width <= 0 || image.height <= 0) {
                    UnloadImage(image);
                    return {};
                }
                const auto pixel_count =
                    static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height);
                if (pixel_count > std::numeric_limits<std::size_t>::max() / 4U) {
                    UnloadImage(image);
                    return {};
                }
                DecodedImage decoded {
                    .width = image.width,
                    .height = image.height,
                    .rgba = std::vector<std::uint8_t>(pixel_count * 4U),
                };
                std::memcpy(decoded.rgba.data(), image.data, decoded.rgba.size());
                UnloadImage(image);
                return decoded;
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

    auto make_raylib_image_decoder() -> std::shared_ptr<IImageDecoder> {
        return std::make_shared<RaylibImageDecoder>();
    }
} // namespace nandina::render
