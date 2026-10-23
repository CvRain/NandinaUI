#ifndef NANDINA_EXPERIMENT_RESOURCE_RESOURCE_BACKEND_HPP
#define NANDINA_EXPERIMENT_RESOURCE_RESOURCE_BACKEND_HPP

#include "resource.hpp"
#include "resource_stream.hpp"

#include <compare>
#include <cstdint>
#include <expected>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::resource
{
    enum class ResourceErrorCode {
        invalid_configuration,
        not_found,
        duplicate_id,
        duplicate_key,
        alias_conflict,
        io_error,
        size_limit_exceeded,
        malformed_backend,
        unsupported_schema,
        integrity_error,
        backend_unavailable,
        generation_exhausted,
        ambiguous_resource_id,
        expired_scope,
    };

    struct ResourceError {
        ResourceErrorCode code;
        std::string operation;
        std::string message;
    };

    template<typename T>
    using ResourceResult = std::expected<T, ResourceError>;

    /// Monotonic identity of one backend's published content.
    ///
    /// Zero is the backend's initial state; every successful publication advances the value by
    /// one. The counter never wraps: `next()` refuses once the maximum is reached, so an old
    /// cache identity can never be reused by a later publication.
    class PublishGeneration {
    public:
        using Value = std::uint64_t;

        constexpr PublishGeneration() noexcept = default;
        explicit constexpr PublishGeneration(Value value) noexcept: value_(value) {}

        [[nodiscard]] constexpr auto value() const noexcept -> Value {
            return value_;
        }
        [[nodiscard]] constexpr auto next() const noexcept -> std::optional<PublishGeneration> {
            if (value_ == std::numeric_limits<Value>::max()) {
                return std::nullopt;
            }
            return PublishGeneration {value_ + 1};
        }
        auto operator<=>(const PublishGeneration&) const = default;

    private:
        Value value_ = 0;
    };

    /// How the delivered bytes relate to the backend's publish generation.
    enum class ResourceConsistency {
        /// Bytes never change for the delivered handle (memory, builtin).
        immutable_snapshot,
        /// Bytes are immutable while the package generation stays the same (SQLite blobs).
        package_snapshot,
        /// The source may change outside the backend (directory, external files).
        ///
        /// Such a result must never enter a shared content cache keyed only by scope and
        /// generation: the same generation may yield different bytes.
        non_snapshot,
    };

    [[nodiscard]] constexpr auto is_snapshot(const ResourceConsistency consistency) noexcept
        -> bool {
        return consistency != ResourceConsistency::non_snapshot;
    }

    /// One lookup answer read atomically with the backend's publish generation.
    ///
    /// `resource` being empty is a negative result at `generation`; it is not an error.
    struct ResourceSnapshot {
        PublishGeneration generation;
        ResourceConsistency consistency = ResourceConsistency::non_snapshot;
        std::optional<ResourceHandle> resource;
    };

    /// One stream answer read atomically with the backend's publish generation.
    struct ResourceStreamSnapshot {
        PublishGeneration generation;
        ResourceConsistency consistency = ResourceConsistency::non_snapshot;
        std::optional<ResourceStreamHandle> stream;
    };

    using ResourceSnapshotLookup = ResourceResult<ResourceSnapshot>;
    using ResourceStreamSnapshotLookup = ResourceResult<ResourceStreamSnapshot>;

    class IResourceBackend {
    public:
        virtual ~IResourceBackend() = default;
        [[nodiscard]] virtual auto name() const noexcept -> std::string_view = 0;
        [[nodiscard]] virtual auto find(const ResourceKey& key) const -> ResourceSnapshotLookup = 0;
        [[nodiscard]] virtual auto find(ResourceId id) const -> ResourceSnapshotLookup = 0;
        [[nodiscard]] virtual auto open_stream(const ResourceKey& key) const
            -> ResourceStreamSnapshotLookup;
        [[nodiscard]] virtual auto open_stream(ResourceId id) const -> ResourceStreamSnapshotLookup;
    };
} // namespace nandina::resource

#endif
