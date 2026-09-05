//
// render/texture_cache - window-scoped shared image texture residency.
//

#ifndef NANDINA_EXPERIMENT_RENDER_TEXTURE_CACHE_HPP
#define NANDINA_EXPERIMENT_RENDER_TEXTURE_CACHE_HPP

#include "image_decoder.hpp"
#include "render_device.hpp"

#include "../resource/resource.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <list>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::resource
{
    class ResourceManager;
}

namespace nandina::render
{
    class CachedTexture final {
    public:
        /// The render device must outlive every shared reference to this resource.
        CachedTexture(IRenderDevice& device, TextureHandle handle, NanSize size) noexcept;
        ~CachedTexture();

        CachedTexture(const CachedTexture&) = delete;
        auto operator=(const CachedTexture&) -> CachedTexture& = delete;

        [[nodiscard]] auto handle() const noexcept -> TextureHandle;
        [[nodiscard]] auto size() const noexcept -> NanSize;
        [[nodiscard]] auto estimated_bytes() const noexcept -> std::size_t;

    private:
        IRenderDevice* device_;
        TextureHandle handle_;
        NanSize size_;
        std::size_t estimated_bytes_ = 0;
    };

    struct TextureCacheLimits {
        std::size_t max_retained_entries = 64;
        std::size_t max_retained_bytes = 128U * 1024U * 1024U;
    };

    struct TextureCacheAsyncServices {
        std::shared_ptr<IImageDecoder> decoder;
        std::function<bool(std::move_only_function<void()>)> submit_background;
        std::function<bool(std::move_only_function<void()>)> post_ui;
        /// C5.5b 预算与重试：并发解码数上限、在途 encoded 字节预算、失败重试次数。
        std::size_t max_concurrent_decodes = 2;
        std::size_t max_inflight_encoded_bytes = 64U * 1024U * 1024U;
        std::size_t max_ready_upload_bytes = 64U * 1024U * 1024U;
        std::size_t max_load_attempts = 2;
        /// C5.6 每帧上传预算：一帧最多上传的 RGBA 纹理数（0 = 不限）。
        std::size_t max_uploads_per_frame = 0;

        [[nodiscard]] explicit operator bool() const noexcept {
            return decoder != nullptr && submit_background && post_ui;
        }
    };

    class TextureCache final {
    public:
        explicit TextureCache(
            IRenderDevice& device,
            TextureCacheLimits limits = {},
            TextureCacheAsyncServices async = {}
        );
        ~TextureCache();

        TextureCache(const TextureCache&) = delete;
        auto operator=(const TextureCache&) -> TextureCache& = delete;

        [[nodiscard]] auto load_file(std::string_view path, const ImageLoadOptions& options = {})
            -> std::shared_ptr<CachedTexture>;

        [[nodiscard]] auto load_memory(
            std::string_view cache_key,
            std::span<const std::uint8_t> bytes,
            std::string_view media_type,
            const ImageLoadOptions& options = {}
        ) -> std::shared_ptr<CachedTexture>;

        using AsyncCompletion = std::move_only_function<void(std::shared_ptr<CachedTexture>)>;

        /// Decode packaged bytes on a worker, then upload/cache on the UI thread.
        /// `bytes_owner` must own the storage referenced by `bytes` until completion.
        [[nodiscard]] auto load_memory_async(
            std::string_view cache_key,
            std::shared_ptr<const void> bytes_owner,
            std::span<const std::uint8_t> bytes,
            std::string_view media_type,
            const ImageLoadOptions& options,
            AsyncCompletion completion,
            int priority = 0
        ) -> bool;

        /// 后台完成 res:// 资源读取 + CPU 解码，UI 线程只上传纹理（C5.5）。缓存键由
        /// 资源 key 文本与预处理选项组成；失败时以空纹理完成回调。
        [[nodiscard]] auto load_resource_async(
            resource::ResourceManager& resources,
            resource::ResourceKey key,
            const ImageLoadOptions& options,
            AsyncCompletion completion,
            int priority = 0
        ) -> bool;

        [[nodiscard]] auto supports_async_loading() const noexcept -> bool;

        /// 帧起点：重置本帧上传计数并补做上一帧排队的上传（C5.6）。
        void begin_frame();

        void clear();
        [[nodiscard]] auto retained_entries() const noexcept -> std::size_t;
        [[nodiscard]] auto retained_bytes() const noexcept -> std::size_t;

    private:
        enum class SourceKind { file, memory };
        enum class PendingSubmitResult { submitted, blocked, rejected };

        struct Key {
            SourceKind kind = SourceKind::file;
            std::string source;
            std::string media_type;
            ImageLoadOptions options;
        };

        struct IndexedEntry {
            Key key;
            std::weak_ptr<CachedTexture> texture;
        };

        struct RetainedEntry {
            Key key;
            std::shared_ptr<CachedTexture> texture;
        };

        struct PendingEntry {
            Key key;
            std::vector<AsyncCompletion> completions;
            /// 后台读取 + 解码 + 重试，产出 RGBA（后台线程执行）。
            std::shared_ptr<std::move_only_function<DecodedImage()>> load;
            std::size_t encoded_bytes = 0;
            bool submitted = false;
            /// C5.6 加载优先级（低值先提交解码；0 = 可见）。
            int priority = 0;
        };

        struct ReadyUpload {
            Key key;
            DecodedImage decoded;
            std::size_t decoded_bytes = 0;
            std::uint64_t generation = 0;
            std::vector<AsyncCompletion> completions;
        };

        [[nodiscard]] auto find(const Key& key) -> std::shared_ptr<CachedTexture>;
        void index(Key key, const std::shared_ptr<CachedTexture>& texture);
        void retain(const Key& key, std::shared_ptr<CachedTexture> texture);
        void trim();
        void prune_expired();
        [[nodiscard]] auto find_pending(const Key& key) -> PendingEntry*;
        [[nodiscard]] auto submit_pending(PendingEntry& entry) -> PendingSubmitResult;
        void drain_pending();
        void drain_uploads();
        void finish_async(Key key, DecodedImage decoded, std::uint64_t generation);
        [[nodiscard]] static auto same_key(const Key& lhs, const Key& rhs) -> bool;

        IRenderDevice* device_;
        TextureCacheLimits limits_;
        TextureCacheAsyncServices async_;
        std::vector<IndexedEntry> index_;
        std::list<RetainedEntry> retained_;
        std::vector<PendingEntry> pending_;
        std::deque<ReadyUpload> ready_uploads_;
        std::size_t retained_bytes_ = 0;
        std::size_t inflight_decodes_ = 0;
        std::size_t inflight_encoded_bytes_ = 0;
        std::size_t ready_upload_bytes_ = 0;
        std::size_t uploads_this_frame_ = 0;
        std::uint64_t generation_ = 0;
        std::shared_ptr<std::atomic_bool> alive_ = std::make_shared<std::atomic_bool>(true);
    };
} // namespace nandina::render

#endif // NANDINA_EXPERIMENT_RENDER_TEXTURE_CACHE_HPP
