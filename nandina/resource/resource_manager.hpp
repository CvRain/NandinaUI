#ifndef NANDINA_EXPERIMENT_RESOURCE_RESOURCE_MANAGER_HPP
#define NANDINA_EXPERIMENT_RESOURCE_RESOURCE_MANAGER_HPP

#include "resource_backend.hpp"

#include <compare>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>

namespace nandina::resource
{
    /// Opaque identity of one live ResourceManager domain.
    ///
    /// Tokens hold the domain weakly: they never keep the manager alive. Equality compares the
    /// owning control block, not `lock().get()`, so two destroyed domains remain distinguishable
    /// while tokens still exist. Only ResourceManager can mint a non-empty token.
    class ResourceDomainToken {
    public:
        ResourceDomainToken() noexcept = default;

        [[nodiscard]] auto expired() const noexcept -> bool {
            return domain_.expired();
        }
        [[nodiscard]] auto owner_before(const ResourceDomainToken& other) const noexcept -> bool {
            return std::owner_less<std::weak_ptr<const void>> {}(domain_, other.domain_);
        }
        [[nodiscard]] auto operator<(const ResourceDomainToken& other) const noexcept -> bool {
            return owner_before(other);
        }
        [[nodiscard]] auto operator==(const ResourceDomainToken& other) const noexcept -> bool {
            return !owner_before(other) && !other.owner_before(*this);
        }
        auto operator<=>(const ResourceDomainToken& other) const noexcept {
            if (operator==(other)) {
                return std::strong_ordering::equal;
            }
            return operator<(other) ? std::strong_ordering::less : std::strong_ordering::greater;
        }

    private:
        explicit ResourceDomainToken(std::weak_ptr<const void> domain):
            domain_(std::move(domain)) {}

        std::weak_ptr<const void> domain_;
        friend class ResourceManager;
    };

    using ResourceMountId = std::uint64_t;

    /// One successful manager resolution: scope, publication identity and immutable content.
    struct ResolvedResource {
        ResourceDomainToken domain;
        ResourceMountId mount_id = 0;
        PublishGeneration generation;
        ResourceConsistency consistency = ResourceConsistency::non_snapshot;
        ResourceId id;
        ResourceHandle resource;
    };

    /// One successful stream resolution; the stream owns the bytes/handle it reads.
    struct ResolvedStream {
        ResourceDomainToken domain;
        ResourceMountId mount_id = 0;
        PublishGeneration generation;
        ResourceConsistency consistency = ResourceConsistency::non_snapshot;
        ResourceId id;
        ResourceStreamHandle stream;
    };

    using ResolvedResourceLookup = ResourceResult<std::optional<ResolvedResource>>;
    using ResolvedStreamLookup = ResourceResult<std::optional<ResolvedStream>>;

    /// Cache identity of one successful resolution.
    ///
    /// Shape: domain ownership identity + MountId + publish generation + record ID. Equality on
    /// two destroyed domains still distinguishes their control blocks. Callers add their own
    /// media/face/options fields around this value.
    struct ResourceScopeIdentity {
        ResourceDomainToken domain;
        ResourceMountId mount_id = 0;
        PublishGeneration generation;
        ResourceId id;
        auto operator<=>(const ResourceScopeIdentity&) const = default;
    };

    [[nodiscard]] inline auto scope_identity(const ResolvedResource& resolved)
        -> ResourceScopeIdentity {
        return {
            .domain = resolved.domain,
            .mount_id = resolved.mount_id,
            .generation = resolved.generation,
            .id = resolved.id,
        };
    }

    [[nodiscard]] inline auto scope_identity(const ResolvedStream& resolved)
        -> ResourceScopeIdentity {
        return {
            .domain = resolved.domain,
            .mount_id = resolved.mount_id,
            .generation = resolved.generation,
            .id = resolved.id,
        };
    }

    class ResourceManager {
    public:
        using MountId = ResourceMountId;
        ResourceManager();
        ~ResourceManager();
        ResourceManager(ResourceManager&&) noexcept;
        auto operator=(ResourceManager&&) noexcept -> ResourceManager&;
        ResourceManager(const ResourceManager&) = delete;

        [[nodiscard]] auto mount(std::shared_ptr<IResourceBackend> backend, int priority = 0)
            -> MountId;
        [[nodiscard]] auto unmount(MountId id) noexcept -> bool;
        void clear() noexcept;

        /// Opaque weak identity of this manager's domain, usable as a cache-key component.
        [[nodiscard]] auto domain() const -> ResourceDomainToken;

        [[nodiscard]] auto find(const ResourceKey& key) const -> ResolvedResourceLookup;
        [[nodiscard]] auto find(ResourceId id) const -> ResolvedResourceLookup;
        [[nodiscard]] auto require(const ResourceKey& key) const
            -> ResourceResult<ResolvedResource>;
        [[nodiscard]] auto require(ResourceId id) const -> ResourceResult<ResolvedResource>;
        [[nodiscard]] auto open_stream(const ResourceKey& key) const -> ResolvedStreamLookup;
        [[nodiscard]] auto open_stream(ResourceId id) const -> ResolvedStreamLookup;
        [[nodiscard]] auto require_stream(const ResourceKey& key) const
            -> ResourceResult<ResolvedStream>;
        [[nodiscard]] auto require_stream(ResourceId id) const -> ResourceResult<ResolvedStream>;

        /// Re-resolve inside the reference's original scope only.
        ///
        /// The query uses the recorded MountId and ResourceId; it never falls back to a global
        /// key/ID search. A foreign or destroyed domain, a missing mount, or a changed publish
        /// generation fails with `expired_scope`.
        [[nodiscard]] auto resolve(const ResolvedResource& reference) const
            -> ResolvedResourceLookup;
        [[nodiscard]] auto resolve(const ResolvedStream& reference) const -> ResolvedStreamLookup;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
} // namespace nandina::resource

#endif
