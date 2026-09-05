#include "resource_backend.hpp"

namespace nandina::resource
{
    auto IResourceBackend::open_stream(const ResourceKey& key) const -> ResourceStreamLookup {
        auto resource = find(key);
        if (!resource) {
            return std::unexpected(resource.error());
        }
        if (!*resource) {
            return std::optional<ResourceStreamHandle> {};
        }
        return std::optional<ResourceStreamHandle> {make_memory_resource_stream(**resource)};
    }

    auto IResourceBackend::open_stream(const ResourceId id) const -> ResourceStreamLookup {
        auto resource = find(id);
        if (!resource) {
            return std::unexpected(resource.error());
        }
        if (!*resource) {
            return std::optional<ResourceStreamHandle> {};
        }
        return std::optional<ResourceStreamHandle> {make_memory_resource_stream(**resource)};
    }
} // namespace nandina::resource
