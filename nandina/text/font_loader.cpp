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
        return load_resolved(*found, face_index);
    }

    auto FontLoader::load(const resource::ResourceId id, const std::uint32_t face_index)
        -> FontResult<std::shared_ptr<FreeTypeFontFace>> {
        auto found = resources_->require(id);
        if (!found) {
            return std::unexpected(resource_failure(found.error()));
        }
        return load_resolved(*found, face_index);
    }

    auto FontLoader::load(resource::ResourceHandle resource, const std::uint32_t face_index)
        -> FontResult<std::shared_ptr<FreeTypeFontFace>> {
        // 直接句柄没有挂载/代次身份，不能伪造作用域进入共享缓存。
        return create_face(std::move(resource), face_index);
    }

    auto FontLoader::load_resolved(
        const resource::ResolvedResource& resolved,
        const std::uint32_t face_index
    ) -> FontResult<std::shared_ptr<FreeTypeFontFace>> {
        // 非快照来源无法证明同代次内容稳定，只能返回本次创建的 face，不进入共享缓存。
        if (!resource::is_snapshot(resolved.consistency)) {
            return create_face(resolved.resource, face_index);
        }
        const FontFaceKey cache_key {
            .identity = resource::scope_identity(resolved),
            .face_index = face_index,
        };
        std::lock_guard lock(mutex_);
        if (const auto found = cache_.find(cache_key); found != cache_.end()) {
            if (auto cached = found->second.lock()) {
                return cached;
            }
        }
        auto created = create_face(resolved.resource, face_index);
        if (!created) {
            return std::unexpected(created.error());
        }
        cache_[cache_key] = *created;
        return *created;
    }

    auto FontLoader::create_face(resource::ResourceHandle resource, const std::uint32_t face_index)
        -> FontResult<std::shared_ptr<FreeTypeFontFace>> {
        if (!resource) {
            return std::unexpected(
                FontError {FontErrorCode::resource_failure, "font.load", "font resource is null"}
            );
        }
        try {
            return std::make_shared<FreeTypeFontFace>(
                std::move(resource),
                static_cast<long>(face_index)
            );
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

    void FontLoader::invalidate(const resource::ResolvedResource& reference) {
        const auto identity = resource::scope_identity(reference);
        std::lock_guard lock(mutex_);
        std::erase_if(cache_, [&identity](const auto& entry) {
            return entry.first.identity == identity;
        });
    }
    void FontLoader::invalidate_wide(const resource::ResourceId resource_id) {
        std::lock_guard lock(mutex_);
        std::erase_if(cache_, [resource_id](const auto& entry) {
            return entry.first.identity.id == resource_id;
        });
    }
    void FontLoader::clear() {
        std::lock_guard lock(mutex_);
        cache_.clear();
    }
} // namespace nandina::text
