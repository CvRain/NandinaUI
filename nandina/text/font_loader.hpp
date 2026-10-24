#ifndef NANDINA_EXPERIMENT_TEXT_FONT_LOADER_HPP
#define NANDINA_EXPERIMENT_TEXT_FONT_LOADER_HPP

#include "../resource/resource_manager.hpp"
#include "font_face.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <map>
#include <mutex>
#include <optional>

namespace nandina::text
{
    enum class FontErrorCode {
        resource_failure,
        invalid_face_index,
        invalid_font,
        duplicate_family,
        duplicate_alias,
        unknown_family,
        invalid_fallback,
        no_matching_face,
        pipeline_failure,
    };

    struct FontError {
        FontErrorCode code;
        std::string operation;
        std::string message;
        std::optional<resource::ResourceError> cause;
    };
    template<typename T>
    using FontResult = std::expected<T, FontError>;

    struct FontFaceKey {
        resource::ResourceScopeIdentity identity;
        std::uint32_t face_index = 0;
        auto operator<=>(const FontFaceKey&) const = default;
    };

    struct FontLoaderLimits {
        /// 最多保留的 face 关联数；0 表示不缓存。淘汰只删除关联，不销毁使用者持有的 face。
        std::size_t max_entries = 64;
    };

    class FontLoader {
    public:
        explicit FontLoader(
            const resource::ResourceManager& resources,
            FontLoaderLimits limits = {}
        );

        [[nodiscard]] auto load(const resource::ResourceKey& key, std::uint32_t face_index = 0)
            -> FontResult<std::shared_ptr<FreeTypeFontFace>>;
        [[nodiscard]] auto load(resource::ResourceId id, std::uint32_t face_index = 0)
            -> FontResult<std::shared_ptr<FreeTypeFontFace>>;
        /// 直接句柄没有挂载身份：绕过共享缓存，每次创建独立 face。
        [[nodiscard]] auto load(resource::ResourceHandle resource, std::uint32_t face_index = 0)
            -> FontResult<std::shared_ptr<FreeTypeFontFace>>;

        /// 从文件路径直接加载字体（不经过资源系统；nanres 集成延后）。
        [[nodiscard]] auto
        load_file(const std::filesystem::path& path, std::uint32_t face_index = 0)
            -> FontResult<std::shared_ptr<FreeTypeFontFace>>;

        /// 使用与 lookup 同源的作用域失效该资源在本实例中的全部 face。
        void invalidate(const resource::ResolvedResource& reference);
        /// 广域操作：按裸记录 ID 失效本实例中的所有作用域，不跨缓存实例广播。
        void invalidate_wide(resource::ResourceId resource_id);
        void clear();
        /// 当前保留的 face 关联数（包含尚未清扫的过期项）。
        [[nodiscard]] auto cached_entries() const -> std::size_t;

    private:
        struct CachedFace {
            std::weak_ptr<FreeTypeFontFace> face;
            std::uint64_t last_used = 0;
        };

        [[nodiscard]] auto
        load_resolved(const resource::ResolvedResource& resolved, std::uint32_t face_index)
            -> FontResult<std::shared_ptr<FreeTypeFontFace>>;
        [[nodiscard]] static auto
        create_face(resource::ResourceHandle resource, std::uint32_t face_index)
            -> FontResult<std::shared_ptr<FreeTypeFontFace>>;
        void trim_locked();

        const resource::ResourceManager* resources_;
        FontLoaderLimits limits_;
        mutable std::mutex mutex_;
        std::uint64_t use_clock_ = 0;
        std::map<FontFaceKey, CachedFace> cache_;
    };
} // namespace nandina::text
#endif
