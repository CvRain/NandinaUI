#include "resource/resource_manifest.hpp"

#include "resource/backends/sqlite_backend.hpp"

#include <openssl/evp.h>
#include <sqlite3.h>

#include <array>
#include <fstream>
#include <map>
#include <memory>

namespace nandina::resource
{
    namespace
    {
        struct DatabaseCloser {
            void operator()(sqlite3* database) const { if (database) { sqlite3_close_v2(database); } }
        };
        using Database = std::unique_ptr<sqlite3, DatabaseCloser>;

        struct Statement {
            sqlite3_stmt* value = nullptr;
            ~Statement() { if (value) { sqlite3_finalize(value); } }
        };

        [[nodiscard]] auto sql_error(sqlite3* database, std::string_view operation) -> std::string {
            return std::string(operation) + ": " + sqlite3_errmsg(database);
        }

        [[nodiscard]] auto exec(sqlite3* database, const char* sql) -> std::expected<void, std::string> {
            char* message = nullptr;
            const auto result = sqlite3_exec(database, sql, nullptr, nullptr, &message);
            std::string detail = message ? message : "SQLite operation failed";
            sqlite3_free(message);
            if (result != SQLITE_OK) { return std::unexpected(std::move(detail)); }
            return {};
        }

        [[nodiscard]] auto read_bytes(const std::filesystem::path& path, std::uint64_t expected_size)
            -> std::expected<std::vector<std::uint8_t>, std::string> {
            std::ifstream input(path, std::ios::binary);
            if (!input) { return std::unexpected("cannot open resource: " + path.string()); }
            std::vector<std::uint8_t> bytes(static_cast<std::size_t>(expected_size));
            if (expected_size > 0 && !input.read(
                    reinterpret_cast<char*>(bytes.data()),
                    static_cast<std::streamsize>(expected_size)))
            {
                return std::unexpected("cannot read resource: " + path.string());
            }
            if (input.peek() != std::ifstream::traits_type::eof()) {
                return std::unexpected("resource size changed after lock generation: " + path.string());
            }
            return bytes;
        }

        [[nodiscard]] auto selected_storage(
            const ResourcePolicy& policy,
            const LockedResource& item
        ) -> ManifestStorage {
            if (item.storage != ManifestStorage::automatic) { return item.storage; }
            if (item.streaming || item.size > policy.embed_threshold
                || item.media_type.starts_with("video/")
                || item.media_type.starts_with("audio/"))
            {
                return ManifestStorage::external;
            }
            return ManifestStorage::embedded;
        }

        [[nodiscard]] auto package_fingerprint(
            const ResourcePolicy& policy,
            const ResourceLockManifest& manifest
        ) -> std::expected<std::string, std::string> {
            EVP_MD_CTX* raw = EVP_MD_CTX_new();
            if (!raw) { return std::unexpected("cannot allocate package fingerprint context"); }
            const auto context = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>(raw, EVP_MD_CTX_free);
            if (EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1) {
                return std::unexpected("cannot initialize package fingerprint");
            }
            const auto update = [&](std::string_view value) {
                return EVP_DigestUpdate(context.get(), value.data(), value.size()) == 1;
            };
            if (!update(manifest.package_id)) { return std::unexpected("cannot hash package manifest"); }
            if (!update(std::to_string(policy.embed_threshold))) {
                return std::unexpected("cannot hash package policy");
            }
            for (const auto& item: manifest.resources) {
                const auto storage = selected_storage(policy, item) == ManifestStorage::embedded ? "e" : "x";
                if (!update(item.id.to_string()) || !update(item.key.value()) || !update(item.sha256)
                    || !update(storage) || !update(item.streaming ? "s" : "n"))
                {
                    return std::unexpected("cannot hash package manifest");
                }
            }
            for (const auto& [alias, target]: policy.aliases) {
                if (!update(alias.value()) || !update(target.value())) {
                    return std::unexpected("cannot hash package aliases");
                }
            }
            std::array<unsigned char, EVP_MAX_MD_SIZE> digest {};
            unsigned int size = 0;
            if (EVP_DigestFinal_ex(context.get(), digest.data(), &size) != 1) {
                return std::unexpected("cannot finalize package fingerprint");
            }
            constexpr char hex[] = "0123456789abcdef";
            std::string output(size * 2, '0');
            for (unsigned int index = 0; index < size; ++index) {
                output[index * 2] = hex[digest[index] >> 4U];
                output[index * 2 + 1] = hex[digest[index] & 0x0fU];
            }
            return output;
        }

        [[nodiscard]] auto existing_fingerprint(const std::filesystem::path& path)
            -> std::optional<std::string> {
            std::ifstream input(path, std::ios::binary);
            std::string value;
            return input && std::getline(input, value) ? std::optional<std::string>(value) : std::nullopt;
        }

        [[nodiscard]] auto bind_resource(
            sqlite3* database,
            sqlite3_stmt* statement,
            const LockedResource& item,
            ManifestStorage storage,
            const std::optional<std::vector<std::uint8_t>>& bytes,
            const std::optional<std::filesystem::path>& external_path
        ) -> std::expected<void, std::string> {
            sqlite3_reset(statement);
            sqlite3_clear_bindings(statement);
            const auto key = item.key.value();
            if (sqlite3_bind_blob(statement, 1, item.id.bytes().data(), item.id.bytes().size(), SQLITE_TRANSIENT) != SQLITE_OK
                || sqlite3_bind_text(statement, 2, key.data(), key.size(), SQLITE_TRANSIENT) != SQLITE_OK
                || sqlite3_bind_text(statement, 3, item.media_type.c_str(), item.media_type.size(), SQLITE_TRANSIENT) != SQLITE_OK
                || sqlite3_bind_int(statement, 4, storage == ManifestStorage::embedded ? 0 : 1) != SQLITE_OK
                || (bytes ? sqlite3_bind_blob(statement, 5, bytes->data(), bytes->size(), SQLITE_TRANSIENT)
                          : sqlite3_bind_null(statement, 5)) != SQLITE_OK
                || (external_path
                        ? sqlite3_bind_text(statement, 6, external_path->generic_string().c_str(), -1, SQLITE_TRANSIENT)
                        : sqlite3_bind_null(statement, 6)) != SQLITE_OK
                || sqlite3_bind_int64(statement, 7, static_cast<sqlite3_int64>(item.size)) != SQLITE_OK)
            {
                return std::unexpected(sql_error(database, "bind resource"));
            }
            if (sqlite3_step(statement) != SQLITE_DONE) {
                return std::unexpected(sql_error(database, "insert resource"));
            }
            return {};
        }
    }

    auto pack_resource_package(
        const ResourcePolicy& policy,
        const ResourceLockManifest& manifest
    ) -> std::expected<ResourcePackageResult, std::string> {
        if (policy.package_id != manifest.package_id) {
            return std::unexpected("package_id does not match lock manifest");
        }
        const auto output = policy.base_directory / policy.package_directory;
        const auto database_path = output / "resources.db";
        const auto external_root = output / "external";
        const auto fingerprint_path = output / ".nanres-pack.sha256";
        auto fingerprint = package_fingerprint(policy, manifest);
        if (!fingerprint) { return std::unexpected(fingerprint.error()); }

        bool outputs_complete = std::filesystem::is_regular_file(database_path);
        for (const auto& item: manifest.resources) {
            if (selected_storage(policy, item) == ManifestStorage::external
                && !std::filesystem::is_regular_file(external_root / item.id.to_string()))
            {
                outputs_complete = false;
                break;
            }
        }
        if (outputs_complete && existing_fingerprint(fingerprint_path) == fingerprint) {
            return ResourcePackageResult {database_path, external_root, 0, 0, true};
        }

        const auto temporary = output.string() + ".tmp";
        std::error_code filesystem_error;
        std::filesystem::remove_all(temporary, filesystem_error);
        if (!std::filesystem::create_directories(std::filesystem::path(temporary) / "external", filesystem_error)
            && filesystem_error)
        {
            return std::unexpected("cannot create package temporary directory: " + filesystem_error.message());
        }
        const auto temporary_database = std::filesystem::path(temporary) / "resources.db";
        sqlite3* raw_database = nullptr;
        if (sqlite3_open_v2(temporary_database.string().c_str(), &raw_database,
                SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK)
        {
            Database failed(raw_database);
            return std::unexpected("cannot create resource database");
        }
        Database database(raw_database);
        auto schema = exec(database.get(),
            "PRAGMA application_id=1312902724; PRAGMA user_version=1; PRAGMA journal_mode=DELETE;"
            "PRAGMA synchronous=FULL; BEGIN IMMEDIATE;"
            "CREATE TABLE resources(id BLOB PRIMARY KEY, resource_key TEXT NOT NULL UNIQUE,"
            "media_type TEXT NOT NULL, storage INTEGER NOT NULL, data BLOB, external_path TEXT,"
            "size INTEGER NOT NULL);"
            "CREATE TABLE aliases(alias TEXT PRIMARY KEY, resource_id BLOB NOT NULL);"
        );
        if (!schema) { return std::unexpected(schema.error()); }
        Statement insert_resource;
        if (sqlite3_prepare_v2(database.get(),
                "INSERT INTO resources VALUES(?1,?2,?3,?4,?5,?6,?7)", -1,
                &insert_resource.value, nullptr) != SQLITE_OK)
        {
            return std::unexpected(sql_error(database.get(), "prepare resource insert"));
        }
        ResourcePackageResult result {database_path, external_root};
        for (const auto& item: manifest.resources) {
            const auto source = policy.base_directory / item.source;
            auto actual_hash = sha256_file(source);
            if (!actual_hash || *actual_hash != item.sha256) {
                return std::unexpected("resource changed after lock generation: " + item.source.string());
            }
            const auto storage = selected_storage(policy, item);
            if (item.streaming && storage == ManifestStorage::embedded) {
                return std::unexpected(
                    "streaming resource cannot use embedded storage: " + std::string(item.key.value())
                );
            }
            if (storage == ManifestStorage::embedded) {
                auto bytes = read_bytes(source, item.size);
                if (!bytes) { return std::unexpected(bytes.error()); }
                auto inserted = bind_resource(database.get(), insert_resource.value, item, storage, *bytes, std::nullopt);
                if (!inserted) { return std::unexpected(inserted.error()); }
                ++result.embedded_count;
            }
            else {
                const auto relative = std::filesystem::path("external") / item.id.to_string();
                const auto destination = std::filesystem::path(temporary) / relative;
                std::filesystem::copy_file(source, destination, std::filesystem::copy_options::overwrite_existing, filesystem_error);
                if (filesystem_error) { return std::unexpected("cannot copy external resource: " + filesystem_error.message()); }
                auto inserted = bind_resource(database.get(), insert_resource.value, item, storage, std::nullopt, relative);
                if (!inserted) { return std::unexpected(inserted.error()); }
                ++result.external_count;
            }
        }
        std::map<ResourceKey, ResourceId> ids;
        for (const auto& item: manifest.resources) { ids.emplace(item.key, item.id); }
        Statement insert_alias;
        if (sqlite3_prepare_v2(database.get(), "INSERT INTO aliases VALUES(?1,?2)", -1,
                &insert_alias.value, nullptr) != SQLITE_OK)
        {
            return std::unexpected(sql_error(database.get(), "prepare alias insert"));
        }
        for (const auto& [alias, target]: policy.aliases) {
            const auto target_id = ids.find(target);
            if (target_id == ids.end()) { return std::unexpected("alias target is not in lock manifest"); }
            const auto alias_text = alias.value();
            sqlite3_reset(insert_alias.value);
            sqlite3_clear_bindings(insert_alias.value);
            if (sqlite3_bind_text(insert_alias.value, 1, alias_text.data(), alias_text.size(), SQLITE_TRANSIENT) != SQLITE_OK
                || sqlite3_bind_blob(insert_alias.value, 2, target_id->second.bytes().data(),
                    target_id->second.bytes().size(), SQLITE_TRANSIENT) != SQLITE_OK
                || sqlite3_step(insert_alias.value) != SQLITE_DONE)
            {
                return std::unexpected(sql_error(database.get(), "insert alias"));
            }
        }
        auto committed = exec(database.get(), "COMMIT; VACUUM;");
        if (!committed) { return std::unexpected(committed.error()); }
        database.reset();
        {
            std::ofstream output_fingerprint(std::filesystem::path(temporary) / ".nanres-pack.sha256");
            output_fingerprint << *fingerprint << '\n';
            if (!output_fingerprint) { return std::unexpected("cannot write package fingerprint"); }
        }
        const auto backup = output.string() + ".old";
        std::filesystem::remove_all(backup, filesystem_error);
        if (std::filesystem::exists(output)) {
            std::filesystem::rename(output, backup, filesystem_error);
            if (filesystem_error) { return std::unexpected("cannot stage previous package: " + filesystem_error.message()); }
        }
        std::filesystem::rename(temporary, output, filesystem_error);
        if (filesystem_error) {
            if (std::filesystem::exists(backup)) {
                std::error_code restore_error;
                std::filesystem::rename(backup, output, restore_error);
            }
            return std::unexpected("cannot install resource package: " + filesystem_error.message());
        }
        std::filesystem::remove_all(backup, filesystem_error);
        return result;
    }
} // namespace nandina::resource
