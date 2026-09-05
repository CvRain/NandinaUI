#ifndef NANDINA_EXPERIMENT_RESOURCE_BACKENDS_DIRECTORY_BACKEND_HPP
#define NANDINA_EXPERIMENT_RESOURCE_BACKENDS_DIRECTORY_BACKEND_HPP

#include "../resource_backend.hpp"

#include <filesystem>
#include <memory>

namespace nandina::resource
{
    struct DirectoryResource {
        ResourceId id;
        ResourceKey key;
        std::filesystem::path relative_path;
        std::string media_type;
        std::vector<ResourceKey> aliases;
        std::optional<std::uint64_t> expected_size;
    };

    struct DirectoryBackendOptions {
        std::string name = "directory";
        std::filesystem::path root;
        std::vector<DirectoryResource> resources;
        std::uint64_t max_resource_size = 64U * 1024U * 1024U;
        std::uint64_t max_stream_size = 4ULL * 1024ULL * 1024ULL * 1024ULL;
        bool allow_symlinks = false;
    };

    class DirectoryBackend final: public IResourceBackend {
    public:
        [[nodiscard]] static auto open(DirectoryBackendOptions options)
            -> ResourceResult<std::shared_ptr<DirectoryBackend>>;
        ~DirectoryBackend() override;
        [[nodiscard]] auto name() const noexcept -> std::string_view override;
        [[nodiscard]] auto find(const ResourceKey& key) const -> ResourceLookup override;
        [[nodiscard]] auto find(ResourceId id) const -> ResourceLookup override;
        [[nodiscard]] auto open_stream(const ResourceKey& key) const
            -> ResourceStreamLookup override;
        [[nodiscard]] auto open_stream(ResourceId id) const -> ResourceStreamLookup override;

    private:
        class Impl;
        explicit DirectoryBackend(std::unique_ptr<Impl> impl);
        std::unique_ptr<Impl> impl_;
    };
} // namespace nandina::resource
#endif
