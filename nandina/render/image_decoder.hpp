//
// render/image_decoder - backend-neutral CPU image decoding.
//

#ifndef NANDINA_EXPERIMENT_RENDER_IMAGE_DECODER_HPP
#define NANDINA_EXPERIMENT_RENDER_IMAGE_DECODER_HPP

#include "render_device.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace nandina::render
{
    struct DecodedImage {
        int width = 0;
        int height = 0;
        std::vector<std::uint8_t> rgba;

        [[nodiscard]] auto valid() const noexcept -> bool;
    };

    class IImageDecoder {
    public:
        virtual ~IImageDecoder() = default;

        /// CPU-only and safe to call from a background worker. The returned pixels
        /// are tightly packed RGBA8 and contain all requested preprocessing.
        [[nodiscard]] virtual auto decode_memory(
            std::span<const std::uint8_t> bytes,
            std::string_view media_type,
            const ImageLoadOptions& options = {}
        ) -> DecodedImage = 0;
    };

    [[nodiscard]] auto make_raylib_image_decoder() -> std::shared_ptr<IImageDecoder>;
} // namespace nandina::render

#endif // NANDINA_EXPERIMENT_RENDER_IMAGE_DECODER_HPP
