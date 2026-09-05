#include "resource_manager.hpp"

#include <algorithm>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <vector>

namespace nandina::resource
{
    class ResourceManager::Impl {
    public:
        struct Mount {
            MountId id;
            int priority;
            std::uint64_t order;
            std::shared_ptr<IResourceBackend> backend;
        };
        mutable std::shared_mutex mutex;
        std::vector<Mount> mounts;
        MountId next_id = 1;
        std::uint64_t next_order = 0;

        template<typename Lookup, typename Query>
        [[nodiscard]] auto lookup(Query&& query) const -> Lookup {
            std::vector<std::shared_ptr<IResourceBackend>> backends;
            {
                std::shared_lock lock(mutex);
                for (const auto& mount: mounts) {
                    backends.push_back(mount.backend);
                }
            }
            for (const auto& backend: backends) {
                auto result = query(*backend);
                if (!result || result->has_value()) {
                    return result;
                }
            }
            return typename Lookup::value_type {};
        }
    };

    ResourceManager::ResourceManager(): impl_(std::make_unique<Impl>()) {}
    ResourceManager::~ResourceManager() = default;
    ResourceManager::ResourceManager(ResourceManager&&) noexcept = default;
    auto ResourceManager::operator=(ResourceManager&&) noexcept -> ResourceManager& = default;

    auto ResourceManager::mount(std::shared_ptr<IResourceBackend> backend, const int priority)
        -> MountId {
        if (!backend) {
            throw std::invalid_argument("resource backend is null");
        }
        std::unique_lock lock(impl_->mutex);
        const auto id = impl_->next_id++;
        impl_->mounts.push_back({id, priority, impl_->next_order++, std::move(backend)});
        std::ranges::stable_sort(impl_->mounts, [](const auto& left, const auto& right) {
            return left.priority > right.priority;
        });
        return id;
    }

    auto ResourceManager::unmount(const MountId id) noexcept -> bool {
        std::unique_lock lock(impl_->mutex);
        return std::erase_if(impl_->mounts, [id](const auto& mount) { return mount.id == id; }) > 0;
    }
    void ResourceManager::clear() noexcept {
        std::unique_lock lock(impl_->mutex);
        impl_->mounts.clear();
    }

    auto ResourceManager::find(const ResourceKey& key) const -> ResourceLookup {
        return impl_->lookup<ResourceLookup>([&](const auto& backend) {
            return backend.find(key);
        });
    }
    auto ResourceManager::find(const ResourceId id) const -> ResourceLookup {
        return impl_->lookup<ResourceLookup>([&](const auto& backend) { return backend.find(id); });
    }
    auto ResourceManager::require(const ResourceKey& key) const -> ResourceResult<ResourceHandle> {
        auto found = find(key);
        if (!found) {
            return std::unexpected(found.error());
        }
        if (!*found) {
            return std::unexpected(
                ResourceError {
                    ResourceErrorCode::not_found,
                    "require",
                    "resource key was not found"
                }
            );
        }
        return **found;
    }
    auto ResourceManager::require(const ResourceId id) const -> ResourceResult<ResourceHandle> {
        auto found = find(id);
        if (!found) {
            return std::unexpected(found.error());
        }
        if (!*found) {
            return std::unexpected(
                ResourceError {ResourceErrorCode::not_found, "require", "resource id was not found"}
            );
        }
        return **found;
    }
    auto ResourceManager::open_stream(const ResourceKey& key) const -> ResourceStreamLookup {
        return impl_->lookup<ResourceStreamLookup>([&](const auto& backend) {
            return backend.open_stream(key);
        });
    }
    auto ResourceManager::open_stream(const ResourceId id) const -> ResourceStreamLookup {
        return impl_->lookup<ResourceStreamLookup>([&](const auto& backend) {
            return backend.open_stream(id);
        });
    }
    auto ResourceManager::require_stream(const ResourceKey& key) const
        -> ResourceResult<ResourceStreamHandle> {
        auto found = open_stream(key);
        if (!found) {
            return std::unexpected(found.error());
        }
        if (!*found) {
            return std::unexpected(
                ResourceError {
                    ResourceErrorCode::not_found,
                    "require_stream",
                    "resource key was not found"
                }
            );
        }
        return **found;
    }
    auto ResourceManager::require_stream(const ResourceId id) const
        -> ResourceResult<ResourceStreamHandle> {
        auto found = open_stream(id);
        if (!found) {
            return std::unexpected(found.error());
        }
        if (!*found) {
            return std::unexpected(
                ResourceError {
                    ResourceErrorCode::not_found,
                    "require_stream",
                    "resource id was not found"
                }
            );
        }
        return **found;
    }
} // namespace nandina::resource
