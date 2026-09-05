#include "resource.hpp"

#include <charconv>
#include <format>
#include <random>
#include <stdexcept>

namespace nandina::resource
{
    namespace
    {
        [[nodiscard]] auto hex_value(const char ch) -> int {
            if (ch >= '0' && ch <= '9') {
                return ch - '0';
            }
            if (ch >= 'a' && ch <= 'f') {
                return ch - 'a' + 10;
            }
            return -1;
        }

        [[nodiscard]] auto valid_key(std::string_view value) -> bool {
            if (value.empty() || value.size() > 255 || value.front() == '/' || value.back() == '/')
            {
                return false;
            }
            std::size_t segment_start = 0;
            for (std::size_t index = 0; index <= value.size(); ++index) {
                if (index == value.size() || value[index] == '/') {
                    const auto segment = value.substr(segment_start, index - segment_start);
                    if (segment.empty() || segment == "." || segment == "..") {
                        return false;
                    }
                    segment_start = index + 1;
                    continue;
                }
                const char ch = value[index];
                if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '.' || ch == '_'
                      || ch == '-'))
                {
                    return false;
                }
            }
            return true;
        }
    } // namespace

    auto ResourceId::parse(const std::string_view text) -> std::optional<ResourceId> {
        if (text.size() != 36 || text[8] != '-' || text[13] != '-' || text[18] != '-'
            || text[23] != '-')
        {
            return std::nullopt;
        }
        Bytes bytes {};
        std::size_t output = 0;
        for (std::size_t index = 0; index < text.size();) {
            if (text[index] == '-') {
                ++index;
                continue;
            }
            const int high = hex_value(text[index++]);
            const int low = index < text.size() ? hex_value(text[index++]) : -1;
            if (high < 0 || low < 0 || output >= bytes.size()) {
                return std::nullopt;
            }
            bytes[output++] = static_cast<std::uint8_t>((high << 4) | low);
        }
        return output == bytes.size() ? std::optional(ResourceId(bytes)) : std::nullopt;
    }

    auto ResourceId::random() -> ResourceId {
        Bytes bytes {};
        std::random_device source;
        for (auto& byte: bytes) {
            byte = static_cast<std::uint8_t>(source());
        }
        bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0fU) | 0x40U);
        bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3fU) | 0x80U);
        return ResourceId(bytes);
    }

    auto ResourceId::to_string() const -> std::string {
        constexpr char digits[] = "0123456789abcdef";
        std::string result;
        result.reserve(36);
        for (std::size_t index = 0; index < bytes_.size(); ++index) {
            if (index == 4 || index == 6 || index == 8 || index == 10) {
                result.push_back('-');
            }
            result.push_back(digits[bytes_[index] >> 4]);
            result.push_back(digits[bytes_[index] & 0x0fU]);
        }
        return result;
    }

    ResourceKey::ResourceKey(std::string value): value_(std::move(value)) {
        if (!valid_key(value_)) {
            throw std::invalid_argument("invalid resource key");
        }
    }

    auto ResourceKey::parse(const std::string_view value) -> std::optional<ResourceKey> {
        if (!valid_key(value)) {
            return std::nullopt;
        }
        return ResourceKey(std::string(value));
    }

    Resource::Resource(
        const ResourceId id,
        ResourceKey key,
        std::string media_type,
        const ResourceStorage storage,
        std::vector<std::uint8_t> bytes
    ):
        id_(id),
        key_(std::move(key)),
        media_type_(std::move(media_type)),
        storage_(storage),
        bytes_(std::move(bytes)) {
        if (id_.is_nil()) {
            throw std::invalid_argument("resource id cannot be nil");
        }
    }
} // namespace nandina::resource
