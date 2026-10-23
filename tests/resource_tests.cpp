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
#include <nandina/resource/resource_scanner.hpp>
#include <nandina/resource/resource_uri.hpp>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <filesystem>
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
    REQUIRE((*manager.require(key))->bytes()[0] == 4);
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

    std::optional<resource::ResourceLookup> lookup;
    std::jthread finder([&] { lookup = manager.find(key); });
    backend->wait_until_entered();
    // 查询停在后端期间管理器必须仍可卸载；否则说明后端调用发生在管理器锁内。
    REQUIRE(manager.unmount(mount));
    backend->release();
    finder.join();

    REQUIRE(lookup.has_value());
    REQUIRE(lookup->has_value());
    REQUIRE((***lookup)->bytes()[0] == 7);

    // 卸载完成后新发起的解析不得成功；已经固定的旧结果继续可用。
    const auto after = manager.find(key);
    REQUIRE(after.has_value());
    REQUIRE_FALSE(after->has_value());
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
    // Windows 不允许删除仍被打开的文件；用 error_code 容忍，测试意图是“流在删除后仍可读”。
    std::error_code remove_error;
    std::filesystem::remove(temp.path / "large.bin", remove_error);
    REQUIRE((*stream)->seek(0).has_value());
    REQUIRE((*stream)->read(buffer) == 4);
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
