#include "file_resource_stream.hpp"

#include <algorithm>
#include <fstream>

namespace nandina::resource
{
    namespace
    {
        class FileResourceStream final: public ResourceStream {
        public:
            FileResourceStream(FileResourceStreamOptions options, std::ifstream stream):
                options_(std::move(options)),
                stream_(std::move(stream)) {}

            [[nodiscard]] auto id() const noexcept -> ResourceId override {
                return options_.id;
            }
            [[nodiscard]] auto key() const noexcept -> const ResourceKey& override {
                return options_.key;
            }
            [[nodiscard]] auto media_type() const noexcept -> std::string_view override {
                return options_.media_type;
            }
            [[nodiscard]] auto storage() const noexcept -> ResourceStorage override {
                return options_.storage;
            }
            [[nodiscard]] auto size() const noexcept -> std::uint64_t override {
                return options_.declared_size;
            }
            [[nodiscard]] auto tell() const noexcept -> std::uint64_t override {
                return position_;
            }
            [[nodiscard]] auto seekable() const noexcept -> bool override {
                return true;
            }

            auto read(std::span<std::uint8_t> destination)
                -> std::expected<std::size_t, ResourceStreamError> override {
                const auto remaining = size() - position_;
                const auto count = static_cast<std::size_t>(
                    std::min<std::uint64_t>(remaining, destination.size())
                );
                if (count == 0) {
                    return 0;
                }
                stream_.read(
                    reinterpret_cast<char*>(destination.data()),
                    static_cast<std::streamsize>(count)
                );
                const auto read = static_cast<std::size_t>(stream_.gcount());
                position_ += read;
                if (read != count) {
                    return std::unexpected(
                        ResourceStreamError {
                            ResourceStreamErrorCode::io_error,
                            "resource stream ended before its declared size",
                        }
                    );
                }
                return read;
            }

            auto seek(const std::uint64_t position)
                -> std::expected<void, ResourceStreamError> override {
                if (position > size()) {
                    return std::unexpected(
                        ResourceStreamError {
                            ResourceStreamErrorCode::invalid_seek,
                            "resource stream seek exceeds declared size",
                        }
                    );
                }
                stream_.clear();
                stream_.seekg(static_cast<std::streamoff>(position), std::ios::beg);
                if (!stream_) {
                    return std::unexpected(
                        ResourceStreamError {
                            ResourceStreamErrorCode::io_error,
                            "resource stream seek failed",
                        }
                    );
                }
                position_ = position;
                return {};
            }

        private:
            FileResourceStreamOptions options_;
            std::ifstream stream_;
            std::uint64_t position_ = 0;
        };
    } // namespace

    auto open_file_resource_stream(FileResourceStreamOptions options)
        -> ResourceResult<ResourceStreamHandle> {
        std::error_code error;
        const auto actual_size = std::filesystem::file_size(options.path, error);
        if (error) {
            return std::unexpected(
                ResourceError {
                    ResourceErrorCode::io_error,
                    "stream.open",
                    "resource file size cannot be read",
                }
            );
        }
        if (actual_size != options.declared_size) {
            return std::unexpected(
                ResourceError {
                    ResourceErrorCode::integrity_error,
                    "stream.open",
                    "resource file size does not match its declaration",
                }
            );
        }
        std::ifstream stream(options.path, std::ios::binary);
        if (!stream) {
            return std::unexpected(
                ResourceError {
                    ResourceErrorCode::io_error,
                    "stream.open",
                    "resource file cannot be opened",
                }
            );
        }
        return std::make_shared<FileResourceStream>(std::move(options), std::move(stream));
    }
} // namespace nandina::resource
