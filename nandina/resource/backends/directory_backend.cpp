#include "directory_backend.hpp"
#include "../file_resource_stream.hpp"

#include <fstream>
#include <map>

namespace nandina::resource
{
    class DirectoryBackend::Impl {
    public:
        DirectoryBackendOptions options;
        std::map<ResourceId, DirectoryResource> ids;
        std::map<ResourceKey, ResourceId> keys;
        std::map<ResourceKey, ResourceId> aliases;

        [[nodiscard]] auto load(const DirectoryResource& entry) const -> ResourceLookup {
            const auto path = options.root / entry.relative_path;
            std::error_code error;
            if (!options.allow_symlinks && std::filesystem::is_symlink(path, error)) {
                return std::unexpected(
                    ResourceError {
                        ResourceErrorCode::io_error,
                        "directory.load",
                        "resource path is a symlink"
                    }
                );
            }
            const auto size = std::filesystem::file_size(path, error);
            if (error) {
                return std::unexpected(
                    ResourceError {
                        ResourceErrorCode::io_error,
                        "directory.load",
                        "resource file cannot be read"
                    }
                );
            }
            if (size > options.max_resource_size) {
                return std::unexpected(
                    ResourceError {
                        ResourceErrorCode::size_limit_exceeded,
                        "directory.load",
                        "resource exceeds configured size limit"
                    }
                );
            }
            if (entry.expected_size && size != *entry.expected_size) {
                return std::unexpected(
                    ResourceError {
                        ResourceErrorCode::integrity_error,
                        "directory.load",
                        "resource size does not match manifest"
                    }
                );
            }
            std::ifstream stream(path, std::ios::binary);
            if (!stream) {
                return std::unexpected(
                    ResourceError {
                        ResourceErrorCode::io_error,
                        "directory.load",
                        "resource file cannot be opened"
                    }
                );
            }
            std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
            if (size > 0
                && !stream.read(
                    reinterpret_cast<char*>(bytes.data()),
                    static_cast<std::streamsize>(size)
                ))
            {
                return std::unexpected(
                    ResourceError {
                        ResourceErrorCode::io_error,
                        "directory.load",
                        "resource file read failed"
                    }
                );
            }
            return std::optional<ResourceHandle> {std::make_shared<const Resource>(
                entry.id,
                entry.key,
                entry.media_type,
                ResourceStorage::external_file,
                std::move(bytes)
            )};
        }

        [[nodiscard]] auto stream(const DirectoryResource& entry) const -> ResourceStreamLookup {
            const auto path = options.root / entry.relative_path;
            std::error_code error;
            if (!options.allow_symlinks && std::filesystem::is_symlink(path, error)) {
                return std::unexpected(
                    ResourceError {
                        ResourceErrorCode::io_error,
                        "directory.stream",
                        "resource path is a symlink"
                    }
                );
            }
            const auto size = std::filesystem::file_size(path, error);
            if (error) {
                return std::unexpected(
                    ResourceError {
                        ResourceErrorCode::io_error,
                        "directory.stream",
                        "resource file cannot be read"
                    }
                );
            }
            if (size > options.max_stream_size) {
                return std::unexpected(
                    ResourceError {
                        ResourceErrorCode::size_limit_exceeded,
                        "directory.stream",
                        "resource exceeds configured stream size limit"
                    }
                );
            }
            if (entry.expected_size && size != *entry.expected_size) {
                return std::unexpected(
                    ResourceError {
                        ResourceErrorCode::integrity_error,
                        "directory.stream",
                        "resource size does not match manifest"
                    }
                );
            }
            auto opened = open_file_resource_stream({
                .id = entry.id,
                .key = entry.key,
                .media_type = entry.media_type,
                .path = path,
                .declared_size = size,
            });
            if (!opened) {
                return std::unexpected(opened.error());
            }
            return std::optional<ResourceStreamHandle> {*opened};
        }
    };

    DirectoryBackend::DirectoryBackend(std::unique_ptr<Impl> impl): impl_(std::move(impl)) {}
    DirectoryBackend::~DirectoryBackend() = default;

    auto DirectoryBackend::open(DirectoryBackendOptions options)
        -> ResourceResult<std::shared_ptr<DirectoryBackend>> {
        if (options.root.empty() || options.max_resource_size == 0 || options.max_stream_size == 0)
        {
            return std::unexpected(
                ResourceError {
                    ResourceErrorCode::invalid_configuration,
                    "directory.open",
                    "invalid root or size limit"
                }
            );
        }
        auto impl = std::make_unique<Impl>();
        impl->options = std::move(options);
        for (const auto& entry: impl->options.resources) {
            if (entry.id.is_nil() || entry.relative_path.empty()
                || entry.relative_path.is_absolute())
            {
                return std::unexpected(
                    ResourceError {
                        ResourceErrorCode::invalid_configuration,
                        "directory.open",
                        "invalid manifest entry"
                    }
                );
            }
            for (const auto& part: entry.relative_path) {
                if (part == ".." || part == ".") {
                    return std::unexpected(
                        ResourceError {
                            ResourceErrorCode::invalid_configuration,
                            "directory.open",
                            "resource path contains traversal"
                        }
                    );
                }
            }
            if (impl->ids.contains(entry.id) || impl->keys.contains(entry.key)
                || impl->aliases.contains(entry.key))
            {
                return std::unexpected(
                    ResourceError {
                        ResourceErrorCode::duplicate_key,
                        "directory.open",
                        "duplicate manifest identity"
                    }
                );
            }
            impl->ids.emplace(entry.id, entry);
            impl->keys.emplace(entry.key, entry.id);
            for (const auto& alias: entry.aliases) {
                if (impl->keys.contains(alias) || !impl->aliases.emplace(alias, entry.id).second) {
                    return std::unexpected(
                        ResourceError {
                            ResourceErrorCode::alias_conflict,
                            "directory.open",
                            "duplicate manifest alias"
                        }
                    );
                }
            }
        }
        return std::shared_ptr<DirectoryBackend>(new DirectoryBackend(std::move(impl)));
    }

    auto DirectoryBackend::name() const noexcept -> std::string_view {
        return impl_->options.name;
    }
    auto DirectoryBackend::find(const ResourceKey& key) const -> ResourceLookup {
        auto canonical = impl_->keys.find(key);
        if (canonical != impl_->keys.end()) {
            return impl_->load(impl_->ids.at(canonical->second));
        }
        const auto alias = impl_->aliases.find(key);
        return alias == impl_->aliases.end() ? ResourceLookup(std::optional<ResourceHandle> {})
                                             : impl_->load(impl_->ids.at(alias->second));
    }
    auto DirectoryBackend::find(const ResourceId id) const -> ResourceLookup {
        const auto found = impl_->ids.find(id);
        return found == impl_->ids.end() ? ResourceLookup(std::optional<ResourceHandle> {})
                                         : impl_->load(found->second);
    }
    auto DirectoryBackend::open_stream(const ResourceKey& key) const -> ResourceStreamLookup {
        auto canonical = impl_->keys.find(key);
        if (canonical != impl_->keys.end()) {
            return impl_->stream(impl_->ids.at(canonical->second));
        }
        const auto alias = impl_->aliases.find(key);
        return alias == impl_->aliases.end()
            ? ResourceStreamLookup(std::optional<ResourceStreamHandle> {})
            : impl_->stream(impl_->ids.at(alias->second));
    }
    auto DirectoryBackend::open_stream(const ResourceId id) const -> ResourceStreamLookup {
        const auto found = impl_->ids.find(id);
        return found == impl_->ids.end()
            ? ResourceStreamLookup(std::optional<ResourceStreamHandle> {})
            : impl_->stream(found->second);
    }
} // namespace nandina::resource
