#include <catch2/catch_test_macros.hpp>

#include "resource/backends/builtin_backend.hpp"
#include "resource/backends/directory_backend.hpp"
#include "resource/backends/memory_backend.hpp"
#include "resource/backends/sqlite_backend.hpp"
#include "resource/platform_resource_locator.hpp"
#include "resource/resource_manager.hpp"
#include "resource/resource_scanner.hpp"
#include "resource/resource_uri.hpp"

#include <sqlite3.h>

#include <algorithm>
#include <filesystem>
#include <fstream>

using namespace nandina;

namespace
{
    [[nodiscard]] auto id(std::string_view value) -> resource::ResourceId {
        return *resource::ResourceId::parse(value);
    }

    struct TempDirectory {
        std::filesystem::path path = std::filesystem::temp_directory_path()
            / ("nandina-resource-" + resource::ResourceId::random().to_string());
        TempDirectory() {
            std::filesystem::create_directories(path);
        }
        ~TempDirectory() {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    };

    void exec(sqlite3* db, const char* sql) {
        char* message = nullptr;
        const int result = sqlite3_exec(db, sql, nullptr, nullptr, &message);
        if (message) {
            sqlite3_free(message);
        }
        REQUIRE(result == SQLITE_OK);
    }

    void create_database(const std::filesystem::path& path) {
        sqlite3* db = nullptr;
        REQUIRE(sqlite3_open(path.string().c_str(), &db) == SQLITE_OK);
        exec(db, "PRAGMA application_id=1312902724; PRAGMA user_version=1;");
        exec(
            db,
            "CREATE TABLE resources("
            "id BLOB PRIMARY KEY, resource_key TEXT NOT NULL UNIQUE, media_type TEXT NOT NULL,"
            "storage INTEGER NOT NULL, data BLOB, external_path TEXT, size INTEGER NOT NULL);"
            "CREATE TABLE aliases(alias TEXT PRIMARY KEY, resource_id BLOB NOT NULL);"
        );
        exec(
            db,
            "INSERT INTO resources VALUES("
            "X'00112233445546778899aabbccddeeff','fonts/default','font/ttf',0,X'010203',NULL,3);"
            "INSERT INTO aliases VALUES('fonts/ui',X'00112233445546778899aabbccddeeff');"
            "INSERT INTO resources VALUES("
            "X'10112233445546778899aabbccddeeff','shaders/basic','text/plain',1,NULL,'basic.glsl',4);"
        );
        REQUIRE(sqlite3_close(db) == SQLITE_OK);
    }
} // namespace

TEST_CASE("resource identity and keys are canonical", "[resource]") {
    const auto value = id("00112233-4455-4677-8899-aabbccddeeff");
    REQUIRE(value.to_string() == "00112233-4455-4677-8899-aabbccddeeff");
    REQUIRE_FALSE(value.is_nil());
    REQUIRE_FALSE(resource::ResourceId::parse("00112233-4455-4677-8899-AABBCCDDEEFF"));
    REQUIRE(resource::ResourceKey::parse("fonts/ui/default").has_value());
    REQUIRE_FALSE(resource::ResourceKey::parse("fonts/../secret"));
    REQUIRE_FALSE(resource::ResourceKey::parse("Fonts/Default"));
    const auto random = resource::ResourceId::random();
    REQUIRE((random.bytes()[6] & 0xf0U) == 0x40U);
    REQUIRE((random.bytes()[8] & 0xc0U) == 0x80U);
}

TEST_CASE("resource URIs preserve schemes and canonical logical keys", "[resource][uri]") {
    const auto app = resource::ResourceUri::parse("res://fonts/ui/regular");
    REQUIRE(app.has_value());
    REQUIRE(app->scheme() == resource::ResourceUriScheme::res);
    REQUIRE(app->resource_key()->value() == "fonts/ui/regular");
    REQUIRE(app->to_string() == "res://fonts/ui/regular");

    const auto builtin = resource::ResourceUri::parse("builtin://fonts/default");
    REQUIRE(builtin.has_value());
    REQUIRE(builtin->scheme() == resource::ResourceUriScheme::builtin);

    const auto user = resource::ResourceUri::parse("user://settings/editor.json");
    const auto cache = resource::ResourceUri::parse("cache://thumbnails/item-1");
    REQUIRE(user.has_value());
    REQUIRE(cache.has_value());

    const auto file = resource::ResourceUri::parse("file:///opt/demo/resources.db");
    REQUIRE(file.has_value());
    REQUIRE(file->file_path() == std::filesystem::path("/opt/demo/resources.db"));
    REQUIRE_FALSE(file->resource_key().has_value());

    REQUIRE_FALSE(resource::ResourceUri::parse("fonts/default"));
    REQUIRE_FALSE(resource::ResourceUri::parse("http://example.com/font.ttf"));
    REQUIRE_FALSE(resource::ResourceUri::parse("res://fonts/../secret"));
    REQUIRE_FALSE(resource::ResourceUri::parse("res:///fonts/default"));
    REQUIRE_FALSE(resource::ResourceUri::parse("file://relative/path"));
}

TEST_CASE("Linux locator follows executable and XDG resource order", "[resource][locator]") {
    auto locator = resource::PlatformResourceLocator::create({
        .application_id = "org.nandina.todo",
        .executable_path = "/opt/nandina/bin/todo",
        .environment = {
            {"HOME", "/home/tester"},
            {"XDG_CONFIG_HOME", "/config/user"},
            {"XDG_DATA_HOME", "/data/user"},
            {"XDG_DATA_DIRS", "/data/company:/usr/share"},
            {"XDG_STATE_HOME", "/state/user"},
            {"XDG_CACHE_HOME", "/cache/user"},
        },
    });
    REQUIRE(locator.has_value());
    REQUIRE(locator->config_root() == "/config/user/org.nandina.todo");
    REQUIRE(locator->data_root() == "/data/user/org.nandina.todo");
    REQUIRE(locator->state_root() == "/state/user/org.nandina.todo");
    REQUIRE(locator->cache_root() == "/cache/user/org.nandina.todo");
    const auto roots = locator->resource_roots();
    REQUIRE(roots.size() == 5);
    REQUIRE(roots[0].root == "/opt/nandina/bin/resources");
    REQUIRE(roots[1].root == "/data/user/org.nandina.todo");
    REQUIRE(roots[2].root == "/data/company/org.nandina.todo");
    REQUIRE(roots[3].root == "/usr/share/org.nandina.todo");
    REQUIRE(roots[4].root == "/usr/local/share/org.nandina.todo");
}

TEST_CASE("Linux locator uses HOME and default system data directories", "[resource][locator]") {
    auto locator = resource::PlatformResourceLocator::create({
        .application_id = "org.nandina.todo",
        .executable_path = "/home/tester/bin/todo",
        .environment = {{"HOME", "/home/tester"}},
    });
    REQUIRE(locator.has_value());
    REQUIRE(locator->config_root() == "/home/tester/.config/org.nandina.todo");
    REQUIRE(locator->data_root() == "/home/tester/.local/share/org.nandina.todo");
    REQUIRE(locator->state_root() == "/home/tester/.local/state/org.nandina.todo");
    REQUIRE(locator->cache_root() == "/home/tester/.cache/org.nandina.todo");
    const auto roots = locator->resource_roots();
    REQUIRE(roots.size() == 4);
    REQUIRE(roots[2].root == "/usr/local/share/org.nandina.todo");
    REQUIRE(roots[3].root == "/usr/share/org.nandina.todo");

    REQUIRE_FALSE(
        resource::PlatformResourceLocator::create({
            .application_id = "../invalid",
            .executable_path = "/bin/app",
        })
    );
    REQUIRE_FALSE(
        resource::PlatformResourceLocator::create({
            .application_id = "valid.app",
            .executable_path = "relative/app",
            .environment = {{"HOME", "/home/tester"}},
        })
    );
    REQUIRE_FALSE(
        resource::PlatformResourceLocator::create({
            .application_id = "valid.app",
            .executable_path = "/bin/app",
            .environment = {{"HOME", "/home/tester"}, {"XDG_CONFIG_HOME", "relative/config"}},
        })
    );
}

TEST_CASE(
    "resource scanner is deterministic and follows media type precedence",
    "[resource][scan]"
) {
    TempDirectory temp;
    std::filesystem::create_directories(temp.path / "assets/nested");
    {
        std::ofstream file(temp.path / "assets/image.data", std::ios::binary);
        const std::array<unsigned char, 8> png {0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a};
        file.write(reinterpret_cast<const char*>(png.data()), png.size());
    }
    {
        std::ofstream file(temp.path / "assets/nested/config.json");
        file << "{}";
    }
    {
        std::ofstream file(temp.path / "assets/unknown.asset");
        file << "binary";
    }
    {
        std::ofstream file(temp.path / "assets/forced.png");
        file << "not-png";
    }

    const auto result = resource::scan_resources({
        .roots = {{.path = temp.path / "assets", .key_prefix = "app"}},
        .type_rules = {{"app/forced.png", "application/x-custom"}},
    });
    REQUIRE(result.valid());
    REQUIRE(result.resources.size() == 4);
    REQUIRE(result.resources[0].key.value() == "app/forced.png");
    REQUIRE(result.resources[0].media_type == "application/x-custom");
    REQUIRE(result.resources[1].key.value() == "app/image.data");
    REQUIRE(result.resources[1].media_type == "image/png");
    REQUIRE(result.resources[2].media_type == "application/json");
    REQUIRE(result.resources[3].media_type == "application/octet-stream");
}

TEST_CASE("resource scanner rejects unsafe paths and normalized collisions", "[resource][scan]") {
    TempDirectory temp;
    std::filesystem::create_directories(temp.path / "assets/.hidden");
    std::filesystem::create_directories(temp.path / "assets/build-output");
    std::filesystem::create_directories(temp.path / "assets/package");
    {
        std::ofstream file(temp.path / "assets/valid.txt");
        file << "ok";
    }
    {
        std::ofstream file(temp.path / "assets/Upper.txt");
        file << "a";
    }
    {
        std::ofstream file(temp.path / "assets/upper.txt");
        file << "b";
    }
    {
        std::ofstream file(temp.path / "assets/.hidden/secret.txt");
        file << "secret";
    }
    {
        std::ofstream file(temp.path / "assets/build-output/generated.bin");
        file << "generated";
    }
    {
        std::ofstream file(temp.path / "assets/package/resources.db");
        file << "output";
    }
    std::error_code symlink_error;
    std::filesystem::create_symlink(
        temp.path / "assets/valid.txt",
        temp.path / "assets/link.txt",
        symlink_error
    );

    const auto result = resource::scan_resources({
        .roots = {{.path = temp.path / "assets"}},
        .output_paths = {temp.path / "assets/package"},
    });
    REQUIRE_FALSE(result.valid());
    REQUIRE(std::ranges::any_of(result.diagnostics, [](const auto& diagnostic) {
        return diagnostic.code == resource::ScanDiagnosticCode::normalized_key_collision;
    }));
    REQUIRE(std::ranges::any_of(result.diagnostics, [](const auto& diagnostic) {
        return diagnostic.code == resource::ScanDiagnosticCode::output_recursion;
    }));
    if (!symlink_error) {
        REQUIRE(std::ranges::any_of(result.diagnostics, [](const auto& diagnostic) {
            return diagnostic.code == resource::ScanDiagnosticCode::symlink_rejected;
        }));
    }
    REQUIRE(std::ranges::none_of(result.resources, [](const auto& item) {
        return item.key.value().contains("hidden") || item.key.value().contains("build-output")
            || item.key.value().contains("package");
    }));
}

TEST_CASE("memory resources survive replacement and manager overlays", "[resource]") {
    const auto resource_id = id("00112233-4455-4677-8899-aabbccddeeff");
    auto base = std::make_shared<resource::MemoryBackend>("base");
    auto override = std::make_shared<resource::MemoryBackend>("override");
    auto old =
        base->insert(resource_id, resource::ResourceKey("fonts/default"), "font/ttf", {1, 2});
    REQUIRE(old.has_value());
    REQUIRE(base->add_alias(resource::ResourceKey("fonts/ui"), resource_id).has_value());
    REQUIRE(override->insert(resource_id, resource::ResourceKey("fonts/default"), "font/ttf", {9})
                .has_value());

    resource::ResourceManager manager;
    const auto base_mount = manager.mount(base, 0);
    const auto override_mount = manager.mount(override, 10);
    REQUIRE(manager.require(resource::ResourceKey("fonts/default"))->get()->bytes()[0] == 9);
    REQUIRE(manager.require(resource::ResourceKey("fonts/ui"))->get()->bytes()[0] == 1);
    REQUIRE(manager.unmount(override_mount));
    REQUIRE(manager.require(resource::ResourceKey("fonts/default"))->get()->bytes()[0] == 1);

    REQUIRE(base->insert(
                    resource_id,
                    resource::ResourceKey("fonts/default"),
                    "font/ttf",
                    {3},
                    resource::InsertMode::replace
    )
                .has_value());
    REQUIRE((*old)->bytes()[0] == 1);
    REQUIRE(manager.unmount(base_mount));
    REQUIRE((*old)->bytes()[1] == 2);
}

TEST_CASE(
    "builtin resources provide an overridable default font and license",
    "[resource][builtin]"
) {
    const auto builtin = resource::BuiltinBackend::create();
    REQUIRE(builtin->name() == "builtin");

    const auto font = builtin->find(resource::ResourceKey("fonts/default"));
    REQUIRE(font.has_value());
    REQUIRE(font->has_value());
    REQUIRE((**font)->id() == id("737980c0-0000-4000-8000-000000000001"));
    REQUIRE((**font)->media_type() == "font/ttf");
    REQUIRE((**font)->storage() == resource::ResourceStorage::embedded_blob);
    REQUIRE((**font)->size() > 100'000);

    const auto license = builtin->find(resource::ResourceKey("licenses/fonts/caskaydia-cove"));
    REQUIRE(license.has_value());
    REQUIRE(license->has_value());
    REQUIRE((**license)->media_type() == "text/plain");
    const std::string license_text((**license)->bytes().begin(), (**license)->bytes().end());
    REQUIRE(license_text.contains("SIL OPEN FONT LICENSE Version 1.1"));

    resource::ResourceManager manager;
    (void)manager.mount(builtin, -1000);
    auto override = std::make_shared<resource::MemoryBackend>("project");
    REQUIRE(override
                ->insert(
                    id("00112233-4455-4677-8899-aabbccddeeff"),
                    resource::ResourceKey("fonts/default"),
                    "font/ttf",
                    {1, 2, 3}
                )
                .has_value());
    (void)manager.mount(override, 0);
    REQUIRE(manager.require(resource::ResourceKey("fonts/default"))->get()->size() == 3);
}

TEST_CASE("directory backend loads manifest resources into owned handles", "[resource]") {
    TempDirectory temp;
    {
        std::ofstream file(temp.path / "asset.bin", std::ios::binary);
        file.write("abc", 3);
    }
    const auto resource_id = id("00112233-4455-4677-8899-aabbccddeeff");
    auto backend = resource::DirectoryBackend::open({
        .root = temp.path,
        .resources = {{
            .id = resource_id,
            .key = resource::ResourceKey("data/asset"),
            .relative_path = "asset.bin",
            .media_type = "application/octet-stream",
            .aliases = {resource::ResourceKey("data/default")},
            .expected_size = 3,
        }},
    });
    REQUIRE(backend.has_value());
    auto found = (*backend)->find(resource::ResourceKey("data/default"));
    REQUIRE(found.has_value());
    REQUIRE((**found)->size() == 3);
    std::filesystem::remove(temp.path / "asset.bin");
    (*backend).reset();
    REQUIRE((**found)->bytes()[2] == static_cast<std::uint8_t>('c'));
}

TEST_CASE("directory streams are bounded and survive backend removal", "[resource][stream]") {
    TempDirectory temp;
    {
        std::ofstream file(temp.path / "large.bin", std::ios::binary);
        file.write("abcdef", 6);
    }
    const auto resource_id = id("20112233-4455-4677-8899-aabbccddeeff");
    auto backend = resource::DirectoryBackend::open({
        .root = temp.path,
        .resources = {{
            .id = resource_id,
            .key = resource::ResourceKey("media/large"),
            .relative_path = "large.bin",
            .media_type = "application/octet-stream",
            .aliases = {resource::ResourceKey("media/default")},
            .expected_size = 6,
        }},
        .max_resource_size = 4,
        .max_stream_size = 8,
    });
    REQUIRE(backend.has_value());
    resource::ResourceManager manager;
    const auto mount = manager.mount(*backend);
    const auto snapshot = manager.find(resource::ResourceKey("media/large"));
    REQUIRE_FALSE(snapshot.has_value());
    REQUIRE(snapshot.error().code == resource::ResourceErrorCode::size_limit_exceeded);

    auto stream = manager.require_stream(resource::ResourceKey("media/default"));
    REQUIRE(stream.has_value());
    REQUIRE((*stream)->id() == resource_id);
    REQUIRE((*stream)->size() == 6);
    REQUIRE((*stream)->seekable());
    std::array<std::uint8_t, 4> buffer {};
    REQUIRE((*stream)->read(buffer) == 4);
    REQUIRE(std::string(buffer.begin(), buffer.end()) == "abcd");
    REQUIRE((*stream)->seek(2).has_value());
    REQUIRE((*stream)->read(buffer) == 4);
    REQUIRE(std::string(buffer.begin(), buffer.end()) == "cdef");
    REQUIRE((*stream)->read(buffer) == 0);
    REQUIRE_FALSE((*stream)->seek(7).has_value());

    REQUIRE(manager.unmount(mount));
    (*backend).reset();
    std::filesystem::remove(temp.path / "large.bin");
    REQUIRE((*stream)->seek(0).has_value());
    REQUIRE((*stream)->read(buffer) == 4);
    REQUIRE(std::string(buffer.begin(), buffer.end()) == "abcd");
}

TEST_CASE("SQLite backend resolves blobs aliases and external files", "[resource][sqlite]") {
    TempDirectory temp;
    const auto database = temp.path / "resources.db";
    create_database(database);
    {
        std::ofstream file(temp.path / "basic.glsl", std::ios::binary);
        file.write("main", 4);
    }
    auto backend = resource::SQLiteBackend::open({.database = database});
    REQUIRE(backend.has_value());

    auto blob = (*backend)->find(resource::ResourceKey("fonts/ui"));
    REQUIRE(blob.has_value());
    REQUIRE((**blob)->key().value() == "fonts/default");
    REQUIRE((**blob)->storage() == resource::ResourceStorage::embedded_blob);
    REQUIRE((**blob)->bytes()[2] == 3);

    auto external = (*backend)->find(resource::ResourceKey("shaders/basic"));
    REQUIRE(external.has_value());
    REQUIRE((**external)->storage() == resource::ResourceStorage::external_file);
    REQUIRE((**external)->size() == 4);

    auto blob_stream = (*backend)->open_stream(resource::ResourceKey("fonts/ui"));
    REQUIRE(blob_stream.has_value());
    REQUIRE(blob_stream->has_value());
    std::array<std::uint8_t, 8> bytes {};
    REQUIRE((**blob_stream)->read(bytes) == 3);
    REQUIRE(bytes[0] == 1);
    REQUIRE(bytes[2] == 3);

    auto external_stream = (*backend)->open_stream(id("10112233-4455-4677-8899-aabbccddeeff"));
    REQUIRE(external_stream.has_value());
    REQUIRE(external_stream->has_value());
    REQUIRE((**external_stream)->read(bytes) == 4);
    REQUIRE(std::string(bytes.begin(), bytes.begin() + 4) == "main");

    std::filesystem::remove(database);
    (*backend).reset();
    REQUIRE((**blob)->bytes()[0] == 1);
    REQUIRE((**external_stream)->seek(0).has_value());
    REQUIRE((**external_stream)->read(bytes) == 4);
}

TEST_CASE("SQLite backend rejects non-Nandina databases", "[resource][sqlite]") {
    TempDirectory temp;
    const auto database = temp.path / "foreign.db";
    sqlite3* db = nullptr;
    REQUIRE(sqlite3_open(database.string().c_str(), &db) == SQLITE_OK);
    exec(db, "CREATE TABLE unrelated(value INTEGER);");
    REQUIRE(sqlite3_close(db) == SQLITE_OK);
    const auto backend = resource::SQLiteBackend::open({.database = database});
    REQUIRE_FALSE(backend.has_value());
    REQUIRE(backend.error().code == resource::ResourceErrorCode::malformed_backend);
}
