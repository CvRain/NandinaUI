#include <catch2/catch_test_macros.hpp>

#include <nandina/resource/backends/builtin_backend.hpp>
#include <nandina/resource/backends/directory_backend.hpp>
#include <nandina/resource/backends/memory_backend.hpp>
#ifdef NANDINA_HAS_RESOURCE_SQLITE
    #include <nandina/resource/backends/sqlite_backend.hpp>
    #include <sqlite3.h>
#endif
#include <nandina/resource/platform_resource_locator.hpp>
#include <nandina/resource/resource_manager.hpp>
#include <nandina/resource/resource_manifest.hpp>
#include <nandina/resource/resource_scanner.hpp>
#include <nandina/resource/resource_uri.hpp>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <limits>
#include <mutex>
#include <thread>
#include <vector>

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

    /// 让一次后端查询停在中途，用来证明管理器不持全局锁调用后端，且卸载后旧快照仍被固定。
    class BlockingLookupBackend final: public resource::IResourceBackend {
    public:
        explicit BlockingLookupBackend(std::shared_ptr<resource::MemoryBackend> inner):
            inner_(std::move(inner)) {}

        [[nodiscard]] auto name() const noexcept -> std::string_view override {
            return inner_->name();
        }
        [[nodiscard]] auto find(const resource::ResourceKey& key) const
            -> resource::ResourceSnapshotLookup override {
            block();
            return inner_->find(key);
        }
        [[nodiscard]] auto find(resource::ResourceId id) const
            -> resource::ResourceSnapshotLookup override {
            block();
            return inner_->find(id);
        }

        void wait_until_entered() const {
            std::unique_lock lock(mutex_);
            condition_.wait(lock, [this] { return entered_; });
        }
        void release() const {
            std::unique_lock lock(mutex_);
            released_ = true;
            condition_.notify_all();
        }

    private:
        void block() const {
            std::unique_lock lock(mutex_);
            entered_ = true;
            condition_.notify_all();
            condition_.wait(lock, [this] { return released_; });
        }

        std::shared_ptr<resource::MemoryBackend> inner_;
        mutable std::mutex mutex_;
        mutable std::condition_variable condition_;
        mutable bool entered_ = false;
        mutable bool released_ = false;
    };

    /// 所有查询都失败的后端，用来证明 ID 扫描不会忽略错误后宣称唯一命中。
    class FailingLookupBackend final: public resource::IResourceBackend {
    public:
        [[nodiscard]] auto name() const noexcept -> std::string_view override {
            return "failing";
        }
        [[nodiscard]] auto find(const resource::ResourceKey&) const
            -> resource::ResourceSnapshotLookup override {
            return std::unexpected(failure());
        }
        [[nodiscard]] auto find(resource::ResourceId) const
            -> resource::ResourceSnapshotLookup override {
            return std::unexpected(failure());
        }
        [[nodiscard]] auto open_stream(const resource::ResourceKey&) const
            -> resource::ResourceStreamSnapshotLookup override {
            return std::unexpected(failure());
        }
        [[nodiscard]] auto open_stream(resource::ResourceId) const
            -> resource::ResourceStreamSnapshotLookup override {
            return std::unexpected(failure());
        }

    private:
        [[nodiscard]] static auto failure() -> resource::ResourceError {
            return {
                resource::ResourceErrorCode::integrity_error,
                "find",
                "injected backend failure",
            };
        }
    };

#ifdef NANDINA_HAS_RESOURCE_SQLITE
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
#endif
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

#if !defined(_WIN32)
// 这两个用例用 POSIX 绝对路径（/opt、/home）验证 XDG 语义；Windows 的盘符路径不是
// 绝对路径，且 Windows 资源定位只是兼容 shim（HOME→USERPROFILE），未验收，先不在此断言。
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

#endif // !defined(_WIN32)

#ifdef NANDINA_TEST_RESOURCE_TOOLS
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
    #if !defined(_WIN32)
    // NTFS 默认大小写不敏感：Upper.txt / upper.txt 会落到同一个文件，碰撞场景无法构造。
    {
        std::ofstream file(temp.path / "assets/Upper.txt");
        file << "a";
    }
    {
        std::ofstream file(temp.path / "assets/upper.txt");
        file << "b";
    }
    #endif
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
    #if !defined(_WIN32)
    REQUIRE(std::ranges::any_of(result.diagnostics, [](const auto& diagnostic) {
        return diagnostic.code == resource::ScanDiagnosticCode::normalized_key_collision;
    }));
    #endif
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

TEST_CASE("resource id derivation matches frozen vectors", "[resource][identity]") {
    const auto small = resource::ResourceKey("app/small.txt");
    REQUIRE(
        resource::derive_resource_id("org.nandina.test", small).to_string()
        == "2320d16a-80d2-4640-fb6a-afb242396aff"
    );
    REQUIRE(
        resource::derive_resource_id("org.nandina.test", resource::ResourceKey("app/small2.txt"))
            .to_string()
        == "c742fdac-210f-b9ac-e68c-ced7380c4a8b"
    );
    REQUIRE(
        resource::derive_resource_id("org.nandina.other", small).to_string()
        == "65905463-1da5-dcc8-a0fb-5a0919451868"
    );
    REQUIRE(
        resource::derive_resource_id("org.nandina.test", small)
        == resource::derive_resource_id("org.nandina.test", small)
    );
    REQUIRE(
        resource::derive_resource_id("org.nandina.test", small)
        != resource::derive_resource_id("org.nandina.other", small)
    );
    // Length prefixes keep "ab"+"c" distinct from "a"+"bc"; plain concatenation would collide.
    REQUIRE(
        resource::derive_resource_id("ab", resource::ResourceKey("c"))
        != resource::derive_resource_id("a", resource::ResourceKey("bc"))
    );
}

TEST_CASE("scanner keeps case-insensitive slots for non-canonical names", "[resource][scan]") {
    // 两个根按顺序访问，因此“大写名先被看到”的顺序是确定的：非规范名也必须占用其大小写
    // 归一化槽位，否则碰撞诊断会随目录遍历顺序漂移。
    TempDirectory temp;
    std::filesystem::create_directories(temp.path / "first");
    std::filesystem::create_directories(temp.path / "second");
    {
        std::ofstream file(temp.path / "first/Upper.txt");
        file << "a";
    }
    {
        std::ofstream file(temp.path / "second/upper.txt");
        file << "b";
    }
    const auto result = resource::scan_resources({
        .roots = {
            {.path = temp.path / "first"},
            {.path = temp.path / "second"},
        },
    });
    REQUIRE_FALSE(result.valid());
    REQUIRE(
        std::ranges::count_if(
            result.diagnostics,
            [](const auto& diagnostic) {
                return diagnostic.code == resource::ScanDiagnosticCode::normalized_key_collision;
            }
        )
        == 1
    );
}

TEST_CASE("resource key derivation preserves prefix and path", "[resource][identity]") {
    const auto prefixed = resource::derive_resource_key("app", "dir/file.txt");
    REQUIRE(prefixed.has_value());
    REQUIRE(prefixed->value() == "app/dir/file.txt");
    const auto bare = resource::derive_resource_key("", "file.txt");
    REQUIRE(bare.has_value());
    REQUIRE(bare->value() == "file.txt");
    // ResourceKey 只允许小写规范段；大写路径不能成为隐式 key。
    REQUIRE_FALSE(resource::derive_resource_key("app", "Fonts/Default.ttf"));
    REQUIRE_FALSE(resource::derive_resource_key("Bad", "file.txt"));
    REQUIRE_FALSE(resource::derive_resource_key("app", ""));
    REQUIRE_FALSE(resource::derive_resource_key("app", "a/../b"));
    REQUIRE_FALSE(resource::derive_resource_key("app", "/absolute/file.txt"));
}

TEST_CASE("scanner leaves explicitly mapped paths to the mapping", "[resource][scan]") {
    TempDirectory temp;
    std::filesystem::create_directories(temp.path / "assets");
    {
        std::ofstream file(temp.path / "assets/Upper.txt");
        file << "a";
    }
    const auto mapped = temp.path / "assets/Upper.txt";
    // 映射路径的身份归一化：`. /` 与直接拼写是同一个文件。
    REQUIRE(
        resource::resource_path_identity(temp.path / "assets" / "." / "Upper.txt")
        == resource::resource_path_identity(mapped)
    );
    const auto skipped = resource::scan_resources({
        .roots = {{.path = temp.path / "assets"}},
        .mapped_paths = {mapped},
    });
    // 无法派生规范 key 的文件由显式映射拥有：扫描器不派生、不诊断。
    REQUIRE(skipped.valid());
    REQUIRE(skipped.resources.empty());
    const auto derived = resource::scan_resources({
        .roots = {{.path = temp.path / "assets"}},
    });
    REQUIRE_FALSE(derived.valid());
    REQUIRE(std::ranges::any_of(derived.diagnostics, [](const auto& diagnostic) {
        return diagnostic.code == resource::ScanDiagnosticCode::invalid_resource_key;
    }));
}

#endif

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
    REQUIRE(manager.require(resource::ResourceKey("fonts/default"))->resource->bytes()[0] == 9);
    REQUIRE(manager.require(resource::ResourceKey("fonts/ui"))->resource->bytes()[0] == 1);
    REQUIRE(manager.unmount(override_mount));
    REQUIRE(manager.require(resource::ResourceKey("fonts/default"))->resource->bytes()[0] == 1);

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

TEST_CASE("publish generations advance monotonically and never wrap", "[resource][generation]") {
    const resource::PublishGeneration initial;
    REQUIRE(initial.value() == 0);
    const auto first = initial.next();
    REQUIRE(first.has_value());
    REQUIRE(first->value() == 1);

    const auto maximum = std::numeric_limits<std::uint64_t>::max();
    const resource::PublishGeneration penultimate(maximum - 1);
    const auto last = penultimate.next();
    REQUIRE(last.has_value());
    REQUIRE(last->value() == maximum);
    // 回绕会让旧缓存身份与后来的发布相等，因此在最大值处拒绝前进。
    REQUIRE_FALSE(last->next().has_value());
    REQUIRE_FALSE(resource::PublishGeneration(maximum).next().has_value());
}

TEST_CASE(
    "memory backend publishes a result and its generation together",
    "[resource][generation]"
) {
    auto backend = std::make_shared<resource::MemoryBackend>("publish");
    const auto key = resource::ResourceKey("data/current");
    const auto alias = resource::ResourceKey("data/alias");
    const auto missing = resource::ResourceKey("data/missing");
    const auto resource_id = id("30112233-4455-4677-8899-aabbccddeeff");
    const auto other_id = id("31112233-4455-4677-8899-aabbccddeeff");

    const auto observed_generation = [&] {
        const auto snapshot = backend->find(missing);
        REQUIRE(snapshot.has_value());
        REQUIRE_FALSE(snapshot->resource.has_value());
        REQUIRE(snapshot->consistency == resource::ResourceConsistency::immutable_snapshot);
        return snapshot->generation.value();
    };

    REQUIRE(observed_generation() == 0);
    const auto missing_stream = backend->open_stream(missing);
    REQUIRE(missing_stream.has_value());
    REQUIRE(missing_stream->generation.value() == 0);
    REQUIRE(missing_stream->consistency == resource::ResourceConsistency::immutable_snapshot);
    REQUIRE_FALSE(missing_stream->stream.has_value());

    REQUIRE(backend->insert(resource_id, key, "application/octet-stream", {1}).has_value());

    auto published = backend->find(key);
    REQUIRE(published.has_value());
    REQUIRE(published->generation.value() == 1);
    REQUIRE(published->consistency == resource::ResourceConsistency::immutable_snapshot);
    REQUIRE(published->resource.has_value());
    REQUIRE((*published->resource)->bytes()[0] == 1);

    auto stream = backend->open_stream(key);
    REQUIRE(stream.has_value());
    REQUIRE(stream->generation.value() == 1);
    REQUIRE(stream->consistency == resource::ResourceConsistency::immutable_snapshot);
    REQUIRE(stream->stream.has_value());

    // 失败的发布不推进代次，也不改变可观察结果。
    REQUIRE_FALSE(backend->insert(resource_id, key, "application/octet-stream", {2}).has_value());
    REQUIRE(observed_generation() == 1);
    REQUIRE_FALSE(backend->insert(other_id, key, "application/octet-stream", {2}).has_value());
    REQUIRE(observed_generation() == 1);
    REQUIRE_FALSE(backend->add_alias(alias, other_id).has_value());
    REQUIRE(observed_generation() == 1);

    REQUIRE(backend->add_alias(alias, resource_id).has_value());
    REQUIRE(observed_generation() == 2);
    auto aliased = backend->find(alias);
    REQUIRE(aliased.has_value());
    REQUIRE(aliased->generation.value() == 2);
    REQUIRE((*aliased->resource)->id() == resource_id);
    REQUIRE_FALSE(backend->add_alias(alias, resource_id).has_value());
    REQUIRE(observed_generation() == 2);

    const auto previous = published->resource;
    REQUIRE(backend
                ->insert(
                    resource_id,
                    key,
                    "application/octet-stream",
                    {9},
                    resource::InsertMode::replace
                )
                .has_value());
    REQUIRE(observed_generation() == 3);
    auto replaced = backend->find(key);
    REQUIRE(replaced->generation.value() == 3);
    REQUIRE((*replaced->resource)->bytes()[0] == 9);
    // 旧句柄仍是旧快照，不会被新发布改写。
    REQUIRE((*previous)->bytes()[0] == 1);

    REQUIRE_FALSE(backend->erase(other_id));
    REQUIRE(observed_generation() == 3);
    REQUIRE(backend->erase(resource_id));
    REQUIRE(observed_generation() == 4);
    const auto erased = backend->find(key);
    REQUIRE(erased.has_value());
    REQUIRE(erased->generation.value() == 4);
    REQUIRE_FALSE(erased->resource.has_value());
}

TEST_CASE(
    "multiple mounts of one backend observe the same publish generation",
    "[resource][generation]"
) {
    const auto key = resource::ResourceKey("data/shared");
    auto backend = std::make_shared<resource::MemoryBackend>("shared");
    REQUIRE(backend
                ->insert(
                    id("32112233-4455-4677-8899-aabbccddeeff"),
                    key,
                    "application/octet-stream",
                    {4}
                )
                .has_value());

    resource::ResourceManager manager;
    const auto first_mount = manager.mount(backend, 0);
    const auto second_mount = manager.mount(backend, 0);
    REQUIRE(first_mount != second_mount);

    const auto left = backend->find(key);
    const auto right = backend->find(key);
    REQUIRE(left.has_value());
    REQUIRE(right.has_value());
    REQUIRE(left->generation == right->generation);
    REQUIRE(left->generation.value() == 1);
    REQUIRE(manager.require(key).has_value());
    REQUIRE(manager.require(key)->resource->bytes()[0] == 4);
}

TEST_CASE(
    "concurrent publishes never pair old content with a new generation",
    "[resource][generation][concurrency]"
) {
    constexpr std::uint64_t publish_count = 20'000;
    const auto key = resource::ResourceKey("data/current");
    const auto resource_id = id("33112233-4455-4677-8899-aabbccddeeff");
    const auto encode = [](const std::uint64_t value) {
        std::vector<std::uint8_t> bytes(sizeof(value));
        std::memcpy(bytes.data(), &value, sizeof(value));
        return bytes;
    };
    auto backend = std::make_shared<resource::MemoryBackend>("concurrent");
    REQUIRE(backend->insert(resource_id, key, "application/octet-stream", encode(1)).has_value());

    std::atomic<bool> start = false;
    std::atomic<bool> failed = false;
    std::jthread writer([&] {
        while (!start.load(std::memory_order_acquire)) {}
        for (std::uint64_t generation = 1; generation < publish_count; ++generation) {
            const auto inserted = backend->insert(
                resource_id,
                key,
                "application/octet-stream",
                encode(generation + 1),
                resource::InsertMode::replace
            );
            if (!inserted) {
                failed.store(true, std::memory_order_relaxed);
                return;
            }
        }
    });

    std::array<std::jthread, 3> readers;
    for (auto& reader: readers) {
        reader = std::jthread([&] {
            while (!start.load(std::memory_order_acquire)) {}
            std::uint64_t previous = 0;
            for (int iteration = 0; iteration < 30'000; ++iteration) {
                if (failed.load(std::memory_order_relaxed)) {
                    return;
                }
                const auto snapshot = backend->find(key);
                if (!snapshot || !snapshot->resource
                    || snapshot->consistency != resource::ResourceConsistency::immutable_snapshot)
                {
                    failed.store(true, std::memory_order_relaxed);
                    return;
                }
                const auto generation = snapshot->generation.value();
                const auto bytes = (*snapshot->resource)->bytes();
                if (generation < previous || bytes.size() != sizeof(std::uint64_t)) {
                    failed.store(true, std::memory_order_relaxed);
                    return;
                }
                std::uint64_t content = 0;
                std::memcpy(&content, bytes.data(), sizeof(content));
                if (content != generation) {
                    failed.store(true, std::memory_order_relaxed);
                    return;
                }
                previous = generation;
            }
        });
    }

    start.store(true, std::memory_order_release);
    writer.join();
    for (auto& reader: readers) {
        reader.join();
    }
    REQUIRE_FALSE(failed.load());
}

TEST_CASE(
    "manager pins a selected backend snapshot and does not call backends under its lock",
    "[resource][generation][concurrency]"
) {
    const auto key = resource::ResourceKey("data/value");
    auto inner = std::make_shared<resource::MemoryBackend>("blocking");
    REQUIRE(inner
                ->insert(
                    id("34112233-4455-4677-8899-aabbccddeeff"),
                    key,
                    "application/octet-stream",
                    {7}
                )
                .has_value());
    auto backend = std::make_shared<BlockingLookupBackend>(std::move(inner));

    resource::ResourceManager manager;
    const auto mount = manager.mount(backend);

    std::optional<resource::ResolvedResourceLookup> lookup;
    std::jthread finder([&] { lookup = manager.find(key); });
    backend->wait_until_entered();
    // 查询停在后端期间管理器必须仍可卸载；否则说明后端调用发生在管理器锁内。
    REQUIRE(manager.unmount(mount));
    backend->release();
    finder.join();

    REQUIRE(lookup.has_value());
    REQUIRE(lookup->has_value());
    const auto pinned = ***lookup;
    REQUIRE(pinned.resource->bytes()[0] == 7);

    // 卸载完成后新发起的解析不得成功；已经固定的旧结果继续可用，但旧作用域重新解析失败。
    const auto after = manager.find(key);
    REQUIRE(after.has_value());
    REQUIRE_FALSE(after->has_value());
    const auto expired = manager.resolve(pinned);
    REQUIRE_FALSE(expired.has_value());
    REQUIRE(expired.error().code == resource::ResourceErrorCode::expired_scope);
}

TEST_CASE("resolved results carry domain mount generation and identity", "[resource][scope]") {
    const auto key = resource::ResourceKey("data/value");
    const auto resource_id = id("50112233-4455-4677-8899-aabbccddeeff");
    auto backend = std::make_shared<resource::MemoryBackend>("scope");
    REQUIRE(backend->insert(resource_id, key, "application/octet-stream", {5}).has_value());

    resource::ResourceManager manager;
    const auto mount = manager.mount(backend);

    const auto by_key = manager.require(key);
    REQUIRE(by_key.has_value());
    REQUIRE_FALSE(by_key->domain.expired());
    REQUIRE(by_key->mount_id == mount);
    REQUIRE(by_key->generation.value() == 1);
    REQUIRE(by_key->consistency == resource::ResourceConsistency::immutable_snapshot);
    REQUIRE(by_key->id == resource_id);
    REQUIRE(by_key->resource->id() == resource_id);

    const auto by_id = manager.find(resource_id);
    REQUIRE(by_id.has_value());
    REQUIRE(by_id->has_value());
    REQUIRE((*by_id)->domain == by_key->domain);
    REQUIRE((*by_id)->mount_id == mount);
    REQUIRE((*by_id)->generation == by_key->generation);

    const auto stream = manager.require_stream(key);
    REQUIRE(stream.has_value());
    REQUIRE(stream->domain == by_key->domain);
    REQUIRE(stream->mount_id == mount);
    REQUIRE(stream->generation == by_key->generation);
    REQUIRE(stream->consistency == resource::ResourceConsistency::immutable_snapshot);
    REQUIRE(stream->id == resource_id);
}

TEST_CASE("bare resource ids are ambiguous across mounts", "[resource][scope][ambiguity]") {
    const auto resource_id = id("51112233-4455-4677-8899-aabbccddeeff");
    auto first = std::make_shared<resource::MemoryBackend>("first");
    auto second = std::make_shared<resource::MemoryBackend>("second");
    REQUIRE(first->insert(resource_id, resource::ResourceKey("data/first"), "text/plain", {1})
                .has_value());
    REQUIRE(second->insert(resource_id, resource::ResourceKey("data/second"), "text/plain", {2})
                .has_value());

    resource::ResourceManager manager;
    const auto first_mount = manager.mount(first);
    const auto second_mount = manager.mount(second);

    const auto found = manager.find(resource_id);
    REQUIRE_FALSE(found.has_value());
    REQUIRE(found.error().code == resource::ResourceErrorCode::ambiguous_resource_id);
    REQUIRE(found.error().message.contains(std::format("mount {} (first)", first_mount)));
    REQUIRE(found.error().message.contains(std::format("mount {} (second)", second_mount)));

    REQUIRE(
        manager.require(resource_id).error().code
        == resource::ResourceErrorCode::ambiguous_resource_id
    );
    REQUIRE(
        manager.open_stream(resource_id).error().code
        == resource::ResourceErrorCode::ambiguous_resource_id
    );
    REQUIRE(
        manager.require_stream(resource_id).error().code
        == resource::ResourceErrorCode::ambiguous_resource_id
    );

    // 同一后端挂两次也是两个挂载命中，不能按指针去重成“唯一”。
    resource::ResourceManager duplicated;
    const auto duplicate_first = duplicated.mount(first);
    const auto duplicate_second = duplicated.mount(first);
    const auto duplicated_hit = duplicated.find(resource_id);
    REQUIRE_FALSE(duplicated_hit.has_value());
    REQUIRE(duplicated_hit.error().code == resource::ResourceErrorCode::ambiguous_resource_id);
    REQUIRE(
        duplicated_hit.error().message.contains(std::format("mount {} (first)", duplicate_first))
    );
    REQUIRE(
        duplicated_hit.error().message.contains(std::format("mount {} (first)", duplicate_second))
    );

    // 单一命中的 ID 查询继续可用。
    REQUIRE(duplicated.unmount(duplicate_second));
    const auto unique = duplicated.find(resource_id);
    REQUIRE(unique.has_value());
    REQUIRE(unique->has_value());
    REQUIRE((*unique)->mount_id == duplicate_first);

    // 扫描全部挂载，但只有一个挂载真正持有该 ID：这是唯一命中，不是歧义。
    auto empty = std::make_shared<resource::MemoryBackend>("empty");
    resource::ResourceManager partial;
    const auto partial_hit = partial.mount(first);
    (void)partial.mount(empty);
    const auto only_one = partial.find(resource_id);
    REQUIRE(only_one.has_value());
    REQUIRE(only_one->has_value());
    REQUIRE((*only_one)->mount_id == partial_hit);
}

TEST_CASE(
    "alias resolution follows mount priority inside the key path",
    "[resource][scope][alias]"
) {
    const auto low_id = id("52112233-4455-4677-8899-aabbccddeeff");
    const auto high_id = id("53112233-4455-4677-8899-aabbccddeeff");
    const auto canonical = resource::ResourceKey("fonts/default");
    const auto low_only = resource::ResourceKey("app/logo");

    auto low = std::make_shared<resource::MemoryBackend>("low");
    auto high = std::make_shared<resource::MemoryBackend>("high");
    REQUIRE(low->insert(low_id, canonical, "font/ttf", {1}).has_value());
    REQUIRE(
        high->insert(high_id, resource::ResourceKey("fonts/override"), "font/ttf", {2}).has_value()
    );
    REQUIRE(high->add_alias(canonical, high_id).has_value());
    REQUIRE(low->add_alias(low_only, low_id).has_value());

    resource::ResourceManager manager;
    const auto low_mount = manager.mount(low, 0);
    const auto high_mount = manager.mount(high, 10);

    // 高优先级后端的别名参与 key 路径，压过低优先级的同 key 资源。
    const auto aliased = manager.require(canonical);
    REQUIRE(aliased->mount_id == high_mount);
    REQUIRE(aliased->resource->bytes()[0] == 2);

    // 别名只在所属后端内解析，不会跨挂载搜索目标。
    const auto low_alias = manager.require(low_only);
    REQUIRE(low_alias->mount_id == low_mount);
    REQUIRE(low_alias->resource->bytes()[0] == 1);

    REQUIRE(manager.unmount(high_mount));
    const auto canonical_again = manager.require(canonical);
    REQUIRE(canonical_again->mount_id == low_mount);
    REQUIRE(canonical_again->resource->bytes()[0] == 1);
}

TEST_CASE("an id scan never ignores a backend error to claim a unique hit", "[resource][scope]") {
    const auto resource_id = id("54112233-4455-4677-8899-aabbccddeeff");
    auto hit = std::make_shared<resource::MemoryBackend>("hit");
    REQUIRE(
        hit->insert(resource_id, resource::ResourceKey("data/hit"), "text/plain", {1}).has_value()
    );
    auto failing = std::make_shared<FailingLookupBackend>();

    resource::ResourceManager manager;
    (void)manager.mount(hit, 10);
    (void)manager.mount(failing, 0);
    const auto found = manager.find(resource_id);
    REQUIRE_FALSE(found.has_value());
    REQUIRE(found.error().code == resource::ResourceErrorCode::integrity_error);

    // key 查询按优先级短路：更高优先级已有命中的后端不会被低优先级错误干扰。
    REQUIRE(manager.require(resource::ResourceKey("data/hit"))->resource->bytes()[0] == 1);

    // 反过来，高优先级错误不能被低优先级命中忽略。
    resource::ResourceManager failing_first;
    (void)failing_first.mount(hit, 0);
    (void)failing_first.mount(failing, 10);
    const auto key_error = failing_first.find(resource::ResourceKey("data/hit"));
    REQUIRE_FALSE(key_error.has_value());
    REQUIRE(key_error.error().code == resource::ResourceErrorCode::integrity_error);
}

TEST_CASE(
    "resolved references fail after publish unmount or domain loss",
    "[resource][scope][expired]"
) {
    const auto key = resource::ResourceKey("data/value");
    const auto resource_id = id("55112233-4455-4677-8899-aabbccddeeff");
    auto backend = std::make_shared<resource::MemoryBackend>("scope");
    REQUIRE(backend->insert(resource_id, key, "text/plain", {1}).has_value());

    resource::ResourceManager manager;
    const auto mount = manager.mount(backend);

    const auto first = manager.require(key);
    REQUIRE(first.has_value());
    const auto reopened = manager.resolve(*first);
    REQUIRE(reopened.has_value());
    REQUIRE((*reopened)->generation == first->generation);
    REQUIRE((*reopened)->mount_id == first->mount_id);
    REQUIRE((*reopened)->resource->bytes()[0] == 1);

    const auto stream = manager.require_stream(key);
    REQUIRE(stream.has_value());
    const auto reopened_stream = manager.resolve(*stream);
    REQUIRE(reopened_stream.has_value());
    REQUIRE((*reopened_stream)->generation == stream->generation);

    // 发布推进代次后，旧引用不能重新解析到新内容。
    REQUIRE(backend->insert(resource_id, key, "text/plain", {2}, resource::InsertMode::replace)
                .has_value());
    const auto stale = manager.resolve(*first);
    REQUIRE_FALSE(stale.has_value());
    REQUIRE(stale.error().code == resource::ResourceErrorCode::expired_scope);
    const auto stale_stream = manager.resolve(*stream);
    REQUIRE_FALSE(stale_stream.has_value());
    REQUIRE(stale_stream.error().code == resource::ResourceErrorCode::expired_scope);

    const auto fresh = manager.require(key);
    REQUIRE(fresh->generation.value() == 2);
    REQUIRE(fresh->resource->bytes()[0] == 2);
    const auto fresh_stream = manager.require_stream(key);
    REQUIRE(fresh_stream.has_value());

    // 卸载后即使另一个挂载提供相同 ID/key，旧引用也不得回退到全树搜索。
    REQUIRE(manager.unmount(mount));
    auto replacement = std::make_shared<resource::MemoryBackend>("replacement");
    REQUIRE(replacement->insert(resource_id, key, "text/plain", {9}).has_value());
    (void)manager.mount(replacement);
    REQUIRE(manager.require(key)->resource->bytes()[0] == 9);
    const auto unmounted = manager.resolve(*fresh);
    REQUIRE_FALSE(unmounted.has_value());
    REQUIRE(unmounted.error().code == resource::ResourceErrorCode::expired_scope);
    const auto unmounted_stream = manager.resolve(*fresh_stream);
    REQUIRE_FALSE(unmounted_stream.has_value());
    REQUIRE(unmounted_stream.error().code == resource::ResourceErrorCode::expired_scope);

    // clear() 等价于卸载全部挂载：新引用也会在 clear 后过期。
    const auto replacement_reference = manager.require(key);
    REQUIRE(replacement_reference.has_value());
    const auto replacement_stream = manager.require_stream(key);
    REQUIRE(replacement_stream.has_value());
    manager.clear();
    REQUIRE(manager.require(key).error().code == resource::ResourceErrorCode::not_found);
    const auto cleared = manager.resolve(*replacement_reference);
    REQUIRE_FALSE(cleared.has_value());
    REQUIRE(cleared.error().code == resource::ResourceErrorCode::expired_scope);
    const auto cleared_stream = manager.resolve(*replacement_stream);
    REQUIRE_FALSE(cleared_stream.has_value());
    REQUIRE(cleared_stream.error().code == resource::ResourceErrorCode::expired_scope);
}

TEST_CASE(
    "domain tokens are ownership identities and never keep the manager alive",
    "[resource][scope][domain]"
) {
    const auto key = resource::ResourceKey("data/value");
    auto backend = std::make_shared<resource::MemoryBackend>("domain");
    REQUIRE(backend->insert(id("56112233-4455-4677-8899-aabbccddeeff"), key, "text/plain", {1})
                .has_value());

    std::optional<resource::ResolvedResource> reference;
    resource::ResourceDomainToken first_domain;
    {
        resource::ResourceManager owner;
        (void)owner.mount(backend);
        auto resolved = owner.require(key);
        REQUIRE(resolved.has_value());
        reference = *resolved;
        first_domain = resolved->domain;
        REQUIRE_FALSE(first_domain.expired());
    }
    REQUIRE(first_domain.expired());
    REQUIRE(reference.has_value());
    REQUIRE(reference->domain.expired());

    resource::ResourceDomainToken second_destroyed;
    {
        resource::ResourceManager temp;
        (void)temp.mount(backend);
        auto resolved = temp.require(key);
        REQUIRE(resolved.has_value());
        second_destroyed = resolved->domain;
    }
    // 两个已销毁域仍是不同身份，不能把所有 expired 令牌当成同一个空值。
    REQUIRE(second_destroyed.expired());
    REQUIRE_FALSE(first_domain == second_destroyed);

    resource::ResourceManager other;
    (void)other.mount(backend);
    const auto foreign = other.resolve(*reference);
    REQUIRE_FALSE(foreign.has_value());
    REQUIRE(foreign.error().code == resource::ResourceErrorCode::expired_scope);
    auto other_reference = other.require(key);
    REQUIRE_FALSE(other_reference->domain == first_domain);

    // 移动构造/赋值转移同一逻辑域。
    auto before_move = other.require(key);
    resource::ResourceManager moved(std::move(other));
    const auto after_move = moved.resolve(*before_move);
    REQUIRE(after_move.has_value());
    REQUIRE((*after_move)->domain == before_move->domain);

    resource::ResourceManager assigned;
    assigned = std::move(moved);
    const auto after_assignment = assigned.resolve(*before_move);
    REQUIRE(after_assignment.has_value());
    REQUIRE((*after_assignment)->mount_id == before_move->mount_id);

    // 未绑定任何域的默认引用不能被当成当前域成员。
    const resource::ResolvedResource unbound {};
    const auto unbound_result = assigned.resolve(unbound);
    REQUIRE_FALSE(unbound_result.has_value());
    REQUIRE(unbound_result.error().code == resource::ResourceErrorCode::expired_scope);
}

TEST_CASE("mount ids are unique within a domain and never reused", "[resource][scope][domain]") {
    auto backend = std::make_shared<resource::MemoryBackend>("mounts");
    resource::ResourceManager manager;
    const auto first = manager.mount(backend);
    REQUIRE(manager.unmount(first));
    const auto second = manager.mount(backend);
    REQUIRE(second != first);
    manager.clear();
    const auto third = manager.mount(backend);
    REQUIRE(third != first);
    REQUIRE(third != second);
}

TEST_CASE(
    "builtin resources provide an overridable default font and license",
    "[resource][builtin]"
) {
    const auto builtin = resource::BuiltinBackend::create();
    REQUIRE(builtin->name() == "builtin");

    const auto font = builtin->find(resource::ResourceKey("fonts/default"));
    REQUIRE(font.has_value());
    REQUIRE(font->resource.has_value());
    REQUIRE(font->generation.value() == 0);
    REQUIRE(font->consistency == resource::ResourceConsistency::immutable_snapshot);
    REQUIRE(resource::is_snapshot(font->consistency));
    REQUIRE((*font->resource)->id() == id("737980c0-0000-4000-8000-000000000001"));
    REQUIRE((*font->resource)->media_type() == "font/ttf");
    REQUIRE((*font->resource)->storage() == resource::ResourceStorage::embedded_blob);
    REQUIRE((*font->resource)->size() > 100'000);

    const auto license = builtin->find(resource::ResourceKey("licenses/fonts/caskaydia-cove"));
    REQUIRE(license.has_value());
    REQUIRE(license->resource.has_value());
    REQUIRE((*license->resource)->media_type() == "text/plain");
    const std::string license_text(
        (*license->resource)->bytes().begin(),
        (*license->resource)->bytes().end()
    );
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
    REQUIRE(manager.require(resource::ResourceKey("fonts/default"))->resource->size() == 3);
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
    REQUIRE(found->resource.has_value());
    REQUIRE(found->consistency == resource::ResourceConsistency::non_snapshot);
    REQUIRE_FALSE(resource::is_snapshot(found->consistency));
    REQUIRE(found->generation.value() == 0);
    REQUIRE((*found->resource)->size() == 3);
    const auto missing = (*backend)->find(resource::ResourceKey("data/missing"));
    REQUIRE(missing.has_value());
    REQUIRE_FALSE(missing->resource.has_value());
    REQUIRE(missing->generation.value() == 0);
    REQUIRE(missing->consistency == resource::ResourceConsistency::non_snapshot);
    const auto missing_stream = (*backend)->open_stream(resource::ResourceKey("data/missing"));
    REQUIRE(missing_stream.has_value());
    REQUIRE_FALSE(missing_stream->stream.has_value());
    REQUIRE(missing_stream->consistency == resource::ResourceConsistency::non_snapshot);
    std::filesystem::remove(temp.path / "asset.bin");
    (*backend).reset();
    REQUIRE((*found->resource)->bytes()[2] == static_cast<std::uint8_t>('c'));
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
    REQUIRE(stream->stream->id() == resource_id);
    REQUIRE(stream->stream->size() == 6);
    REQUIRE(stream->stream->seekable());
    std::array<std::uint8_t, 4> buffer {};
    REQUIRE(stream->stream->read(buffer) == 4);
    REQUIRE(std::string(buffer.begin(), buffer.end()) == "abcd");
    REQUIRE(stream->stream->seek(2).has_value());
    REQUIRE(stream->stream->read(buffer) == 4);
    REQUIRE(std::string(buffer.begin(), buffer.end()) == "cdef");
    REQUIRE(stream->stream->read(buffer) == 0);
    REQUIRE_FALSE(stream->stream->seek(7).has_value());

    REQUIRE(manager.unmount(mount));
    (*backend).reset();
    // Windows 不允许删除仍被打开的文件；用 error_code 容忍，测试意图是“流在删除后仍可读”。
    std::error_code remove_error;
    std::filesystem::remove(temp.path / "large.bin", remove_error);
    REQUIRE(stream->stream->seek(0).has_value());
    REQUIRE(stream->stream->read(buffer) == 4);
    REQUIRE(std::string(buffer.begin(), buffer.end()) == "abcd");
}

#ifdef NANDINA_HAS_RESOURCE_SQLITE
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
    REQUIRE(blob->resource.has_value());
    REQUIRE(blob->consistency == resource::ResourceConsistency::package_snapshot);
    REQUIRE(resource::is_snapshot(blob->consistency));
    REQUIRE(blob->generation.value() == 0);
    REQUIRE((*blob->resource)->key().value() == "fonts/default");
    REQUIRE((*blob->resource)->storage() == resource::ResourceStorage::embedded_blob);
    REQUIRE((*blob->resource)->bytes()[2] == 3);

    auto external = (*backend)->find(resource::ResourceKey("shaders/basic"));
    REQUIRE(external.has_value());
    REQUIRE(external->resource.has_value());
    REQUIRE(external->consistency == resource::ResourceConsistency::non_snapshot);
    REQUIRE_FALSE(resource::is_snapshot(external->consistency));
    REQUIRE((*external->resource)->storage() == resource::ResourceStorage::external_file);
    REQUIRE((*external->resource)->size() == 4);

    const auto absent = (*backend)->find(resource::ResourceKey("fonts/absent"));
    REQUIRE(absent.has_value());
    REQUIRE_FALSE(absent->resource.has_value());
    REQUIRE(absent->generation.value() == 0);
    REQUIRE(absent->consistency == resource::ResourceConsistency::package_snapshot);
    const auto absent_stream = (*backend)->open_stream(resource::ResourceKey("fonts/absent"));
    REQUIRE(absent_stream.has_value());
    REQUIRE_FALSE(absent_stream->stream.has_value());
    REQUIRE(absent_stream->consistency == resource::ResourceConsistency::package_snapshot);

    auto blob_stream = (*backend)->open_stream(resource::ResourceKey("fonts/ui"));
    REQUIRE(blob_stream.has_value());
    REQUIRE(blob_stream->stream.has_value());
    REQUIRE(blob_stream->consistency == resource::ResourceConsistency::package_snapshot);
    std::array<std::uint8_t, 8> bytes {};
    REQUIRE((*blob_stream->stream)->read(bytes) == 3);
    REQUIRE(bytes[0] == 1);
    REQUIRE(bytes[2] == 3);

    auto external_stream = (*backend)->open_stream(id("10112233-4455-4677-8899-aabbccddeeff"));
    REQUIRE(external_stream.has_value());
    REQUIRE(external_stream->stream.has_value());
    REQUIRE(external_stream->consistency == resource::ResourceConsistency::non_snapshot);
    REQUIRE((*external_stream->stream)->read(bytes) == 4);
    REQUIRE(std::string(bytes.begin(), bytes.begin() + 4) == "main");

    std::error_code remove_error;
    std::filesystem::remove(database, remove_error);
    (*backend).reset();
    REQUIRE((*blob->resource)->bytes()[0] == 1);
    REQUIRE((*external_stream->stream)->seek(0).has_value());
    REQUIRE((*external_stream->stream)->read(bytes) == 4);
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
#endif
