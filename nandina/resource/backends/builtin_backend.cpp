#include "builtin_backend.hpp"

#include <array>

namespace nandina::resource::detail
{
    extern const std::array<std::uint8_t, NANDINA_BUILTIN_DEFAULT_FONT_SIZE>
        builtin_default_font_bytes;
    extern const std::array<std::uint8_t, NANDINA_BUILTIN_DEFAULT_FONT_LICENSE_SIZE>
        builtin_default_font_license_bytes;
} // namespace nandina::resource::detail

namespace nandina::resource
{
    namespace
    {
        [[nodiscard]] auto id(std::string_view value) -> ResourceId {
            return *ResourceId::parse(value);
        }

        template<std::size_t Size>
        [[nodiscard]] auto copy_bytes(const std::array<std::uint8_t, Size>& bytes)
            -> std::vector<std::uint8_t> {
            return {bytes.begin(), bytes.end()};
        }
    } // namespace

    BuiltinBackend::BuiltinBackend():
        default_font_(
            std::make_shared<const Resource>(
                id("737980c0-0000-4000-8000-000000000001"),
                ResourceKey(std::string(builtin_default_font_key)),
                "font/ttf",
                ResourceStorage::embedded_blob,
                copy_bytes(detail::builtin_default_font_bytes)
            )
        ),
        default_font_license_(
            std::make_shared<const Resource>(
                id("737980c0-0000-4000-8000-000000000002"),
                ResourceKey(std::string(builtin_default_font_license_key)),
                "text/plain",
                ResourceStorage::embedded_blob,
                copy_bytes(detail::builtin_default_font_license_bytes)
            )
        ) {}

    auto BuiltinBackend::create() -> std::shared_ptr<BuiltinBackend> {
        static auto backend = std::shared_ptr<BuiltinBackend>(new BuiltinBackend());
        return backend;
    }

    auto BuiltinBackend::name() const noexcept -> std::string_view {
        return "builtin";
    }

    auto BuiltinBackend::find(const ResourceKey& key) const -> ResourceLookup {
        if (key == default_font_->key()) {
            return std::optional<ResourceHandle>(default_font_);
        }
        if (key == default_font_license_->key()) {
            return std::optional<ResourceHandle>(default_font_license_);
        }
        return std::optional<ResourceHandle> {};
    }

    auto BuiltinBackend::find(const ResourceId id) const -> ResourceLookup {
        if (id == default_font_->id()) {
            return std::optional<ResourceHandle>(default_font_);
        }
        if (id == default_font_license_->id()) {
            return std::optional<ResourceHandle>(default_font_license_);
        }
        return std::optional<ResourceHandle> {};
    }
} // namespace nandina::resource
