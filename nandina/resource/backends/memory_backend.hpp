#ifndef NANDINA_EXPERIMENT_RESOURCE_BACKENDS_MEMORY_BACKEND_HPP
#define NANDINA_EXPERIMENT_RESOURCE_BACKENDS_MEMORY_BACKEND_HPP

#include "../resource_backend.hpp"

#include <memory>

namespace nandina::resource
{
    enum class InsertMode { fail_if_present, replace };

    class MemoryBackend final: public IResourceBackend {
    public:
        explicit MemoryBackend(std::string name = "memory");
        ~MemoryBackend() override;
        [[nodiscard]] auto insert(
            ResourceId id,
            ResourceKey key,
            std::string media_type,
            std::vector<std::uint8_t> bytes,
            InsertMode mode = InsertMode::fail_if_present
        ) -> ResourceResult<ResourceHandle>;
        [[nodiscard]] auto add_alias(ResourceKey alias, ResourceId target) -> ResourceResult<void>;
        [[nodiscard]] auto erase(ResourceId id) -> bool;
        [[nodiscard]] auto name() const noexcept -> std::string_view override;
        [[nodiscard]] auto find(const ResourceKey& key) const -> ResourceLookup override;
        [[nodiscard]] auto find(ResourceId id) const -> ResourceLookup override;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
} // namespace nandina::resource
#endif
