#ifndef NANDINA_EXPERIMENT_RESOURCE_RESOURCE_BACKEND_HPP
#define NANDINA_EXPERIMENT_RESOURCE_RESOURCE_BACKEND_HPP

#include "resource.hpp"
#include "resource_stream.hpp"

#include <expected>
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
    };

    struct ResourceError {
        ResourceErrorCode code;
        std::string operation;
        std::string message;
    };

    template<typename T>
    using ResourceResult = std::expected<T, ResourceError>;
    using ResourceLookup = ResourceResult<std::optional<ResourceHandle>>;
    using ResourceStreamLookup = ResourceResult<std::optional<ResourceStreamHandle>>;

    class IResourceBackend {
    public:
        virtual ~IResourceBackend() = default;
        [[nodiscard]] virtual auto name() const noexcept -> std::string_view = 0;
        [[nodiscard]] virtual auto find(const ResourceKey& key) const -> ResourceLookup = 0;
        [[nodiscard]] virtual auto find(ResourceId id) const -> ResourceLookup = 0;
        [[nodiscard]] virtual auto open_stream(const ResourceKey& key) const
            -> ResourceStreamLookup;
        [[nodiscard]] virtual auto open_stream(ResourceId id) const -> ResourceStreamLookup;
    };
} // namespace nandina::resource

#endif
