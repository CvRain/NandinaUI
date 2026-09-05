#include "sqlite_backend.hpp"
#include "../file_resource_stream.hpp"
#include <sqlite3.h>

#include <cstring>
#include <fstream>
#include <mutex>

namespace nandina::resource
{
    namespace
    {
        struct Statement {
            sqlite3_stmt* value = nullptr;
            ~Statement() {
                if (value) {
                    sqlite3_finalize(value);
                }
            }
        };
        [[nodiscard]] auto error(std::string operation, std::string message) -> ResourceError {
            return {ResourceErrorCode::malformed_backend, std::move(operation), std::move(message)};
        }
        [[nodiscard]] auto pragma_int(sqlite3* db, const char* sql) -> std::optional<int> {
            Statement statement;
            if (sqlite3_prepare_v2(db, sql, -1, &statement.value, nullptr) != SQLITE_OK
                || sqlite3_step(statement.value) != SQLITE_ROW)
            {
                return std::nullopt;
            }
            return sqlite3_column_int(statement.value, 0);
        }
    } // namespace

    class SQLiteBackend::Impl {
    public:
        SQLiteBackendOptions options;
        sqlite3* db = nullptr;
        mutable std::mutex mutex;
        ~Impl() {
            if (db) {
                sqlite3_close_v2(db);
            }
        }

        [[nodiscard]] auto materialize(sqlite3_stmt* row) const -> ResourceLookup {
            const auto* raw_id = static_cast<const std::uint8_t*>(sqlite3_column_blob(row, 0));
            if (!raw_id || sqlite3_column_bytes(row, 0) != 16) {
                return std::unexpected(error("sqlite.lookup", "invalid resource id"));
            }
            ResourceId::Bytes id {};
            std::memcpy(id.data(), raw_id, id.size());
            const auto* raw_key = reinterpret_cast<const char*>(sqlite3_column_text(row, 1));
            const auto parsed_key = raw_key ? ResourceKey::parse(raw_key) : std::nullopt;
            if (!parsed_key) {
                return std::unexpected(error("sqlite.lookup", "invalid resource key"));
            }
            const auto* media = reinterpret_cast<const char*>(sqlite3_column_text(row, 2));
            const int storage = sqlite3_column_int(row, 3);
            const auto size = sqlite3_column_int64(row, 6);
            if (size < 0 || static_cast<std::uint64_t>(size) > options.max_resource_size) {
                return std::unexpected(
                    ResourceError {
                        ResourceErrorCode::size_limit_exceeded,
                        "sqlite.lookup",
                        "resource exceeds configured size limit"
                    }
                );
            }
            std::vector<std::uint8_t> bytes;
            ResourceStorage kind;
            if (storage == 0) {
                const auto* blob = static_cast<const std::uint8_t*>(sqlite3_column_blob(row, 4));
                const int count = sqlite3_column_bytes(row, 4);
                if (count != size || (count > 0 && !blob)) {
                    return std::unexpected(
                        ResourceError {
                            ResourceErrorCode::integrity_error,
                            "sqlite.lookup",
                            "embedded resource size mismatch"
                        }
                    );
                }
                if (count > 0) {
                    bytes.assign(blob, blob + count);
                }
                kind = ResourceStorage::embedded_blob;
            }
            else if (storage == 1) {
                const auto* raw_path = reinterpret_cast<const char*>(sqlite3_column_text(row, 5));
                const std::filesystem::path relative = raw_path ? raw_path : "";
                if (relative.empty() || relative.is_absolute()) {
                    return std::unexpected(
                        error("sqlite.lookup", "invalid external resource path")
                    );
                }
                for (const auto& part: relative) {
                    if (part == "." || part == "..") {
                        return std::unexpected(
                            error("sqlite.lookup", "invalid external resource path")
                        );
                    }
                }
                const auto root = options.external_root.value_or(options.database.parent_path());
                const auto path = root / relative;
                std::error_code file_error;
                if (std::filesystem::file_size(path, file_error) != static_cast<std::uint64_t>(size)
                    || file_error)
                {
                    return std::unexpected(
                        ResourceError {
                            ResourceErrorCode::integrity_error,
                            "sqlite.lookup",
                            "external resource size mismatch"
                        }
                    );
                }
                std::ifstream stream(path, std::ios::binary);
                if (!stream) {
                    return std::unexpected(
                        ResourceError {
                            ResourceErrorCode::io_error,
                            "sqlite.lookup",
                            "external resource cannot be opened"
                        }
                    );
                }
                bytes.resize(static_cast<std::size_t>(size));
                if (size > 0
                    && !stream.read(
                        reinterpret_cast<char*>(bytes.data()),
                        static_cast<std::streamsize>(size)
                    ))
                {
                    return std::unexpected(
                        ResourceError {
                            ResourceErrorCode::io_error,
                            "sqlite.lookup",
                            "external resource read failed"
                        }
                    );
                }
                kind = ResourceStorage::external_file;
            }
            else {
                return std::unexpected(error("sqlite.lookup", "unknown resource storage"));
            }
            return std::optional<ResourceHandle> {std::make_shared<const Resource>(
                ResourceId(id),
                *parsed_key,
                media ? media : "",
                kind,
                std::move(bytes)
            )};
        }

        [[nodiscard]] auto query(const char* sql, const void* value, int size, bool text) const
            -> ResourceLookup {
            std::lock_guard lock(mutex);
            Statement statement;
            if (sqlite3_prepare_v2(db, sql, -1, &statement.value, nullptr) != SQLITE_OK) {
                return std::unexpected(
                    error("sqlite.prepare", "resource query preparation failed")
                );
            }
            const int rc = text
                ? sqlite3_bind_text(
                      statement.value,
                      1,
                      static_cast<const char*>(value),
                      size,
                      SQLITE_TRANSIENT
                  )
                : sqlite3_bind_blob(statement.value, 1, value, size, SQLITE_TRANSIENT);
            if (rc != SQLITE_OK) {
                return std::unexpected(error("sqlite.bind", "resource query binding failed"));
            }
            const int step = sqlite3_step(statement.value);
            if (step == SQLITE_DONE) {
                return std::optional<ResourceHandle> {};
            }
            if (step != SQLITE_ROW) {
                return std::unexpected(error("sqlite.step", "resource query failed"));
            }
            return materialize(statement.value);
        }

        [[nodiscard]] auto stream_from_row(sqlite3_stmt* row) const -> ResourceStreamLookup {
            const auto* raw_id = static_cast<const std::uint8_t*>(sqlite3_column_blob(row, 0));
            if (!raw_id || sqlite3_column_bytes(row, 0) != 16) {
                return std::unexpected(error("sqlite.stream", "invalid resource id"));
            }
            ResourceId::Bytes id {};
            std::memcpy(id.data(), raw_id, id.size());
            const auto* raw_key = reinterpret_cast<const char*>(sqlite3_column_text(row, 1));
            const auto key = raw_key ? ResourceKey::parse(raw_key) : std::nullopt;
            if (!key) {
                return std::unexpected(error("sqlite.stream", "invalid resource key"));
            }
            const auto* raw_media = reinterpret_cast<const char*>(sqlite3_column_text(row, 2));
            const std::string media = raw_media ? raw_media : "";
            const int storage = sqlite3_column_int(row, 3);
            const auto raw_size = sqlite3_column_int64(row, 6);
            if (raw_size < 0 || static_cast<std::uint64_t>(raw_size) > options.max_stream_size) {
                return std::unexpected(
                    ResourceError {
                        ResourceErrorCode::size_limit_exceeded,
                        "sqlite.stream",
                        "resource exceeds configured stream size limit",
                    }
                );
            }
            const auto size = static_cast<std::uint64_t>(raw_size);
            if (storage == 0) {
                const auto* blob = static_cast<const std::uint8_t*>(sqlite3_column_blob(row, 4));
                const int count = sqlite3_column_bytes(row, 4);
                if (count < 0 || static_cast<std::uint64_t>(count) != size || (count > 0 && !blob))
                {
                    return std::unexpected(
                        ResourceError {
                            ResourceErrorCode::integrity_error,
                            "sqlite.stream",
                            "embedded resource size mismatch",
                        }
                    );
                }
                std::vector<std::uint8_t> bytes;
                if (count > 0) {
                    bytes.assign(blob, blob + count);
                }
                auto resource = std::make_shared<const Resource>(
                    ResourceId(id),
                    *key,
                    media,
                    ResourceStorage::embedded_blob,
                    std::move(bytes)
                );
                return std::optional<ResourceStreamHandle> {
                    make_memory_resource_stream(std::move(resource))
                };
            }
            if (storage != 1) {
                return std::unexpected(error("sqlite.stream", "unknown resource storage"));
            }
            const auto* raw_path = reinterpret_cast<const char*>(sqlite3_column_text(row, 5));
            const std::filesystem::path relative = raw_path ? raw_path : "";
            if (relative.empty() || relative.is_absolute()) {
                return std::unexpected(error("sqlite.stream", "invalid external resource path"));
            }
            for (const auto& part: relative) {
                if (part == "." || part == "..") {
                    return std::unexpected(
                        error("sqlite.stream", "invalid external resource path")
                    );
                }
            }
            const auto root = options.external_root.value_or(options.database.parent_path());
            auto opened = open_file_resource_stream({
                .id = ResourceId(id),
                .key = *key,
                .media_type = media,
                .path = root / relative,
                .declared_size = size,
            });
            if (!opened) {
                return std::unexpected(opened.error());
            }
            return std::optional<ResourceStreamHandle> {*opened};
        }

        [[nodiscard]] auto
        stream_query(const char* sql, const void* value, int size, bool text) const
            -> ResourceStreamLookup {
            std::lock_guard lock(mutex);
            Statement statement;
            if (sqlite3_prepare_v2(db, sql, -1, &statement.value, nullptr) != SQLITE_OK) {
                return std::unexpected(error("sqlite.prepare", "stream query preparation failed"));
            }
            const int rc = text
                ? sqlite3_bind_text(
                      statement.value,
                      1,
                      static_cast<const char*>(value),
                      size,
                      SQLITE_TRANSIENT
                  )
                : sqlite3_bind_blob(statement.value, 1, value, size, SQLITE_TRANSIENT);
            if (rc != SQLITE_OK) {
                return std::unexpected(error("sqlite.bind", "stream query binding failed"));
            }
            const int step = sqlite3_step(statement.value);
            if (step == SQLITE_DONE) {
                return std::optional<ResourceStreamHandle> {};
            }
            if (step != SQLITE_ROW) {
                return std::unexpected(error("sqlite.step", "stream query failed"));
            }
            return stream_from_row(statement.value);
        }
    };

    SQLiteBackend::SQLiteBackend(std::unique_ptr<Impl> impl): impl_(std::move(impl)) {}
    SQLiteBackend::~SQLiteBackend() = default;
    auto SQLiteBackend::open(SQLiteBackendOptions options)
        -> ResourceResult<std::shared_ptr<SQLiteBackend>> {
        if (options.database.empty() || options.max_resource_size == 0
            || options.max_stream_size == 0 || options.busy_timeout_ms < 0)
        {
            return std::unexpected(
                ResourceError {
                    ResourceErrorCode::invalid_configuration,
                    "sqlite.open",
                    "invalid SQLite backend options"
                }
            );
        }
        auto impl = std::make_unique<Impl>();
        impl->options = std::move(options);
        if (sqlite3_open_v2(
                impl->options.database.string().c_str(),
                &impl->db,
                SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX,
                nullptr
            )
            != SQLITE_OK)
        {
            return std::unexpected(
                ResourceError {
                    ResourceErrorCode::backend_unavailable,
                    "sqlite.open",
                    "resource database cannot be opened"
                }
            );
        }
        sqlite3_busy_timeout(impl->db, impl->options.busy_timeout_ms);
        sqlite3_db_config(impl->db, SQLITE_DBCONFIG_DEFENSIVE, 1, nullptr);
        sqlite3_exec(
            impl->db,
            "PRAGMA query_only=ON; PRAGMA trusted_schema=OFF; PRAGMA foreign_keys=ON;",
            nullptr,
            nullptr,
            nullptr
        );
        const auto app = pragma_int(impl->db, "PRAGMA application_id");
        const auto version = pragma_int(impl->db, "PRAGMA user_version");
        if (!app || *app != application_id) {
            return std::unexpected(
                ResourceError {
                    ResourceErrorCode::malformed_backend,
                    "sqlite.open",
                    "database application id does not match Nandina"
                }
            );
        }
        if (!version || *version != schema_version) {
            return std::unexpected(
                ResourceError {
                    ResourceErrorCode::unsupported_schema,
                    "sqlite.open",
                    "unsupported resource database schema"
                }
            );
        }
        return std::shared_ptr<SQLiteBackend>(new SQLiteBackend(std::move(impl)));
    }
    auto SQLiteBackend::name() const noexcept -> std::string_view {
        return impl_->options.name;
    }
    auto SQLiteBackend::find(const ResourceKey& key) const -> ResourceLookup {
        constexpr auto sql =
            "SELECT r.id,r.resource_key,r.media_type,r.storage,r.data,r.external_path,r.size "
            "FROM resources r "
            "WHERE r.resource_key=?1 OR r.id=(SELECT resource_id FROM aliases WHERE alias=?1) "
            "ORDER BY (r.resource_key=?1) DESC LIMIT 1";

        const auto value = key.value();
        return impl_->query(sql, value.data(), static_cast<int>(value.size()), true);
    }
    auto SQLiteBackend::find(const ResourceId id) const -> ResourceLookup {
        constexpr auto sql =
            "SELECT id,resource_key,media_type,storage,data,external_path,size "
            "FROM resources "
            "WHERE id=?1";
        return impl_->query(sql, id.bytes().data(), static_cast<int>(id.bytes().size()), false);
    }
    auto SQLiteBackend::open_stream(const ResourceKey& key) const -> ResourceStreamLookup {
        constexpr auto sql =
            "SELECT r.id,r.resource_key,r.media_type,r.storage,r.data,r.external_path,r.size "
            "FROM resources r "
            "WHERE r.resource_key=?1 OR r.id=(SELECT resource_id FROM aliases WHERE alias=?1) "
            "ORDER BY (r.resource_key=?1) DESC LIMIT 1";
        const auto value = key.value();
        return impl_->stream_query(sql, value.data(), static_cast<int>(value.size()), true);
    }
    auto SQLiteBackend::open_stream(const ResourceId id) const -> ResourceStreamLookup {
        constexpr auto sql =
            "SELECT id,resource_key,media_type,storage,data,external_path,size "
            "FROM resources WHERE id=?1";
        return impl_
            ->stream_query(sql, id.bytes().data(), static_cast<int>(id.bytes().size()), false);
    }
} // namespace nandina::resource
