#ifndef NANDINA_EXPERIMENT_RESOURCE_BACKENDS_BUILTIN_BACKEND_HPP
#define NANDINA_EXPERIMENT_RESOURCE_BACKENDS_BUILTIN_BACKEND_HPP

#include "../resource_backend.hpp"

#include <memory>

namespace nandina::resource
{
    inline constexpr std::string_view builtin_default_font_key = "fonts/default";
    inline constexpr std::string_view builtin_default_font_license_key =
        "licenses/fonts/caskaydia-cove";
    class BuiltinBackend final: public IResourceBackend {
    public:
        [[nodiscard]] static auto create() -> std::shared_ptr<BuiltinBackend>;
        [[nodiscard]] auto name() const noexcept -> std::string_view override;
        [[nodiscard]] auto find(const ResourceKey& key) const -> ResourceLookup override;
        [[nodiscard]] auto find(ResourceId id) const -> ResourceLookup override;

    private:
        BuiltinBackend();
        ResourceHandle default_font_;
        ResourceHandle default_font_license_;
    };
} // namespace nandina::resource

#endif
