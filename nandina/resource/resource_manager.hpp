#ifndef NANDINA_EXPERIMENT_RESOURCE_RESOURCE_MANAGER_HPP
#define NANDINA_EXPERIMENT_RESOURCE_RESOURCE_MANAGER_HPP

#include "resource_backend.hpp"

#include <cstdint>
#include <memory>

namespace nandina::resource
{
    class ResourceManager {
    public:
        using MountId = std::uint64_t;
        ResourceManager();
        ~ResourceManager();
        ResourceManager(ResourceManager&&) noexcept;
        auto operator=(ResourceManager&&) noexcept -> ResourceManager&;
        ResourceManager(const ResourceManager&) = delete;

        [[nodiscard]] auto mount(std::shared_ptr<IResourceBackend> backend, int priority = 0)
            -> MountId;
        [[nodiscard]] auto unmount(MountId id) noexcept -> bool;
        void clear() noexcept;
        [[nodiscard]] auto find(const ResourceKey& key) const -> ResourceLookup;
        [[nodiscard]] auto find(ResourceId id) const -> ResourceLookup;
        [[nodiscard]] auto require(const ResourceKey& key) const -> ResourceResult<ResourceHandle>;
        [[nodiscard]] auto require(ResourceId id) const -> ResourceResult<ResourceHandle>;
        [[nodiscard]] auto open_stream(const ResourceKey& key) const -> ResourceStreamLookup;
        [[nodiscard]] auto open_stream(ResourceId id) const -> ResourceStreamLookup;
        [[nodiscard]] auto require_stream(const ResourceKey& key) const
            -> ResourceResult<ResourceStreamHandle>;
        [[nodiscard]] auto require_stream(ResourceId id) const
            -> ResourceResult<ResourceStreamHandle>;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
} // namespace nandina::resource

#endif
