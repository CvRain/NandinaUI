#include "resource_stream.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace nandina::resource
{
    namespace
    {
        class MemoryResourceStream final: public ResourceStream {
        public:
            explicit MemoryResourceStream(ResourceHandle resource): resource_(std::move(resource)) {
                if (!resource_) {
                    throw std::invalid_argument("resource stream handle is null");
                }
            }

            [[nodiscard]] auto id() const noexcept -> ResourceId override {
                return resource_->id();
            }
            [[nodiscard]] auto key() const noexcept -> const ResourceKey& override {
                return resource_->key();
            }
            [[nodiscard]] auto media_type() const noexcept -> std::string_view override {
                return resource_->media_type();
            }
            [[nodiscard]] auto storage() const noexcept -> ResourceStorage override {
                return resource_->storage();
            }
            [[nodiscard]] auto size() const noexcept -> std::uint64_t override {
                return resource_->size();
            }
            [[nodiscard]] auto tell() const noexcept -> std::uint64_t override {
                return position_;
            }
            [[nodiscard]] auto seekable() const noexcept -> bool override {
                return true;
            }

            auto read(std::span<std::uint8_t> destination)
                -> std::expected<std::size_t, ResourceStreamError> override {
                const auto bytes = resource_->bytes();
                const auto remaining = bytes.size() - static_cast<std::size_t>(position_);
                const auto count = std::min(destination.size(), remaining);
                if (count > 0) {
                    std::memcpy(destination.data(), bytes.data() + position_, count);
                    position_ += count;
                }
                return count;
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
                position_ = position;
                return {};
            }

        private:
            ResourceHandle resource_;
            std::uint64_t position_ = 0;
        };
    } // namespace

    auto make_memory_resource_stream(ResourceHandle resource) -> ResourceStreamHandle {
        return std::make_shared<MemoryResourceStream>(std::move(resource));
    }
} // namespace nandina::resource
