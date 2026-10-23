#include "resource_manager.hpp"

#include <algorithm>
#include <format>
#include <limits>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace nandina::resource
{
    namespace
    {
        [[nodiscard]] auto not_found_error(std::string operation, std::string message)
            -> ResourceError {
            return {ResourceErrorCode::not_found, std::move(operation), std::move(message)};
        }

        [[nodiscard]] auto expired_scope_error(std::string operation, std::string message)
            -> ResourceError {
            return {ResourceErrorCode::expired_scope, std::move(operation), std::move(message)};
        }

        struct ConflictingMount {
            ResourceMountId mount_id;
            std::string backend;
        };

        [[nodiscard]] auto
        ambiguous_id_error(std::string operation, const std::vector<ConflictingMount>& conflicts)
            -> ResourceError {
            std::string mounts;
            for (const auto& conflict: conflicts) {
                if (!mounts.empty()) {
                    mounts += ", ";
                }
                mounts += std::format("mount {} ({})", conflict.mount_id, conflict.backend);
            }
            return {
                ResourceErrorCode::ambiguous_resource_id,
                std::move(operation),
                std::format(
                    "resource id is ambiguous across {} mounts: {}",
                    conflicts.size(),
                    mounts
                ),
            };
        }
    } // namespace

    class ResourceManager::Impl {
    public:
        /// Empty owner type: tokens compare its control block, they never see its contents.
        struct Domain {};

        struct Mount {
            MountId id;
            int priority;
            std::uint64_t order;
            std::shared_ptr<IResourceBackend> backend;
        };

        std::shared_ptr<Domain> domain = std::make_shared<Domain>();
        mutable std::shared_mutex mutex;
        std::vector<Mount> mounts;
        MountId next_id = 1;
        std::uint64_t next_order = 0;

        [[nodiscard]] auto snapshot_mounts() const -> std::vector<Mount> {
            std::shared_lock lock(mutex);
            return mounts;
        }

        [[nodiscard]] static auto make_resolved(
            const Mount& mount,
            const ResourceSnapshot& snapshot,
            const ResourceDomainToken& domain
        ) -> ResolvedResource {
            return {
                .domain = domain,
                .mount_id = mount.id,
                .generation = snapshot.generation,
                .consistency = snapshot.consistency,
                .id = (*snapshot.resource)->id(),
                .resource = *snapshot.resource,
            };
        }

        [[nodiscard]] static auto make_resolved(
            const Mount& mount,
            const ResourceStreamSnapshot& snapshot,
            const ResourceDomainToken& domain
        ) -> ResolvedStream {
            return {
                .domain = domain,
                .mount_id = mount.id,
                .generation = snapshot.generation,
                .consistency = snapshot.consistency,
                .id = (*snapshot.stream)->id(),
                .stream = *snapshot.stream,
            };
        }

        [[nodiscard]] auto find_key(const ResourceKey& key, const ResourceDomainToken& domain) const
            -> ResolvedResourceLookup {
            // key 查询按挂载优先级短路：第一个命中即胜者，别名沿该后端自己的路径解析。
            for (const auto& mount: snapshot_mounts()) {
                auto snapshot = mount.backend->find(key);
                if (!snapshot) {
                    return std::unexpected(std::move(snapshot.error()));
                }
                if (snapshot->resource) {
                    return std::optional<ResolvedResource> {
                        make_resolved(mount, *snapshot, domain)
                    };
                }
            }
            return std::optional<ResolvedResource> {};
        }

        [[nodiscard]] auto find_id(const ResourceId id, const ResourceDomainToken& domain) const
            -> ResolvedResourceLookup {
            // 裸 ID 必须扫描全部挂载：priority 不裁决平手，同一后端挂两次也是两个命中。
            std::vector<ResolvedResource> hits;
            std::vector<ConflictingMount> conflicts;
            const auto mounted = snapshot_mounts();
            for (const auto& mount: mounted) {
                auto snapshot = mount.backend->find(id);
                if (!snapshot) {
                    return std::unexpected(std::move(snapshot.error()));
                }
                if (!snapshot->resource) {
                    continue;
                }
                hits.push_back(make_resolved(mount, *snapshot, domain));
                conflicts.push_back(
                    ConflictingMount {
                        .mount_id = mount.id,
                        .backend = std::string(mount.backend->name()),
                    }
                );
            }
            if (hits.empty()) {
                return std::optional<ResolvedResource> {};
            }
            if (hits.size() > 1) {
                return std::unexpected(ambiguous_id_error("find", conflicts));
            }
            return std::optional<ResolvedResource> {std::move(hits.front())};
        }

        [[nodiscard]] auto open_key(const ResourceKey& key, const ResourceDomainToken& domain) const
            -> ResolvedStreamLookup {
            for (const auto& mount: snapshot_mounts()) {
                auto snapshot = mount.backend->open_stream(key);
                if (!snapshot) {
                    return std::unexpected(std::move(snapshot.error()));
                }
                if (snapshot->stream) {
                    return std::optional<ResolvedStream> {make_resolved(mount, *snapshot, domain)};
                }
            }
            return std::optional<ResolvedStream> {};
        }

        [[nodiscard]] auto open_id(const ResourceId id, const ResourceDomainToken& domain) const
            -> ResolvedStreamLookup {
            std::vector<ResolvedStream> hits;
            std::vector<ConflictingMount> conflicts;
            for (const auto& mount: snapshot_mounts()) {
                auto snapshot = mount.backend->open_stream(id);
                if (!snapshot) {
                    return std::unexpected(std::move(snapshot.error()));
                }
                if (!snapshot->stream) {
                    continue;
                }
                hits.push_back(make_resolved(mount, *snapshot, domain));
                conflicts.push_back(
                    ConflictingMount {
                        .mount_id = mount.id,
                        .backend = std::string(mount.backend->name()),
                    }
                );
            }
            if (hits.empty()) {
                return std::optional<ResolvedStream> {};
            }
            if (hits.size() > 1) {
                return std::unexpected(ambiguous_id_error("open_stream", conflicts));
            }
            return std::optional<ResolvedStream> {std::move(hits.front())};
        }

        [[nodiscard]] auto resolve_resource(
            const ResourceDomainToken& current,
            const ResolvedResource& reference
        ) const -> ResolvedResourceLookup {
            if (!(reference.domain == current)) {
                return std::unexpected(
                    expired_scope_error("resolve", "resource domain is no longer alive")
                );
            }
            const auto mounted = snapshot_mounts();
            const auto found = std::ranges::find(mounted, reference.mount_id, &Mount::id);
            if (found == mounted.end()) {
                return std::unexpected(expired_scope_error(
                    "resolve",
                    std::format("resource mount {} is no longer active", reference.mount_id)
                ));
            }
            auto snapshot = found->backend->find(reference.id);
            if (!snapshot) {
                return std::unexpected(std::move(snapshot.error()));
            }
            if (!(snapshot->generation == reference.generation)) {
                return std::unexpected(expired_scope_error(
                    "resolve",
                    std::format(
                        "resource generation changed from {} to {}",
                        reference.generation.value(),
                        snapshot->generation.value()
                    )
                ));
            }
            if (!snapshot->resource) {
                return std::unexpected(
                    not_found_error("resolve", "resource is no longer present in its mount")
                );
            }
            return std::optional<ResolvedResource> {make_resolved(*found, *snapshot, current)};
        }

        [[nodiscard]] auto
        resolve_stream(const ResourceDomainToken& current, const ResolvedStream& reference) const
            -> ResolvedStreamLookup {
            if (!(reference.domain == current)) {
                return std::unexpected(
                    expired_scope_error("resolve", "stream domain is no longer alive")
                );
            }
            const auto mounted = snapshot_mounts();
            const auto found = std::ranges::find(mounted, reference.mount_id, &Mount::id);
            if (found == mounted.end()) {
                return std::unexpected(expired_scope_error(
                    "resolve",
                    std::format("stream mount {} is no longer active", reference.mount_id)
                ));
            }
            auto snapshot = found->backend->open_stream(reference.id);
            if (!snapshot) {
                return std::unexpected(std::move(snapshot.error()));
            }
            if (!(snapshot->generation == reference.generation)) {
                return std::unexpected(expired_scope_error(
                    "resolve",
                    std::format(
                        "stream generation changed from {} to {}",
                        reference.generation.value(),
                        snapshot->generation.value()
                    )
                ));
            }
            if (!snapshot->stream) {
                return std::unexpected(
                    not_found_error("resolve", "stream is no longer present in its mount")
                );
            }
            return std::optional<ResolvedStream> {make_resolved(*found, *snapshot, current)};
        }
    };

    ResourceManager::ResourceManager(): impl_(std::make_unique<Impl>()) {}
    ResourceManager::~ResourceManager() = default;
    ResourceManager::ResourceManager(ResourceManager&&) noexcept = default;
    auto ResourceManager::operator=(ResourceManager&&) noexcept -> ResourceManager& = default;

    auto ResourceManager::domain_token() const -> ResourceDomainToken {
        return ResourceDomainToken {std::weak_ptr<const void> {impl_->domain}};
    }

    auto ResourceManager::mount(std::shared_ptr<IResourceBackend> backend, const int priority)
        -> MountId {
        if (!backend) {
            throw std::invalid_argument("resource backend is null");
        }
        std::unique_lock lock(impl_->mutex);
        if (impl_->next_id == std::numeric_limits<MountId>::max()) {
            throw std::overflow_error("resource mount id space is exhausted");
        }
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

    auto ResourceManager::find(const ResourceKey& key) const -> ResolvedResourceLookup {
        return impl_->find_key(key, domain_token());
    }
    auto ResourceManager::find(const ResourceId id) const -> ResolvedResourceLookup {
        return impl_->find_id(id, domain_token());
    }
    auto ResourceManager::require(const ResourceKey& key) const
        -> ResourceResult<ResolvedResource> {
        auto found = find(key);
        if (!found) {
            return std::unexpected(found.error());
        }
        if (!*found) {
            return std::unexpected(not_found_error("require", "resource key was not found"));
        }
        return std::move(**found);
    }
    auto ResourceManager::require(const ResourceId id) const -> ResourceResult<ResolvedResource> {
        auto found = find(id);
        if (!found) {
            return std::unexpected(found.error());
        }
        if (!*found) {
            return std::unexpected(not_found_error("require", "resource id was not found"));
        }
        return std::move(**found);
    }
    auto ResourceManager::open_stream(const ResourceKey& key) const -> ResolvedStreamLookup {
        return impl_->open_key(key, domain_token());
    }
    auto ResourceManager::open_stream(const ResourceId id) const -> ResolvedStreamLookup {
        return impl_->open_id(id, domain_token());
    }
    auto ResourceManager::require_stream(const ResourceKey& key) const
        -> ResourceResult<ResolvedStream> {
        auto found = open_stream(key);
        if (!found) {
            return std::unexpected(found.error());
        }
        if (!*found) {
            return std::unexpected(not_found_error("require_stream", "resource key was not found"));
        }
        return std::move(**found);
    }
    auto ResourceManager::require_stream(const ResourceId id) const
        -> ResourceResult<ResolvedStream> {
        auto found = open_stream(id);
        if (!found) {
            return std::unexpected(found.error());
        }
        if (!*found) {
            return std::unexpected(not_found_error("require_stream", "resource id was not found"));
        }
        return std::move(**found);
    }

    auto ResourceManager::resolve(const ResolvedResource& reference) const
        -> ResolvedResourceLookup {
        return impl_->resolve_resource(domain_token(), reference);
    }
    auto ResourceManager::resolve(const ResolvedStream& reference) const -> ResolvedStreamLookup {
        return impl_->resolve_stream(domain_token(), reference);
    }
} // namespace nandina::resource
