#include "resource_backend.hpp"

namespace nandina::resource
{
    auto IResourceBackend::open_stream(const ResourceKey& key) const
        -> ResourceStreamSnapshotLookup {
        auto snapshot = find(key);
        if (!snapshot) {
            return std::unexpected(snapshot.error());
        }
        if (!snapshot->resource) {
            return ResourceStreamSnapshot {
                snapshot->generation,
                snapshot->consistency,
                std::nullopt,
            };
        }
        return ResourceStreamSnapshot {
            snapshot->generation,
            snapshot->consistency,
            make_memory_resource_stream(*snapshot->resource),
        };
    }

    auto IResourceBackend::open_stream(const ResourceId id) const -> ResourceStreamSnapshotLookup {
        auto snapshot = find(id);
        if (!snapshot) {
            return std::unexpected(snapshot.error());
        }
        if (!snapshot->resource) {
            return ResourceStreamSnapshot {
                snapshot->generation,
                snapshot->consistency,
                std::nullopt,
            };
        }
        return ResourceStreamSnapshot {
            snapshot->generation,
            snapshot->consistency,
            make_memory_resource_stream(*snapshot->resource),
        };
    }
} // namespace nandina::resource
