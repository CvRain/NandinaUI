#ifndef NANDINA_EXPERIMENT_RESOURCE_RESOURCE_STREAM_HPP
#define NANDINA_EXPERIMENT_RESOURCE_RESOURCE_STREAM_HPP

#include "resource.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <span>
#include <string>

namespace nandina::resource
{
    enum class ResourceStreamErrorCode { invalid_seek, io_error, closed };

    struct ResourceStreamError {
        ResourceStreamErrorCode code;
        std::string message;
    };

    class ResourceStream {
    public:
        virtual ~ResourceStream() = default;
        [[nodiscard]] virtual auto id() const noexcept -> ResourceId = 0;
        [[nodiscard]] virtual auto key() const noexcept -> const ResourceKey& = 0;
        [[nodiscard]] virtual auto media_type() const noexcept -> std::string_view = 0;
        [[nodiscard]] virtual auto storage() const noexcept -> ResourceStorage = 0;
        [[nodiscard]] virtual auto size() const noexcept -> std::uint64_t = 0;
        [[nodiscard]] virtual auto tell() const noexcept -> std::uint64_t = 0;
        [[nodiscard]] virtual auto seekable() const noexcept -> bool = 0;
        [[nodiscard]] virtual auto read(std::span<std::uint8_t> destination)
            -> std::expected<std::size_t, ResourceStreamError> = 0;
        [[nodiscard]] virtual auto seek(std::uint64_t position)
            -> std::expected<void, ResourceStreamError> = 0;
    };

    using ResourceStreamHandle = std::shared_ptr<ResourceStream>;

    [[nodiscard]] auto make_memory_resource_stream(ResourceHandle resource) -> ResourceStreamHandle;
} // namespace nandina::resource

#endif
