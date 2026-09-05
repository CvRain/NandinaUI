//
// text/glyph_atlas — CPU glyph packing and render-device texture bridge.
//

#ifndef NANDINA_EXPERIMENT_TEXT_GLYPH_ATLAS_HPP
#define NANDINA_EXPERIMENT_TEXT_GLYPH_ATLAS_HPP

#include "../foundation/geometry.hpp"
#include "../foundation/nandina_color.hpp"
#include "../render/render_device.hpp"
#include "font_face.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

namespace nandina::text
{

    struct GlyphAtlasEntry {
        char32_t codepoint = 0;
        float pixel_size = 0.0F;
        GlyphMetrics metrics;
        foundation::NanRect pixel_bounds;
    };

    class GlyphAtlas {
    public:
        explicit GlyphAtlas(
            std::shared_ptr<FreeTypeFontFace> face,
            int width = 512,
            int height = 512,
            int padding = 1
        );

        [[nodiscard]] auto cache(char32_t codepoint, float pixel_size) -> const GlyphAtlasEntry&;
        [[nodiscard]] auto cache_glyph(std::uint32_t glyph_index, float pixel_size)
            -> const GlyphAtlasEntry&;
        [[nodiscard]] auto find(char32_t codepoint, float pixel_size) const
            -> const GlyphAtlasEntry*;
        [[nodiscard]] auto find_glyph(std::uint32_t glyph_index, float pixel_size) const
            -> const GlyphAtlasEntry*;

        [[nodiscard]] auto width() const -> int;
        [[nodiscard]] auto height() const -> int;
        [[nodiscard]] auto revision() const -> std::uint64_t;
        [[nodiscard]] auto pixels() const -> std::span<const std::uint8_t>;
        [[nodiscard]] auto face() const -> const FreeTypeFontFace&;

    private:
        struct Key {
            std::uint32_t glyph_index = 0;
            std::uint32_t pixel_size = 0;

            auto operator==(const Key&) const -> bool = default;
        };

        struct KeyHash {
            [[nodiscard]] auto operator()(const Key& key) const noexcept -> std::size_t;
        };

        [[nodiscard]] static auto key_for(std::uint32_t glyph_index, float pixel_size) -> Key;
        [[nodiscard]] auto allocate(int width, int height) -> foundation::NanRect;

        std::shared_ptr<FreeTypeFontFace> face_;
        int width_ = 0;
        int height_ = 0;
        int padding_ = 0;
        int cursor_x_ = 0;
        int cursor_y_ = 0;
        int row_height_ = 0;
        std::uint64_t revision_ = 0;
        std::vector<std::uint8_t> pixels_;
        std::unordered_map<Key, GlyphAtlasEntry, KeyHash> entries_;
    };

    class GlyphAtlasTexture {
    public:
        /// The device and atlas must outlive this texture bridge.
        GlyphAtlasTexture(render::IRenderDevice& device, GlyphAtlas& atlas);
        ~GlyphAtlasTexture();

        GlyphAtlasTexture(const GlyphAtlasTexture&) = delete;
        auto operator=(const GlyphAtlasTexture&) -> GlyphAtlasTexture& = delete;

        void sync();
        void draw(
            const GlyphAtlasEntry& glyph,
            foundation::NanPoint baseline_origin,
            foundation::NanColor color,
            float screen_to_physical = 1.0F
        );

        [[nodiscard]] auto handle() const -> render::TextureHandle;
        [[nodiscard]] auto uploaded_revision() const -> std::uint64_t;
        [[nodiscard]] auto atlas() const -> const GlyphAtlas&;

    private:
        render::IRenderDevice& device_;
        GlyphAtlas& atlas_;
        render::TextureHandle texture_;
        std::uint64_t uploaded_revision_ = 0;
    };

} // namespace nandina::text

#endif // NANDINA_EXPERIMENT_TEXT_GLYPH_ATLAS_HPP
