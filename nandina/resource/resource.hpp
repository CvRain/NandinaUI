//
// resource/resource — stable identities and immutable loaded resources.
//

#ifndef NANDINA_EXPERIMENT_RESOURCE_RESOURCE_HPP
#define NANDINA_EXPERIMENT_RESOURCE_RESOURCE_HPP

#include <array>
#include <compare>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::resource
{
    class ResourceId {
    public:
        using Bytes = std::array<std::uint8_t, 16>;
        constexpr ResourceId() = default;
        explicit constexpr ResourceId(Bytes bytes): bytes_(bytes) {}

        [[nodiscard]] static auto parse(std::string_view text) -> std::optional<ResourceId>;
        [[nodiscard]] static auto random() -> ResourceId;
        [[nodiscard]] auto to_string() const -> std::string;
        [[nodiscard]] constexpr auto bytes() const -> const Bytes& {
            return bytes_;
        }
        [[nodiscard]] constexpr auto is_nil() const -> bool {
            for (const auto byte: bytes_) {
                if (byte != 0) {
                    return false;
                }
            }
            return true;
        }
        auto operator<=>(const ResourceId&) const = default;

    private:
        Bytes bytes_ {};
    };

    class ResourceKey {
    public:
        explicit ResourceKey(std::string value);
        [[nodiscard]] static auto parse(std::string_view value) -> std::optional<ResourceKey>;
        [[nodiscard]] auto value() const noexcept -> std::string_view {
            return value_;
        }
        auto operator<=>(const ResourceKey&) const = default;

    private:
        std::string value_;
    };

    enum class ResourceStorage { memory, embedded_blob, external_file };

    class Resource final {
    public:
        Resource(
            ResourceId id,
            ResourceKey key,
            std::string media_type,
            ResourceStorage storage,
            std::vector<std::uint8_t> bytes
        );

        [[nodiscard]] auto id() const noexcept -> ResourceId {
            return id_;
        }
        [[nodiscard]] auto key() const noexcept -> const ResourceKey& {
            return key_;
        }
        [[nodiscard]] auto media_type() const noexcept -> std::string_view {
            return media_type_;
        }
        [[nodiscard]] auto storage() const noexcept -> ResourceStorage {
            return storage_;
        }
        [[nodiscard]] auto bytes() const noexcept -> std::span<const std::uint8_t> {
            return bytes_;
        }
        [[nodiscard]] auto size() const noexcept -> std::size_t {
            return bytes_.size();
        }

    private:
        ResourceId id_;
        ResourceKey key_;
        std::string media_type_;
        ResourceStorage storage_;
        std::vector<std::uint8_t> bytes_;
    };

    using ResourceHandle = std::shared_ptr<const Resource>;
} // namespace nandina::resource

#endif
