#include "app/nan_application.hpp"
#include "app/nan_router.hpp"
#include "resource/build_location.hpp"
#include "resource/resource.hpp"
#include "scene/control.hpp"
#include "widget/controls.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

#include <sqlite3.h>

using namespace nandina;

namespace
{
    struct ServiceCapture {
        resource::ResourceManager* resources = nullptr;
        text::FontLoader* loader = nullptr;
        text::FontFamilyRegistry* families = nullptr;
    };

    struct ServiceParams { ServiceCapture* capture = nullptr; };

    class ServicePage final: public app::NanPageT<ServiceParams> {
    public:
        explicit ServicePage(ServiceParams params): NanPageT(params) {}
        [[nodiscard]] auto route_key() const -> std::string_view override { return "services"; }
        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override {
            REQUIRE(context.has_resource_services());
            params().capture->resources = &context.resources();
            params().capture->loader = &context.font_loader();
            params().capture->families = &context.font_families();
            return std::make_shared<scene::NanControl>();
        }
    };

    class DefaultStartupPage final: public app::NanPageT<app::NoParams> {
    public:
        DefaultStartupPage() = default;
        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "startup";
        }
        [[nodiscard]] auto build(app::PageContext&) -> std::shared_ptr<scene::NanNode2D> override {
            return std::make_shared<scene::NanControl>();
        }
    };

    class RecommendedMainPage final: public app::Page<> {
    public:
        [[nodiscard]] auto build(widget::BuildContext& ui) -> widget::View override {
            return ui.make<widget::Label>("Recommended entry").build();
        }
    };

    static_assert(requires(app::NanApplication& application, app::WindowConfig config) {
        { application.run_page<DefaultStartupPage>(std::move(config)) } -> std::same_as<int>;
    });
    static_assert(
        requires(app::NanApplication& application, app::WindowConfig config, ServiceParams params) {
            { application.run_page<ServicePage>(std::move(config), params) } -> std::same_as<int>;
        }
    );
    static_assert(requires(app::NanApplication& application, app::WindowConfig config) {
        {
            application.run(std::move(config), [](widget::BuildContext& ui) {
                return ui.make<widget::Label>("Hello");
            })
        } -> std::same_as<int>;
    });
    static_assert(requires(app::RunConfig config) {
        {
            app::run(std::move(config), [](widget::BuildContext& ui) {
                return ui.make<widget::Label>("Hello");
            })
        } -> std::same_as<int>;
    });
    static_assert(requires(app::RunConfig config) {
        { app::run<RecommendedMainPage>(std::move(config)) } -> std::same_as<int>;
    });

    [[maybe_unused]] auto compile_default_page_runner(
        app::NanApplication& application,
        app::WindowConfig config
    ) -> int {
        return application.run_page<DefaultStartupPage>(std::move(config));
    }
} // namespace

TEST_CASE("NanApplication always installs builtin resource and font services", "[app][resource]") {
    app::NanApplication application;
    auto font = application.resources().require(resource::ResourceKey("fonts/default"));
    REQUIRE(font.has_value());
    REQUIRE((*font)->storage() == resource::ResourceStorage::embedded_blob);
    auto family = application.font_families().resolve({}, application.font_loader());
    REQUIRE(family.has_value());
    // 默认族首位必须是内置拉丁字体；系统 CJK 回退（若存在）追加在后。
    REQUIRE_FALSE(family->faces.empty());
    REQUIRE(family->specs.front().resource == resource::ResourceKey("fonts/default"));
}

TEST_CASE("functional root views use existing page context and concrete nodes", "[app][view]") {
    app::NanApplication application;
    app::NanRouter router {application.graph(), application.theme_manager()};
    bool received_page_context = false;
    auto params = app::detail::make_root_view_params([&](app::PageContext& context) {
        received_page_context = true;
        return context.ui().make<widget::Label>("Functional root");
    });

    (void)router.push<app::detail::RootViewPage>(std::move(params));

    REQUIRE(received_page_context);
    REQUIRE(router.depth() == 1);
    REQUIRE(router.current_key() == "root");
    REQUIRE(router.host()->child_count() == 1);
    REQUIRE(dynamic_cast<widget::Label*>(router.host()->get_child(0)) != nullptr);
}

TEST_CASE("functional root views accept BuildContext-only factories", "[app][view]") {
    app::NanApplication application;
    app::NanRouter router {application.graph(), application.theme_manager()};
    bool received_build_context = false;
    auto params = app::detail::make_root_view_params([&](widget::BuildContext& ui) {
        received_build_context = true;
        return ui.make<widget::Button>("Continue");
    });

    (void)router.push<app::detail::RootViewPage>(std::move(params));

    REQUIRE(received_build_context);
    REQUIRE(dynamic_cast<widget::Button*>(router.host()->get_child(0)) != nullptr);
}

TEST_CASE("recommended pages adapt BuildContext onto the existing router", "[app][page]") {
    app::NanApplication application;
    app::NanRouter router {application.graph(), application.theme_manager()};

    (void)router.push<RecommendedMainPage>();

    REQUIRE(router.depth() == 1);
    REQUIRE(router.host()->child_count() == 1);
    REQUIRE(dynamic_cast<widget::Label*>(router.host()->get_child(0)) != nullptr);
}

TEST_CASE("optional font fallback is absent without a project package", "[app][resource][font]") {
    app::NanApplication application;
    auto registered = text::register_optional_font_fallback(
        application.font_families(),
        application.resources(),
        resource::ResourceKey("families/zh-cn"),
        resource::ResourceKey("fonts/fallback/zh-cn")
    );
    REQUIRE(registered.has_value());
    REQUIRE_FALSE(*registered);
}

TEST_CASE("NanApplication rejects malformed discovered packages", "[app][resource]") {
    const auto root = std::filesystem::temp_directory_path()
        / ("nandina-app-" + resource::ResourceId::random().to_string());
    std::filesystem::create_directories(root / "resources");
    { std::ofstream file(root / "resources/resources.db"); file << "not sqlite"; }
    REQUIRE_THROWS_AS(
        app::NanApplication(app::NanApplicationConfig {
            .application_id = "org.nandina.invalid",
            .executable_path = root / "application",
            .environment = {{"HOME", "/nonexistent-home"}},
        }),
        std::runtime_error
    );
    std::error_code error;
    std::filesystem::remove_all(root, error);
}

namespace
{
    void exec(sqlite3* db, const char* sql) {
        char* message = nullptr;
        const int result = sqlite3_exec(db, sql, nullptr, nullptr, &message);
        if (message) { sqlite3_free(message); }
        REQUIRE(result == SQLITE_OK);
    }

    void create_package_database(const std::filesystem::path& path) {
        sqlite3* db = nullptr;
        REQUIRE(sqlite3_open(path.string().c_str(), &db) == SQLITE_OK);
        exec(db, "PRAGMA application_id=1312902724; PRAGMA user_version=1;");
        exec(db,
            "CREATE TABLE resources("
            "id BLOB PRIMARY KEY, resource_key TEXT NOT NULL UNIQUE, media_type TEXT NOT NULL,"
            "storage INTEGER NOT NULL, data BLOB, external_path TEXT, size INTEGER NOT NULL);"
            "CREATE TABLE aliases(alias TEXT PRIMARY KEY, resource_id BLOB NOT NULL);"
        );
        exec(db,
            "INSERT INTO resources VALUES("
            "X'20112233445546778899aabbccddeeff','assets/hello','text/plain',0,"
            "X'68656c6c6f',NULL,5);"
        );
        REQUIRE(sqlite3_close(db) == SQLITE_OK);
    }
} // namespace

TEST_CASE("NanApplication mounts the build-tree package from resource-location.json", "[app][resource][d2]") {
    const auto root = std::filesystem::temp_directory_path()
        / ("nandina-d2-" + resource::ResourceId::random().to_string());
    std::filesystem::create_directories(root / "resources");       // executable-relative resources root
    std::filesystem::create_directories(root / "build-package");   // the pointed build-tree package

    create_package_database(root / "build-package" / "resources.db");
    {
        std::ofstream metadata(root / "resources" / "resource-location.json");
        metadata << "{\"package_id\":\"org.nandina.d2\",\"package_root\":\""
                 << (root / "build-package").string()
                 << "\",\"database\":\"resources.db\"}\n";
    }

    // executable_path = root/application => executable-relative root = root/resources,
    // which holds only resource-location.json (no direct resources.db), so the package
    // is reachable only through the build metadata.
    app::NanApplication application(app::NanApplicationConfig {
        .application_id = "org.nandina.d2",
        .executable_path = root / "application",
        .environment = {{"HOME", "/nonexistent-home"}},
    });

    const auto hello = application.resources().require(resource::ResourceKey("assets/hello"));
    REQUIRE(hello.has_value());
    REQUIRE((*hello)->size() == 5);
    REQUIRE((*hello)->bytes()[0] == 'h');

    std::error_code error;
    std::filesystem::remove_all(root, error);
}

TEST_CASE("read_build_location_metadata parses, rejects missing, and rejects malformed", "[resource][d2]") {
    const auto root = std::filesystem::temp_directory_path()
        / ("nandina-d2-meta-" + resource::ResourceId::random().to_string());
    std::filesystem::create_directories(root);

    const auto valid =
        resource::read_build_location_metadata(root / "resource-location.json");
    REQUIRE_FALSE(valid.has_value()); // missing file

    {
        std::ofstream file(root / "resource-location.json");
        file << "{\"package_id\":\"org.nandina.meta\",\"package_root\":\"/tmp/pkg\",\"database\":\"resources.db\"}";
    }
    const auto parsed = resource::read_build_location_metadata(root / "resource-location.json");
    REQUIRE(parsed.has_value());
    REQUIRE(parsed->package_id == "org.nandina.meta");
    REQUIRE(parsed->package_root == std::filesystem::path("/tmp/pkg"));
    REQUIRE(parsed->database == "resources.db");

    { std::ofstream file(root / "resource-location.json"); file << "{not json"; }
    const auto malformed = resource::read_build_location_metadata(root / "resource-location.json");
    REQUIRE_FALSE(malformed.has_value());

    std::error_code error;
    std::filesystem::remove_all(root, error);
}
