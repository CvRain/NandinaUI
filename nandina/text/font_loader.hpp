#ifndef NANDINA_EXPERIMENT_TEXT_FONT_LOADER_HPP
#define NANDINA_EXPERIMENT_TEXT_FONT_LOADER_HPP

#include "../resource/resource_manager.hpp"
#include "font_face.hpp"

#include <expected>
#include <filesystem>
#include <map>
#include <mutex>

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

    class FontLoader {
    public:
        explicit FontLoader(const resource::ResourceManager& resources): resources_(&resources) {}

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

    private:
        [[nodiscard]] auto
        load_resolved(const resource::ResolvedResource& resolved, std::uint32_t face_index)
            -> FontResult<std::shared_ptr<FreeTypeFontFace>>;
        [[nodiscard]] static auto
        create_face(resource::ResourceHandle resource, std::uint32_t face_index)
            -> FontResult<std::shared_ptr<FreeTypeFontFace>>;

        const resource::ResourceManager* resources_;
        std::mutex mutex_;
        std::map<FontFaceKey, std::weak_ptr<FreeTypeFontFace>> cache_;
    };
} // namespace nandina::text
#endif
