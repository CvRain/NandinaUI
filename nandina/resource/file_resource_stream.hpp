#ifndef NANDINA_EXPERIMENT_RESOURCE_FILE_RESOURCE_STREAM_HPP
#define NANDINA_EXPERIMENT_RESOURCE_FILE_RESOURCE_STREAM_HPP

#include "resource_backend.hpp"

#include <filesystem>

namespace nandina::resource
{
    struct FileResourceStreamOptions {
        ResourceId id;
        ResourceKey key;
        std::string media_type;
        ResourceStorage storage = ResourceStorage::external_file;
        std::filesystem::path path;
        std::uint64_t declared_size = 0;
    };

    [[nodiscard]] auto open_file_resource_stream(FileResourceStreamOptions options)
        -> ResourceResult<ResourceStreamHandle>;
} // namespace nandina::resource

#endif
