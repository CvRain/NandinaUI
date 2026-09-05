#include "font_loader.hpp"

namespace nandina::text
{
    namespace
    {
        [[nodiscard]] auto resource_failure(resource::ResourceError error) -> FontError {
            return {
                .code = FontErrorCode::resource_failure,
                .operation = "font.load",
                .message = "font resource lookup failed",
                .cause = std::move(error),
            };
        }
    } // namespace

    auto FontLoader::load(const resource::ResourceKey& key, const std::uint32_t face_index)
        -> FontResult<std::shared_ptr<FreeTypeFontFace>> {
        auto found = resources_->require(key);
        if (!found) {
            return std::unexpected(resource_failure(found.error()));
        }
        return load(*found, face_index);
    }

    auto FontLoader::load(const resource::ResourceId id, const std::uint32_t face_index)
        -> FontResult<std::shared_ptr<FreeTypeFontFace>> {
        auto found = resources_->require(id);
        if (!found) {
            return std::unexpected(resource_failure(found.error()));
        }
        return load(*found, face_index);
    }

    auto FontLoader::load(resource::ResourceHandle resource, const std::uint32_t face_index)
        -> FontResult<std::shared_ptr<FreeTypeFontFace>> {
        if (!resource) {
            return std::unexpected(
                FontError {FontErrorCode::resource_failure, "font.load", "font resource is null"}
            );
        }
        std::lock_guard lock(mutex_);
        const FontFaceKey key {.resource_id = resource->id(), .face_index = face_index};
        if (const auto found = cache_.find(key); found != cache_.end()) {
            if (auto cached = found->second.lock()) {
                return cached;
            }
        }
        try {
            auto face = std::make_shared<FreeTypeFontFace>(resource, static_cast<long>(face_index));
            cache_[key] = face;
            return face;
        }
        catch (const std::exception& exception) {
            return std::unexpected(
                FontError {
                    .code = FontErrorCode::invalid_font,
                    .operation = "font.load",
                    .message = exception.what(),
                }
            );
        }
    }

    auto FontLoader::load_file(const std::filesystem::path& path, const std::uint32_t face_index)
        -> FontResult<std::shared_ptr<FreeTypeFontFace>> {
        try {
            auto face = std::make_shared<FreeTypeFontFace>(path, static_cast<long>(face_index));
            return face;
        }
        catch (const std::exception& exception) {
            return std::unexpected(
                FontError {
                    .code = FontErrorCode::invalid_font,
                    .operation = "font.load_file",
                    .message = exception.what(),
                }
            );
        }
    }

    void FontLoader::invalidate(const resource::ResourceId id) {
        std::lock_guard lock(mutex_);
        std::erase_if(cache_, [id](const auto& entry) { return entry.first.resource_id == id; });
    }
    void FontLoader::clear() {
        std::lock_guard lock(mutex_);
        cache_.clear();
    }
} // namespace nandina::text
