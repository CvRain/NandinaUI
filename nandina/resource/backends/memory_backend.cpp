#include "memory_backend.hpp"

#include <map>
#include <mutex>
#include <shared_mutex>

namespace nandina::resource
{
    class MemoryBackend::Impl {
    public:
        explicit Impl(std::string value): name(std::move(value)) {}
        std::string name;
        mutable std::shared_mutex mutex;
        std::map<ResourceId, ResourceHandle> ids;
        std::map<ResourceKey, ResourceId> keys;
        std::map<ResourceKey, ResourceId> aliases;
    };

    MemoryBackend::MemoryBackend(std::string name):
        impl_(std::make_unique<Impl>(std::move(name))) {}
    MemoryBackend::~MemoryBackend() = default;
    auto MemoryBackend::name() const noexcept -> std::string_view {
        return impl_->name;
    }

    auto MemoryBackend::insert(
        const ResourceId id,
        ResourceKey key,
        std::string media_type,
        std::vector<std::uint8_t> bytes,
        const InsertMode mode
    ) -> ResourceResult<ResourceHandle> {
        if (id.is_nil()) {
            return std::unexpected(
                ResourceError {
                    ResourceErrorCode::invalid_configuration,
                    "insert",
                    "resource id is nil"
                }
            );
        }
        std::unique_lock lock(impl_->mutex);
        const auto id_entry = impl_->ids.find(id);
        if (impl_->aliases.contains(key)) {
            return std::unexpected(
                ResourceError {
                    ResourceErrorCode::alias_conflict,
                    "insert",
                    "resource key collides with an alias"
                }
            );
        }
        if (mode == InsertMode::fail_if_present) {
            if (id_entry != impl_->ids.end()) {
                return std::unexpected(
                    ResourceError {
                        ResourceErrorCode::duplicate_id,
                        "insert",
                        "resource id already exists"
                    }
                );
            }
            if (impl_->keys.contains(key)) {
                return std::unexpected(
                    ResourceError {
                        ResourceErrorCode::duplicate_key,
                        "insert",
                        "resource key already exists"
                    }
                );
            }
        }
        if (id_entry != impl_->ids.end()) {
            impl_->keys.erase(id_entry->second->key());
        }
        // 上面的 erase 可能已删除 key 对应的映射，须重新查询以避免悬空迭代器。
        const auto key_entry = impl_->keys.find(key);
        if (key_entry != impl_->keys.end() && key_entry->second != id) {
            const auto replaced_id = key_entry->second;
            std::erase_if(impl_->aliases, [replaced_id](const auto& entry) {
                return entry.second == replaced_id;
            });
            impl_->ids.erase(key_entry->second);
        }
        auto resource = std::make_shared<const Resource>(
            id,
            std::move(key),
            std::move(media_type),
            ResourceStorage::memory,
            std::move(bytes)
        );
        impl_->ids[id] = resource;
        impl_->keys[resource->key()] = id;
        return resource;
    }

    auto MemoryBackend::add_alias(ResourceKey alias, const ResourceId target)
        -> ResourceResult<void> {
        std::unique_lock lock(impl_->mutex);
        if (!impl_->ids.contains(target)) {
            return std::unexpected(
                ResourceError {
                    ResourceErrorCode::not_found,
                    "add_alias",
                    "alias target was not found"
                }
            );
        }
        if (impl_->keys.contains(alias) || impl_->aliases.contains(alias)) {
            return std::unexpected(
                ResourceError {
                    ResourceErrorCode::alias_conflict,
                    "add_alias",
                    "alias already exists"
                }
            );
        }
        impl_->aliases.emplace(std::move(alias), target);
        return {};
    }

    auto MemoryBackend::erase(const ResourceId id) -> bool {
        std::unique_lock lock(impl_->mutex);
        const auto found = impl_->ids.find(id);
        if (found == impl_->ids.end()) {
            return false;
        }
        impl_->keys.erase(found->second->key());
        std::erase_if(impl_->aliases, [id](const auto& entry) { return entry.second == id; });
        impl_->ids.erase(found);
        return true;
    }

    auto MemoryBackend::find(const ResourceKey& key) const -> ResourceLookup {
        std::shared_lock lock(impl_->mutex);
        auto canonical = impl_->keys.find(key);
        ResourceId id;
        if (canonical != impl_->keys.end()) {
            id = canonical->second;
        }
        else {
            const auto alias = impl_->aliases.find(key);
            if (alias == impl_->aliases.end()) {
                return std::optional<ResourceHandle> {};
            }
            id = alias->second;
        }
        return std::optional<ResourceHandle> {impl_->ids.at(id)};
    }
    auto MemoryBackend::find(const ResourceId id) const -> ResourceLookup {
        std::shared_lock lock(impl_->mutex);
        const auto found = impl_->ids.find(id);
        return found == impl_->ids.end()
            ? ResourceLookup(std::optional<ResourceHandle> {})
            : ResourceLookup(std::optional<ResourceHandle> {found->second});
    }
} // namespace nandina::resource
