#include "texture_cache.hpp"

#include "../resource/resource_manager.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace nandina::render
{
    namespace
    {
        [[nodiscard]] auto estimate_rgba_bytes(const NanSize size) noexcept -> std::size_t {
            const float width = size.get_width();
            const float height = size.get_height();
            if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0F || height <= 0.0F)
            {
                return 0;
            }
            const long double bytes =
                static_cast<long double>(width) * static_cast<long double>(height) * 4.0L;
            if (bytes >= static_cast<long double>(std::numeric_limits<std::size_t>::max())) {
                return std::numeric_limits<std::size_t>::max();
            }
            return static_cast<std::size_t>(bytes);
        }

        [[nodiscard]] auto same_options(const ImageLoadOptions& lhs, const ImageLoadOptions& rhs)
            -> bool {
            return lhs.crop == rhs.crop && lhs.resize == rhs.resize && lhs.tint == rhs.tint;
        }

        [[nodiscard]] auto decoded_byte_size(const DecodedImage& decoded) noexcept -> std::size_t {
            return decoded.rgba.size();
        }
    } // namespace

    CachedTexture::CachedTexture(
        IRenderDevice& device,
        const TextureHandle handle,
        const NanSize size
    ) noexcept:
        device_(&device),
        handle_(handle),
        size_(size),
        estimated_bytes_(estimate_rgba_bytes(size)) {}

    CachedTexture::~CachedTexture() {
        if (device_ != nullptr && handle_) {
            device_->destroy_texture(handle_);
        }
    }

    auto CachedTexture::handle() const noexcept -> TextureHandle {
        return handle_;
    }

    auto CachedTexture::size() const noexcept -> NanSize {
        return size_;
    }

    auto CachedTexture::estimated_bytes() const noexcept -> std::size_t {
        return estimated_bytes_;
    }

    TextureCache::TextureCache(
        IRenderDevice& device,
        const TextureCacheLimits limits,
        TextureCacheAsyncServices async
    ):
        device_(&device),
        limits_(limits),
        async_(std::move(async)) {}

    TextureCache::~TextureCache() {
        alive_->store(false, std::memory_order_release);
        pending_.clear();
        clear();
    }

    auto TextureCache::load_file(const std::string_view path, const ImageLoadOptions& options)
        -> std::shared_ptr<CachedTexture> {
        if (path.empty()) {
            return {};
        }
        const Key key {
            .kind = SourceKind::file,
            .source = std::string(path),
            .media_type = {},
            .options = options,
        };
        if (auto cached = find(key)) {
            retain(key, cached);
            return cached;
        }
        const auto handle = device_->load_texture_from_file(path, options);
        if (!handle) {
            return {};
        }
        auto texture =
            std::make_shared<CachedTexture>(*device_, handle, device_->texture_size(handle));
        index(key, texture);
        retain(key, texture);
        return texture;
    }

    auto TextureCache::load_memory(
        const std::string_view cache_key,
        const std::span<const std::uint8_t> bytes,
        const std::string_view media_type,
        const ImageLoadOptions& options
    ) -> std::shared_ptr<CachedTexture> {
        if (cache_key.empty() || bytes.empty() || media_type.empty()) {
            return {};
        }
        const Key key {
            .kind = SourceKind::memory,
            .source = std::string(cache_key),
            .media_type = std::string(media_type),
            .options = options,
        };
        if (auto cached = find(key)) {
            retain(key, cached);
            return cached;
        }
        const auto handle = device_->load_texture_from_memory(bytes, media_type, options);
        if (!handle) {
            return {};
        }
        auto texture =
            std::make_shared<CachedTexture>(*device_, handle, device_->texture_size(handle));
        index(key, texture);
        retain(key, texture);
        return texture;
    }

    auto TextureCache::load_memory_async(
        const std::string_view cache_key,
        std::shared_ptr<const void> bytes_owner,
        const std::span<const std::uint8_t> bytes,
        const std::string_view media_type,
        const ImageLoadOptions& options,
        AsyncCompletion completion,
        const int priority
    ) -> bool {
        if (!supports_async_loading() || cache_key.empty() || !bytes_owner || bytes.empty()
            || media_type.empty() || !completion)
        {
            return false;
        }
        Key key {
            .kind = SourceKind::memory,
            .source = std::string(cache_key),
            .media_type = std::string(media_type),
            .options = options,
        };
        if (auto cached = find(key)) {
            retain(key, cached);
            completion(std::move(cached));
            return true;
        }
        if (auto* pending = find_pending(key)) {
            pending->priority = std::min(pending->priority, priority);
            pending->completions.push_back(std::move(completion));
            return true;
        }
        auto decoder = async_.decoder;
        const auto attempts = async_.max_load_attempts;
        PendingEntry entry {
            .key = key,
            .completions = {},
            .load = std::make_shared<std::move_only_function<DecodedImage()>>(
                [decoder = std::move(decoder),
                 bytes_owner = std::move(bytes_owner),
                 bytes,
                 media_type = std::string(media_type),
                 options,
                 attempts]() mutable -> DecodedImage {
                    (void)bytes_owner;
                    for (std::size_t attempt = 0; attempt < attempts; ++attempt) {
                        auto decoded = decoder->decode_memory(bytes, media_type, options);
                        if (decoded.valid()) {
                            return decoded;
                        }
                    }
                    return DecodedImage {};
                }
            ),
            .encoded_bytes = bytes.size(),
            .submitted = false,
            .priority = priority,
        };
        entry.completions.push_back(std::move(completion));
        pending_.push_back(std::move(entry));
        drain_pending();
        return true;
    }

    auto TextureCache::supports_async_loading() const noexcept -> bool {
        return static_cast<bool>(async_) && async_.max_concurrent_decodes > 0
            && async_.max_load_attempts > 0;
    }

    auto TextureCache::load_resource_async(
        resource::ResourceManager& resources,
        resource::ResourceKey key,
        const ImageLoadOptions& options,
        AsyncCompletion completion,
        const int priority
    ) -> bool {
        if (!supports_async_loading() || !completion) {
            return false;
        }
        const Key cache_key {
            .kind = SourceKind::memory,
            .source = std::string(key.value()),
            .media_type = {},
            .options = options,
        };
        if (auto cached = find(cache_key)) {
            retain(cache_key, cached);
            completion(std::move(cached));
            return true;
        }
        if (auto* pending = find_pending(cache_key)) {
            pending->priority = std::min(pending->priority, priority);
            pending->completions.push_back(std::move(completion));
            return true;
        }
        auto decoder = async_.decoder;
        const auto attempts = async_.max_load_attempts;
        PendingEntry entry {
            .key = cache_key,
            .completions = {},
            .load = std::make_shared<std::move_only_function<DecodedImage()>>(
                [decoder = std::move(decoder), &resources, key, options, attempts]() mutable
                    -> DecodedImage {
                    for (std::size_t attempt = 0; attempt < attempts; ++attempt) {
                        auto loaded = resources.require(key);
                        if (loaded && *loaded && (*loaded)->media_type().starts_with("image/")) {
                            auto decoded = decoder->decode_memory(
                                (*loaded)->bytes(),
                                (*loaded)->media_type(),
                                options
                            );
                            if (decoded.valid()) {
                                return decoded;
                            }
                        }
                    }
                    return DecodedImage {};
                }
            ),
            .encoded_bytes = 0,
            .submitted = false,
            .priority = priority,
        };
        entry.completions.push_back(std::move(completion));
        pending_.push_back(std::move(entry));
        drain_pending();
        return true;
    }

    auto TextureCache::find_pending(const Key& key) -> PendingEntry* {
        const auto found = std::ranges::find_if(pending_, [&key](const PendingEntry& entry) {
            return same_key(entry.key, key);
        });
        return found != pending_.end() ? &*found : nullptr;
    }

    auto TextureCache::submit_pending(PendingEntry& entry) -> PendingSubmitResult {
        if (entry.submitted) {
            return PendingSubmitResult::submitted;
        }
        if (inflight_decodes_ >= async_.max_concurrent_decodes) {
            return PendingSubmitResult::blocked;
        }
        if (async_.max_inflight_encoded_bytes != 0 && inflight_encoded_bytes_ != 0
            && entry.encoded_bytes > async_.max_inflight_encoded_bytes
                    - std::min(inflight_encoded_bytes_, async_.max_inflight_encoded_bytes))
        {
            return PendingSubmitResult::blocked;
        }
        if (async_.max_ready_upload_bytes != 0
            && ready_upload_bytes_ >= async_.max_ready_upload_bytes)
        {
            return PendingSubmitResult::blocked;
        }
        entry.submitted = true;
        ++inflight_decodes_;
        inflight_encoded_bytes_ += entry.encoded_bytes;

        auto load = entry.load;
        auto post_ui = async_.post_ui;
        const auto alive = std::weak_ptr<std::atomic_bool>(alive_);
        const auto generation = generation_;
        auto* cache = this;
        auto key = entry.key;
        const bool submitted = async_.submit_background([load = std::move(load),
                                                         post_ui = std::move(post_ui),
                                                         alive,
                                                         cache,
                                                         generation,
                                                         key = std::move(key)]() mutable {
            const auto guard = alive.lock();
            if (!guard || !guard->load(std::memory_order_acquire)) {
                return;
            }
            auto decoded = (*load)();
            if (!guard->load(std::memory_order_acquire)) {
                return;
            }
            (void)post_ui([alive,
                           cache,
                           key = std::move(key),
                           decoded = std::move(decoded),
                           generation]() mutable {
                const auto current = alive.lock();
                if (!current || !current->load(std::memory_order_acquire)) {
                    return;
                }
                cache->finish_async(std::move(key), std::move(decoded), generation);
            });
        });
        if (!submitted) {
            entry.submitted = false;
            --inflight_decodes_;
            inflight_encoded_bytes_ -= entry.encoded_bytes;
            return PendingSubmitResult::rejected;
        }
        return PendingSubmitResult::submitted;
    }

    void TextureCache::drain_pending() {
        // C5.6：按优先级（低值 = 更可见/更近）排序，保证可见图片先提交解码。
        std::ranges::stable_sort(pending_, std::less<> {}, &PendingEntry::priority);
        std::vector<std::vector<AsyncCompletion>> rejected;
        for (auto entry = pending_.begin(); entry != pending_.end();) {
            if (submit_pending(*entry) == PendingSubmitResult::rejected) {
                rejected.push_back(std::move(entry->completions));
                entry = pending_.erase(entry);
            }
            else {
                ++entry;
            }
        }
        for (auto& completions: rejected) {
            for (auto& completion: completions) {
                completion({});
            }
        }
    }

    void TextureCache::clear() {
        ++generation_;
        pending_.clear();
        ready_uploads_.clear();
        inflight_decodes_ = 0;
        inflight_encoded_bytes_ = 0;
        ready_upload_bytes_ = 0;
        uploads_this_frame_ = 0;
        index_.clear();
        retained_.clear();
        retained_bytes_ = 0;
    }

    auto TextureCache::retained_entries() const noexcept -> std::size_t {
        return retained_.size();
    }

    auto TextureCache::retained_bytes() const noexcept -> std::size_t {
        return retained_bytes_;
    }

    auto TextureCache::find(const Key& key) -> std::shared_ptr<CachedTexture> {
        prune_expired();
        const auto found = std::ranges::find_if(index_, [&key](const IndexedEntry& entry) {
            return same_key(entry.key, key);
        });
        return found != index_.end() ? found->texture.lock() : nullptr;
    }

    void TextureCache::index(Key key, const std::shared_ptr<CachedTexture>& texture) {
        index_.push_back(IndexedEntry {.key = std::move(key), .texture = texture});
    }

    void TextureCache::retain(const Key& key, std::shared_ptr<CachedTexture> texture) {
        for (auto it = retained_.begin(); it != retained_.end(); ++it) {
            if (same_key(it->key, key)) {
                retained_bytes_ -= it->texture->estimated_bytes();
                retained_.erase(it);
                break;
            }
        }
        retained_bytes_ += texture->estimated_bytes();
        retained_.push_front(RetainedEntry {.key = key, .texture = std::move(texture)});
        trim();
    }

    void TextureCache::trim() {
        while (!retained_.empty()
               && (retained_.size() > limits_.max_retained_entries
                   || retained_bytes_ > limits_.max_retained_bytes))
        {
            retained_bytes_ -= retained_.back().texture->estimated_bytes();
            retained_.pop_back();
        }
        prune_expired();
    }

    void TextureCache::prune_expired() {
        std::erase_if(index_, [](const IndexedEntry& entry) { return entry.texture.expired(); });
    }

    void TextureCache::begin_frame() {
        uploads_this_frame_ = 0;
        drain_uploads();
        drain_pending();
    }

    void TextureCache::drain_uploads() {
        const auto budget = async_.max_uploads_per_frame;
        while (!ready_uploads_.empty() && (budget == 0 || uploads_this_frame_ < budget)) {
            auto ready = std::move(ready_uploads_.front());
            ready_uploads_.pop_front();
            ready_upload_bytes_ -= ready.decoded_bytes;
            if (ready.generation != generation_) {
                continue;
            }
            ++uploads_this_frame_;
            std::shared_ptr<CachedTexture> texture;
            if (ready.decoded.valid()) {
                const auto handle = device_->create_rgba_texture(
                    ready.decoded.width,
                    ready.decoded.height,
                    ready.decoded.rgba
                );
                if (handle) {
                    texture = std::make_shared<CachedTexture>(
                        *device_,
                        handle,
                        NanSize(
                            static_cast<float>(ready.decoded.width),
                            static_cast<float>(ready.decoded.height)
                        )
                    );
                    index(ready.key, texture);
                    retain(ready.key, texture);
                }
            }
            for (auto& completion: ready.completions) {
                completion(texture);
            }
        }
    }

    void TextureCache::finish_async(Key key, DecodedImage decoded, const std::uint64_t generation) {
        if (generation != generation_)
            return;
        const auto pending = std::ranges::find_if(pending_, [&key](const PendingEntry& entry) {
            return same_key(entry.key, key);
        });
        if (pending == pending_.end())
            return;
        if (pending->submitted) {
            --inflight_decodes_;
            inflight_encoded_bytes_ -= pending->encoded_bytes;
        }
        const auto decoded_bytes = decoded_byte_size(decoded);
        ready_uploads_.push_back(
            ReadyUpload {
                .key = key,
                .decoded = std::move(decoded),
                .decoded_bytes = decoded_bytes,
                .generation = generation,
                .completions = std::move(pending->completions),
            }
        );
        ready_upload_bytes_ += ready_uploads_.back().decoded_bytes;
        pending_.erase(pending);

        drain_uploads();
        drain_pending();
    }

    auto TextureCache::same_key(const Key& lhs, const Key& rhs) -> bool {
        return lhs.kind == rhs.kind && lhs.source == rhs.source && lhs.media_type == rhs.media_type
            && same_options(lhs.options, rhs.options);
    }
} // namespace nandina::render
