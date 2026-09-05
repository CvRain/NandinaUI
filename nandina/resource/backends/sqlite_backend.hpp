#ifndef NANDINA_EXPERIMENT_RESOURCE_BACKENDS_SQLITE_BACKEND_HPP
#define NANDINA_EXPERIMENT_RESOURCE_BACKENDS_SQLITE_BACKEND_HPP

#include "../resource_backend.hpp"
#include <filesystem>
#include <memory>

namespace nandina::resource
{
    struct SQLiteBackendOptions {
        std::string name = "sqlite";
        std::filesystem::path database;
        std::optional<std::filesystem::path> external_root;
        std::uint64_t max_resource_size = 64U * 1024U * 1024U;
        std::uint64_t max_stream_size = 4ULL * 1024ULL * 1024ULL * 1024ULL;
        int busy_timeout_ms = 1000;
    };

    class SQLiteBackend final: public IResourceBackend {
    public:
        static constexpr int application_id = 0x4e414e44;
        static constexpr int schema_version = 1;
        [[nodiscard]] static auto open(SQLiteBackendOptions options)
            -> ResourceResult<std::shared_ptr<SQLiteBackend>>;
        ~SQLiteBackend() override;
        [[nodiscard]] auto name() const noexcept -> std::string_view override;
        [[nodiscard]] auto find(const ResourceKey& key) const -> ResourceLookup override;
        [[nodiscard]] auto find(ResourceId id) const -> ResourceLookup override;
        [[nodiscard]] auto open_stream(const ResourceKey& key) const
            -> ResourceStreamLookup override;
        [[nodiscard]] auto open_stream(ResourceId id) const -> ResourceStreamLookup override;

    private:
        class Impl;
        explicit SQLiteBackend(std::unique_ptr<Impl> impl);
        std::unique_ptr<Impl> impl_;
    };
} // namespace nandina::resource
#endif
